/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/Atapi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DiskInfo.h>
#include <Emex64Platform.h>
#include <Emex64Pcie.h>

#define SATA_PORTS          8
#define SATA_MAX_COUNT      65536
#define SATA_TIMEOUT_US     5000000

#pragma pack (1)
typedef struct {
    VENDOR_DEVICE_PATH Controller;
    SATA_DEVICE_PATH Sata;
    EFI_DEVICE_PATH_PROTOCOL End;
} SATA_DISK_DEVICE_PATH;

typedef struct {
  UINT64 Opcode;
  UINT64 Lba;
  UINT64 Count;
  UINT64 Buffer;
  UINT64 Status;
  UINT64 Bytes;
  UINT64 Reserved[2];
} SATA_SLOT;
#pragma pack ()

#define SATA_DISK_SIGNATURE SIGNATURE_32('S', 'A', 'T', 'A')

typedef struct {
  UINT32 Signature;
  UINTN Port;
  UINT64 PortRegs;
  SATA_SLOT *Slots;
  EFI_BLOCK_IO_PROTOCOL BlockIo;
  EFI_BLOCK_IO_MEDIA Media;
  EFI_DISK_INFO_PROTOCOL DiskInfo;
  ATA_IDENTIFY_DATA Identify;
  SATA_DISK_DEVICE_PATH DevicePath;
} SATA_DISK;

#define SATA_DISK_FROM_BLOCK_IO(a) CR(a, SATA_DISK, BlockIo, SATA_DISK_SIGNATURE)
#define SATA_DISK_FROM_DISK_INFO(a) CR(a, SATA_DISK, DiskInfo, SATA_DISK_SIGNATURE)

STATIC EFI_STATUS SataCommand(IN SATA_DISK *Disk,
                              IN UINT64 Opcode,
                              IN UINT64 Lba,
                              IN UINT64 Count,
                              IN VOID *Buffer)
{
    SATA_SLOT *Slot;
    UINTN Waited;

    Slot = &Disk->Slots[0];
    ZeroMem(Slot, sizeof (*Slot));
    Slot->Opcode = Opcode;
    Slot->Lba = Lba;
    Slot->Count = Count;
    Slot->Buffer = (UINT64)(UINTN)Buffer;
    MemoryFence();

    MmioWrite64(Disk->PortRegs + EMEX64_SATA_P_IS, MAX_UINT64);
    MmioWrite64(Disk->PortRegs + EMEX64_SATA_P_CI, BIT0);

    for(Waited = 0; (MmioRead64(Disk->PortRegs + EMEX64_SATA_P_CI) & BIT0) != 0; Waited += 10)
    {
        if(Waited >= SATA_TIMEOUT_US)
        {
            DEBUG((DEBUG_ERROR, "SataDxe: port %u: command 0x%Lx timed out\n", Disk->Port, Opcode));
            return EFI_TIMEOUT;
        }
        MicroSecondDelay(10);
    }

    MemoryFence();
    if((Slot->Status & EMEX64_SATA_STATUS_ERROR) != 0)
    {
        DEBUG((DEBUG_ERROR, "SataDxe: port %u: command 0x%Lx, LBA %Lu, %Lu sector(s): error %Lu\n", Disk->Port, Opcode, Lba, Count, (Slot->Status >> 8) & 0xFF));
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

STATIC EFI_STATUS SataTransfer(IN SATA_DISK *Disk,
                               IN UINT32 MediaId,
                               IN EFI_LBA Lba,
                               IN UINTN BufferSize,
                               IN VOID *Buffer,
                               IN BOOLEAN Write)
{
    EFI_STATUS Status;
    UINT64 Blocks;
    UINT64 Count;
    UINT8 *Data;

    if(MediaId != Disk->Media.MediaId)
    {
        return EFI_MEDIA_CHANGED;
    }

    if(Write && Disk->Media.ReadOnly)
    {
        return EFI_WRITE_PROTECTED;
    }

    if(Buffer == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    if(BufferSize == 0)
    {
        return EFI_SUCCESS;
    }

    if((BufferSize % Disk->Media.BlockSize) != 0)
    {
        return EFI_BAD_BUFFER_SIZE;
    }

    Blocks = BufferSize / Disk->Media.BlockSize;
    if((Lba > Disk->Media.LastBlock) || (Blocks - 1 > Disk->Media.LastBlock - Lba))
    {
        return EFI_INVALID_PARAMETER;
    }

    Data = Buffer;
    while(Blocks > 0)
    {
        Count = MIN (Blocks, SATA_MAX_COUNT);
        Status = SataCommand(Disk, Write ? EMEX64_SATA_WRITE_DMA_EXT : EMEX64_SATA_READ_DMA_EXT, Lba, Count, Data);
        if(EFI_ERROR(Status))
        {
            return Status;
        }
        Lba += Count;
        Blocks -= Count;
        Data += Count * Disk->Media.BlockSize;
    }

    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI SataReset(IN EFI_BLOCK_IO_PROTOCOL *This,
                                   IN BOOLEAN ExtendedVerification)
{
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI SataReadBlocks(IN EFI_BLOCK_IO_PROTOCOL *This,
                                        IN UINT32 MediaId,
                                        IN EFI_LBA Lba,
                                        IN UINTN BufferSize,
                                        OUT VOID *Buffer)
{
    return SataTransfer(SATA_DISK_FROM_BLOCK_IO(This), MediaId, Lba, BufferSize, Buffer, FALSE);
}

STATIC EFI_STATUS EFIAPI SataWriteBlocks(IN EFI_BLOCK_IO_PROTOCOL *This,
                                         IN UINT32 MediaId,
                                         IN EFI_LBA Lba,
                                         IN UINTN BufferSize,
                                         IN VOID *Buffer)
{
    return SataTransfer(SATA_DISK_FROM_BLOCK_IO(This), MediaId, Lba, BufferSize, Buffer, TRUE);
}

STATIC EFI_STATUS EFIAPI SataFlushBlocks(IN EFI_BLOCK_IO_PROTOCOL *This)
{
    SATA_DISK  *Disk = SATA_DISK_FROM_BLOCK_IO(This);
    if(Disk->Media.ReadOnly)
    {
        return EFI_SUCCESS;
    }
    return SataCommand(Disk, EMEX64_SATA_FLUSH_EXT, 0, 0, NULL);
}

STATIC EFI_STATUS EFIAPI SataDiskInfoInquiry(IN EFI_DISK_INFO_PROTOCOL *This,
                                             IN OUT VOID *InquiryData,
                                             IN OUT UINT32 *InquiryDataSize)
{
    return EFI_NOT_FOUND;
}

STATIC EFI_STATUS EFIAPI SataDiskInfoIdentify(IN EFI_DISK_INFO_PROTOCOL *This,
                                              IN OUT VOID *IdentifyData,
                                              IN OUT UINT32 *IdentifyDataSize)
{
    SATA_DISK *Disk = SATA_DISK_FROM_DISK_INFO(This);
    if(*IdentifyDataSize < sizeof(Disk->Identify))
    {
        *IdentifyDataSize = sizeof(Disk->Identify);
        return EFI_BUFFER_TOO_SMALL;
    }
    CopyMem(IdentifyData, &Disk->Identify, sizeof(Disk->Identify));
    *IdentifyDataSize = sizeof(Disk->Identify);
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI SataDiskInfoSenseData(IN EFI_DISK_INFO_PROTOCOL *This,
                                               IN OUT VOID *SenseData,
                                               IN OUT UINT32 *SenseDataSize,
                                               OUT UINT8 *SenseDataNumber)
{
    return EFI_NOT_FOUND;
}

STATIC EFI_STATUS EFIAPI SataDiskInfoWhichIde(IN EFI_DISK_INFO_PROTOCOL *This,
                                              OUT UINT32 *IdeChannel,
                                              OUT UINT32 *IdeDevice)
{
  SATA_DISK *Disk = SATA_DISK_FROM_DISK_INFO(This);
  *IdeChannel = (UINT32)Disk->Port;
  *IdeDevice = 0;
  return EFI_SUCCESS;
}

STATIC VOID AtaString(OUT CHAR8 *Field,
                      IN UINTN Size,
                      IN CONST UINT8 *Source,
                      IN UINTN SourceSize)
{
    UINTN Index;
    CHAR8 Char;
    for(Index = 0; Index < Size; Index++)
    {
        Char = (Index < SourceSize) ? (CHAR8)Source[Index] : ' ';
        if((Char == '\0') || (Char < ' '))
        {
            Char = ' ';
        }
        Field[Index ^ 1] = Char;
    }
}

STATIC VOID AddDisk(IN UINT64 Regs,
                    IN UINTN Port)
{
    SATA_DISK *Disk = AllocateZeroPool(sizeof (*Disk));
    UINT8 *Raw = AllocatePages(EFI_SIZE_TO_PAGES(EMEX64_SATA_SLOTS * EMEX64_SATA_SLOT_SIZE + 512));
    if((Disk == NULL) || (Raw == NULL))
    {
        return;
    }

    ZeroMem(Raw, EMEX64_SATA_SLOTS * EMEX64_SATA_SLOT_SIZE + 512);

    Disk->Signature = SATA_DISK_SIGNATURE;
    Disk->Port = Port;
    Disk->PortRegs = Regs + EMEX64_SATA_PORT(Port);
    Disk->Slots = (SATA_SLOT *)Raw;

    MmioWrite64(Disk->PortRegs + EMEX64_SATA_P_IE, 0);
    MmioWrite64(Disk->PortRegs + EMEX64_SATA_P_CLB, (UINT64)(UINTN)Raw);
    MmioWrite64(Disk->PortRegs + EMEX64_SATA_P_CMD, EMEX64_SATA_CMD_ST);

    UINT64 Ssts = MmioRead64(Disk->PortRegs + EMEX64_SATA_P_SSTS);
    Disk->Media.MediaId = 1;
    Disk->Media.RemovableMedia = FALSE;
    Disk->Media.MediaPresent = TRUE;
    Disk->Media.LogicalPartition = FALSE;
    Disk->Media.ReadOnly = (Ssts & EMEX64_SATA_SSTS_READONLY) != 0;
    Disk->Media.WriteCaching = FALSE;
    Disk->Media.BlockSize = (UINT32)MmioRead64(Disk->PortRegs + EMEX64_SATA_P_SECSZ);
    Disk->Media.IoAlign = 0;
    Disk->Media.LastBlock = MmioRead64(Disk->PortRegs + EMEX64_SATA_P_SECTORS) - 1;
    Disk->Media.LogicalBlocksPerPhysicalBlock = 1;
    if(Disk->Media.BlockSize == 0)
    {
        Disk->Media.BlockSize = 512;
    }

    Disk->BlockIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION2;
    Disk->BlockIo.Media = &Disk->Media;
    Disk->BlockIo.Reset = SataReset;
    Disk->BlockIo.ReadBlocks  = SataReadBlocks;
    Disk->BlockIo.WriteBlocks = SataWriteBlocks;
    Disk->BlockIo.FlushBlocks = SataFlushBlocks;
    
    if(!EFI_ERROR(SataCommand(Disk, EMEX64_SATA_IDENTIFY, 0, 1, Raw + EMEX64_SATA_SLOTS * EMEX64_SATA_SLOT_SIZE)))
    {
        UINT8 *Id = Raw + EMEX64_SATA_SLOTS * EMEX64_SATA_SLOT_SIZE;

        AtaString(Disk->Identify.ModelName, sizeof (Disk->Identify.ModelName), Id + 0x00, 40);
        AtaString(Disk->Identify.SerialNo, sizeof (Disk->Identify.SerialNo), Id + 0x28, 20);
    }
    else
    {
        AtaString(Disk->Identify.ModelName, sizeof (Disk->Identify.ModelName), (CONST UINT8 *)"emex64 SATA disk", 16);
    }

    CopyGuid(&Disk->DiskInfo.Interface, &gEfiDiskInfoAhciInterfaceGuid);
    Disk->DiskInfo.Inquiry = SataDiskInfoInquiry;
    Disk->DiskInfo.Identify = SataDiskInfoIdentify;
    Disk->DiskInfo.SenseData = SataDiskInfoSenseData;
    Disk->DiskInfo.WhichIde = SataDiskInfoWhichIde;

    Disk->DevicePath.Controller.Header.Type = HARDWARE_DEVICE_PATH;
    Disk->DevicePath.Controller.Header.SubType = HW_VENDOR_DP;
    SetDevicePathNodeLength (&Disk->DevicePath.Controller.Header, sizeof (VENDOR_DEVICE_PATH));
    CopyGuid (&Disk->DevicePath.Controller.Guid, &gEmex64SataDevicePathGuid);
    Disk->DevicePath.Sata.Header.Type = MESSAGING_DEVICE_PATH;
    Disk->DevicePath.Sata.Header.SubType = MSG_SATA_DP;
    SetDevicePathNodeLength (&Disk->DevicePath.Sata.Header, sizeof (SATA_DEVICE_PATH));
    Disk->DevicePath.Sata.HBAPortNumber = (UINT16)Port;
    Disk->DevicePath.Sata.PortMultiplierPortNumber = 0xFFFF;
    Disk->DevicePath.Sata.Lun = 0;
    SetDevicePathEndNode (&Disk->DevicePath.End);

    EFI_HANDLE Handle = NULL;
    EFI_STATUS Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiDevicePathProtocolGuid, &Disk->DevicePath, &gEfiBlockIoProtocolGuid, &Disk->BlockIo, &gEfiDiskInfoProtocolGuid, &Disk->DiskInfo, NULL);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "SataDxe: port %u: %Lu MiB%a\n", Port, MultU64x32(Disk->Media.LastBlock + 1, Disk->Media.BlockSize) >> 20, Disk->Media.ReadOnly ? ", read only" : ""));
}

STATIC UINT64 FindController(VOID)
{
    UINT64  Cfg;
    Cfg = Emex64PcieFind(EMEX64_SATA_DEVICE_ID);
    return (Cfg != 0) ? Emex64PcieMapBar0(Cfg) : 0;
}

EFI_STATUS EFIAPI SataDxeInitialize(IN EFI_HANDLE ImageHandle,
                                    IN EFI_SYSTEM_TABLE *SystemTable)
{
    UINT64 Regs = FindController();
    if(Regs == 0)
    {
        DEBUG((DEBUG_INFO, "SataDxe: no SATA host controller\n"));
        return EFI_UNSUPPORTED;
    }

    MmioWrite64(Regs + EMEX64_SATA_GHC, EMEX64_SATA_GHC_RESET);
    UINT64 Implemented = MmioRead64(Regs + EMEX64_SATA_PI);

    for(UINTN Port = 0; Port < SATA_PORTS; Port++)
    {
        if(((Implemented & LShiftU64(1, Port)) != 0) && ((MmioRead64(Regs + EMEX64_SATA_PORT(Port) + EMEX64_SATA_P_SSTS) & EMEX64_SATA_SSTS_PRESENT) != 0))
        {
            AddDisk(Regs, Port);
        }
    }

    return EFI_SUCCESS;
}
