/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/Nvme.h>
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
#include <Protocol/NvmExpressPassthru.h>
#include <Emex64Platform.h>
#include <Emex64Pcie.h>

#define NVME_PAGE               4096
#define NVME_DEPTH              64
#define NVME_TIMEOUT_US         5000000
#define NVME_MAX_TRANSFER       (128 * NVME_PAGE)
#define NVME_IO_QUEUE_ID        1
#define NVME_NAMESPACES         4
#define NVME_NS_NSATTR          99
#define NVME_NS_NGUID           104

#pragma pack (1)
typedef struct {
    UINT8 Opcode;
    UINT8 Flags;
    UINT16 Cid;
    UINT32 Nsid;
    UINT32 Cdw2;
    UINT32 Cdw3;
    UINT64 Mptr;
    UINT64 Prp1;
    UINT64 Prp2;
    UINT32 Cdw10;
    UINT32 Cdw11;
    UINT32 Cdw12;
    UINT32 Cdw13;
    UINT32 Cdw14;
    UINT32 Cdw15;
} NVME_SQE;

typedef struct {
    UINT32 Dw0;
    UINT32 Dw1;
    UINT16 SqHead;
    UINT16 SqId;
    UINT16 Cid;
    UINT16 Status;
} NVME_CQE;

typedef struct {
    VENDOR_DEVICE_PATH Controller;
    EFI_DEVICE_PATH_PROTOCOL End;
} NVME_CONTROLLER_DEVICE_PATH;

typedef struct {
    VENDOR_DEVICE_PATH Controller;
    NVME_NAMESPACE_DEVICE_PATH Namespace;
    EFI_DEVICE_PATH_PROTOCOL End;
} NVME_DISK_DEVICE_PATH;
#pragma pack ()

typedef struct {
    NVME_SQE *Sq;
    NVME_CQE *Cq;
    UINT16 Tail;
    UINT16 Head;
    UINT16 Phase;
} NVME_QUEUE;

#define NVME_CONTROLLER_SIGNATURE SIGNATURE_32('N', 'V', 'M', 'C')
#define NVME_DISK_SIGNATURE SIGNATURE_32('N', 'V', 'M', 'D')

typedef struct {
    UINT32 Signature;
    UINT64 Regs;
    NVME_QUEUE Queues[2];
    UINT64 *Prps;
    UINT16 Cid;
    UINT32 Namespaces;
    UINT64 Eui[NVME_NAMESPACES];
    EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL PassThru;
    EFI_NVM_EXPRESS_PASS_THRU_MODE PassThruMode;
    NVME_CONTROLLER_DEVICE_PATH DevicePath;
    EFI_HANDLE Handle;
} NVME_CONTROLLER;

typedef struct {
    UINT32 Signature;
    NVME_CONTROLLER *Controller;
    UINT32 Nsid;
    EFI_BLOCK_IO_PROTOCOL BlockIo;
    EFI_BLOCK_IO_MEDIA Media;
    NVME_DISK_DEVICE_PATH DevicePath;
} NVME_DISK;

#define NVME_CONTROLLER_FROM_PASS_THRU(a) CR(a, NVME_CONTROLLER, PassThru, NVME_CONTROLLER_SIGNATURE)
#define NVME_DISK_FROM_BLOCK_IO(a) CR(a, NVME_DISK, BlockIo, NVME_DISK_SIGNATURE)

STATIC EFI_STATUS NvmePrps(IN NVME_CONTROLLER *Controller,
                           IN NVME_SQE *Sqe,
                           IN VOID *Buffer,
                           IN UINTN Length)
{
    UINT64 Address = (UINT64)(UINTN)Buffer;
    UINT64 Next;
    UINTN Pages;
    UINTN Index;

    Sqe->Prp1 = 0;
    Sqe->Prp2 = 0;
    if(Length == 0)
    {
        return EFI_SUCCESS;
    }
    if((Address & 3) != 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    Sqe->Prp1 = Address;
    Next = (Address & ~(UINT64)(NVME_PAGE - 1)) + NVME_PAGE;
    if(Address + Length <= Next)
    {
        return EFI_SUCCESS;
    }

    Pages = (UINTN)((Address + Length - Next + NVME_PAGE - 1) / NVME_PAGE);
    if(Pages == 1)
    {
        Sqe->Prp2 = Next;
        return EFI_SUCCESS;
    }
    if(Pages > NVME_PAGE / sizeof(UINT64))
    {
        return EFI_BAD_BUFFER_SIZE;
    }
    for(Index = 0; Index < Pages; Index++)
    {
        Controller->Prps[Index] = Next + Index * NVME_PAGE;
    }
    Sqe->Prp2 = (UINT64)(UINTN)Controller->Prps;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS NvmeCommand(IN NVME_CONTROLLER *Controller,
                              IN UINT16 QueueId,
                              IN OUT NVME_SQE *Sqe,
                              IN VOID *Buffer,
                              IN UINTN Length,
                              OUT NVME_CQE *Result OPTIONAL)
{
    NVME_QUEUE *Queue = &Controller->Queues[QueueId];
    volatile NVME_CQE *Cqe;
    EFI_STATUS Status;
    UINTN Waited;

    if(Buffer != NULL)
    {
        Status = NvmePrps(Controller, Sqe, Buffer, Length);
        if(EFI_ERROR(Status))
        {
            return Status;
        }
    }

    Sqe->Cid = Controller->Cid++;
    CopyMem(&Queue->Sq[Queue->Tail], Sqe, sizeof(*Sqe));
    Queue->Tail = (Queue->Tail + 1) % NVME_DEPTH;
    MemoryFence();
    MmioWrite64(Controller->Regs + EMEX64_NVME_SQ_TAIL(QueueId), Queue->Tail);

    Cqe = &Queue->Cq[Queue->Head];
    for(Waited = 0; (Cqe->Status & 1) != Queue->Phase; Waited += 10)
    {
        if(Waited >= NVME_TIMEOUT_US)
        {
            DEBUG((DEBUG_ERROR, "NvmeDxe: queue %u: command 0x%x timed out\n", QueueId, Sqe->Opcode));
            return EFI_TIMEOUT;
        }
        MicroSecondDelay(10);
        MemoryFence();
    }
    MemoryFence();

    if(Result != NULL)
    {
        CopyMem(Result, (VOID *)Cqe, sizeof(*Result));
    }
    Queue->Head = (Queue->Head + 1) % NVME_DEPTH;
    if(Queue->Head == 0)
    {
        Queue->Phase ^= 1;
    }
    MmioWrite64(Controller->Regs + EMEX64_NVME_CQ_HEAD(QueueId), Queue->Head);

    if((Cqe->Status >> 1) != 0)
    {
        DEBUG((DEBUG_ERROR, "NvmeDxe: queue %u: command 0x%x: status 0x%x\n", QueueId, Sqe->Opcode, Cqe->Status >> 1));
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}

STATIC EFI_STATUS NvmeTransfer(IN NVME_DISK *Disk,
                               IN UINT32 MediaId,
                               IN EFI_LBA Lba,
                               IN UINTN BufferSize,
                               IN VOID *Buffer,
                               IN BOOLEAN Write)
{
    NVME_SQE Sqe;
    EFI_STATUS Status;
    UINT64 Blocks;
    UINTN Length;
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
    if(((UINTN)Buffer & (Disk->Media.IoAlign - 1)) != 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    Blocks = BufferSize / Disk->Media.BlockSize;
    if((Lba > Disk->Media.LastBlock) || (Blocks - 1 > Disk->Media.LastBlock - Lba))
    {
        return EFI_INVALID_PARAMETER;
    }

    Data = Buffer;
    while(BufferSize > 0)
    {
        Length = MIN(BufferSize, NVME_MAX_TRANSFER);
        Blocks = Length / Disk->Media.BlockSize;
        ZeroMem(&Sqe, sizeof(Sqe));
        Sqe.Opcode = Write ? NVME_IO_WRITE_OPC : NVME_IO_READ_OPC;
        Sqe.Nsid = Disk->Nsid;
        Sqe.Cdw10 = (UINT32)Lba;
        Sqe.Cdw11 = (UINT32)RShiftU64(Lba, 32);
        Sqe.Cdw12 = (UINT32)(Blocks - 1);
        Status = NvmeCommand(Disk->Controller, NVME_IO_QUEUE_ID, &Sqe, Data, Length, NULL);
        if(EFI_ERROR(Status))
        {
            return Status;
        }
        Lba += Blocks;
        Data += Length;
        BufferSize -= Length;
    }
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI NvmeReset(IN EFI_BLOCK_IO_PROTOCOL *This,
                                   IN BOOLEAN ExtendedVerification)
{
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI NvmeReadBlocks(IN EFI_BLOCK_IO_PROTOCOL *This,
                                        IN UINT32 MediaId,
                                        IN EFI_LBA Lba,
                                        IN UINTN BufferSize,
                                        OUT VOID *Buffer)
{
    return NvmeTransfer(NVME_DISK_FROM_BLOCK_IO(This), MediaId, Lba, BufferSize, Buffer, FALSE);
}

STATIC EFI_STATUS EFIAPI NvmeWriteBlocks(IN EFI_BLOCK_IO_PROTOCOL *This,
                                         IN UINT32 MediaId,
                                         IN EFI_LBA Lba,
                                         IN UINTN BufferSize,
                                         IN VOID *Buffer)
{
    return NvmeTransfer(NVME_DISK_FROM_BLOCK_IO(This), MediaId, Lba, BufferSize, Buffer, TRUE);
}

STATIC EFI_STATUS EFIAPI NvmeFlushBlocks(IN EFI_BLOCK_IO_PROTOCOL *This)
{
    NVME_DISK *Disk = NVME_DISK_FROM_BLOCK_IO(This);
    NVME_SQE Sqe;

    if(Disk->Media.ReadOnly)
    {
        return EFI_SUCCESS;
    }
    ZeroMem(&Sqe, sizeof(Sqe));
    Sqe.Opcode = NVME_IO_FLUSH_OPC;
    Sqe.Nsid = Disk->Nsid;
    return NvmeCommand(Disk->Controller, NVME_IO_QUEUE_ID, &Sqe, NULL, 0, NULL);
}

STATIC EFI_STATUS EFIAPI NvmePassThru(IN EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *This,
                                      IN UINT32 NamespaceId,
                                      IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET *Packet,
                                      IN EFI_EVENT Event OPTIONAL)
{
    NVME_CONTROLLER *Controller = NVME_CONTROLLER_FROM_PASS_THRU(This);
    EFI_NVM_EXPRESS_COMMAND *Command;
    NVME_SQE Sqe;
    NVME_CQE Cqe;
    EFI_STATUS Status;

    if((Packet == NULL) || (Packet->NvmeCmd == NULL) || (Packet->NvmeCompletion == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }
    if((Packet->QueueType != NVME_ADMIN_QUEUE) && (Packet->QueueType != NVME_IO_QUEUE))
    {
        return EFI_INVALID_PARAMETER;
    }
    if((Packet->TransferLength != 0) && (Packet->TransferBuffer == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }
    if((Packet->MetadataLength != 0) || (Packet->NvmeCmd->Cdw0.FusedOperation != NORMAL_CMD))
    {
        return EFI_UNSUPPORTED;
    }

    Command = Packet->NvmeCmd;
    ZeroMem(&Sqe, sizeof(Sqe));
    Sqe.Opcode = (UINT8)Command->Cdw0.Opcode;
    Sqe.Nsid = NamespaceId;
    Sqe.Cdw2 = (Command->Flags & CDW2_VALID) ? Command->Cdw2 : 0;
    Sqe.Cdw3 = (Command->Flags & CDW3_VALID) ? Command->Cdw3 : 0;
    Sqe.Cdw10 = (Command->Flags & CDW10_VALID) ? Command->Cdw10 : 0;
    Sqe.Cdw11 = (Command->Flags & CDW11_VALID) ? Command->Cdw11 : 0;
    Sqe.Cdw12 = (Command->Flags & CDW12_VALID) ? Command->Cdw12 : 0;
    Sqe.Cdw13 = (Command->Flags & CDW13_VALID) ? Command->Cdw13 : 0;
    Sqe.Cdw14 = (Command->Flags & CDW14_VALID) ? Command->Cdw14 : 0;
    Sqe.Cdw15 = (Command->Flags & CDW15_VALID) ? Command->Cdw15 : 0;

    ZeroMem(&Cqe, sizeof(Cqe));
    Status = NvmeCommand(Controller, Packet->QueueType == NVME_ADMIN_QUEUE ? 0 : NVME_IO_QUEUE_ID, &Sqe, Packet->TransferLength != 0 ? Packet->TransferBuffer : NULL, Packet->TransferLength, &Cqe);
    CopyMem(Packet->NvmeCompletion, &Cqe, sizeof(Cqe));
    return Status;
}

STATIC EFI_STATUS EFIAPI NvmeGetNextNamespace(IN EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *This,
                                              IN OUT UINT32 *NamespaceId)
{
    NVME_CONTROLLER *Controller = NVME_CONTROLLER_FROM_PASS_THRU(This);

    if(NamespaceId == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }
    if(*NamespaceId == MAX_UINT32)
    {
        if(Controller->Namespaces == 0)
        {
            return EFI_NOT_FOUND;
        }
        *NamespaceId = 1;
        return EFI_SUCCESS;
    }
    if((*NamespaceId == 0) || (*NamespaceId > Controller->Namespaces))
    {
        return EFI_INVALID_PARAMETER;
    }
    if(*NamespaceId == Controller->Namespaces)
    {
        return EFI_NOT_FOUND;
    }
    (*NamespaceId)++;
    return EFI_SUCCESS;
}

STATIC VOID NvmeNamespaceNode(IN NVME_CONTROLLER *Controller,
                              IN UINT32 NamespaceId,
                              OUT NVME_NAMESPACE_DEVICE_PATH *Node)
{
    Node->Header.Type = MESSAGING_DEVICE_PATH;
    Node->Header.SubType = MSG_NVME_NAMESPACE_DP;
    SetDevicePathNodeLength(&Node->Header, sizeof(*Node));
    Node->NamespaceId = NamespaceId;
    Node->NamespaceUuid = Controller->Eui[NamespaceId - 1];
}

STATIC EFI_STATUS EFIAPI NvmeBuildDevicePath(IN EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *This,
                                             IN UINT32 NamespaceId,
                                             OUT EFI_DEVICE_PATH_PROTOCOL **DevicePath)
{
    NVME_CONTROLLER *Controller = NVME_CONTROLLER_FROM_PASS_THRU(This);
    NVME_NAMESPACE_DEVICE_PATH *Node;

    if(DevicePath == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }
    if((NamespaceId == 0) || (NamespaceId > Controller->Namespaces))
    {
        return EFI_NOT_FOUND;
    }
    Node = AllocateZeroPool(sizeof(*Node));
    if(Node == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }
    NvmeNamespaceNode(Controller, NamespaceId, Node);
    *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)Node;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI NvmeGetNamespace(IN EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL *This,
                                          IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
                                          OUT UINT32 *NamespaceId)
{
    NVME_CONTROLLER *Controller = NVME_CONTROLLER_FROM_PASS_THRU(This);
    NVME_NAMESPACE_DEVICE_PATH *Node = (NVME_NAMESPACE_DEVICE_PATH *)DevicePath;

    if((DevicePath == NULL) || (NamespaceId == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }
    if((DevicePathType(DevicePath) != MESSAGING_DEVICE_PATH) || (DevicePathSubType(DevicePath) != MSG_NVME_NAMESPACE_DP) ||
       (DevicePathNodeLength(DevicePath) != sizeof(*Node)))
    {
        return EFI_UNSUPPORTED;
    }
    if((Node->NamespaceId == 0) || (Node->NamespaceId > Controller->Namespaces))
    {
        return EFI_NOT_FOUND;
    }
    *NamespaceId = Node->NamespaceId;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS NvmeWaitReady(IN UINT64 Regs,
                                IN BOOLEAN Ready)
{
    UINTN Waited;

    for(Waited = 0; ((MmioRead64(Regs + EMEX64_NVME_CSTS) & EMEX64_NVME_CSTS_RDY) != 0) != Ready; Waited += 10)
    {
        if((MmioRead64(Regs + EMEX64_NVME_CSTS) & EMEX64_NVME_CSTS_CFS) != 0)
        {
            return EFI_DEVICE_ERROR;
        }
        if(Waited >= NVME_TIMEOUT_US)
        {
            return EFI_TIMEOUT;
        }
        MicroSecondDelay(10);
    }

    return EFI_SUCCESS;
}

STATIC VOID *NvmePage(VOID)
{
    VOID *Page = AllocatePages(1);
    if(Page != NULL)
    {
        ZeroMem(Page, NVME_PAGE);
    }
    return Page;
}

STATIC EFI_STATUS NvmeEnable(IN NVME_CONTROLLER *Controller)
{
    UINT64 Regs = Controller->Regs;
    NVME_QUEUE *Io = &Controller->Queues[NVME_IO_QUEUE_ID];
    NVME_SQE Sqe;
    EFI_STATUS Status;
    UINTN Index;

    MmioWrite64(Regs + EMEX64_NVME_CC, 0);
    Status = NvmeWaitReady(Regs, FALSE);
    if(EFI_ERROR(Status))
    {
        return Status;
    }

    for(Index = 0; Index < 2; Index++)
    {
        Controller->Queues[Index].Sq = NvmePage();
        Controller->Queues[Index].Cq = NvmePage();
        Controller->Queues[Index].Phase = 1;
        if((Controller->Queues[Index].Sq == NULL) || (Controller->Queues[Index].Cq == NULL))
        {
            return EFI_OUT_OF_RESOURCES;
        }
    }

    MmioWrite64(Regs + EMEX64_NVME_INTMS, MAX_UINT64);
    MmioWrite64(Regs + EMEX64_NVME_AQA, (NVME_DEPTH - 1) | ((NVME_DEPTH - 1) << 16));
    MmioWrite64(Regs + EMEX64_NVME_ASQ, (UINT64)(UINTN)Controller->Queues[0].Sq);
    MmioWrite64(Regs + EMEX64_NVME_ACQ, (UINT64)(UINTN)Controller->Queues[0].Cq);
    MmioWrite64(Regs + EMEX64_NVME_CC, BIT0 | (6 << 16) | (4 << 20));
    Status = NvmeWaitReady(Regs, TRUE);
    if(EFI_ERROR(Status))
    {
        return Status;
    }

    ZeroMem(&Sqe, sizeof(Sqe));
    Sqe.Opcode = NVME_ADMIN_CRIOCQ_CMD;
    Sqe.Prp1 = (UINT64)(UINTN)Io->Cq;
    Sqe.Cdw10 = NVME_IO_QUEUE_ID | ((NVME_DEPTH - 1) << 16);
    Sqe.Cdw11 = BIT0;
    Status = NvmeCommand(Controller, 0, &Sqe, NULL, 0, NULL);
    if(EFI_ERROR(Status))
    {
        return Status;
    }

    ZeroMem(&Sqe, sizeof(Sqe));
    Sqe.Opcode = NVME_ADMIN_CRIOSQ_CMD;
    Sqe.Prp1 = (UINT64)(UINTN)Io->Sq;
    Sqe.Cdw10 = NVME_IO_QUEUE_ID | ((NVME_DEPTH - 1) << 16);
    Sqe.Cdw11 = BIT0 | (NVME_IO_QUEUE_ID << 16);
    return NvmeCommand(Controller, 0, &Sqe, NULL, 0, NULL);
}

STATIC VOID NvmeAddDisk(IN NVME_CONTROLLER *Controller,
                        IN UINT32 Nsid,
                        IN NVME_ADMIN_NAMESPACE_DATA *Data)
{
    NVME_DISK *Disk;
    EFI_HANDLE Handle = NULL;
    EFI_STATUS Status;
    UINT32 LbaShift;

    if(Data->Nsze == 0)
    {
        return;
    }
    LbaShift = Data->LbaFormat[Data->Flbas & 0xF].Lbads;
    if((LbaShift < 9) || (LbaShift > 12))
    {
        DEBUG((DEBUG_ERROR, "NvmeDxe: namespace %u: blocks of 2^%u bytes are not supported\n", Nsid, LbaShift));
        return;
    }

    Disk = AllocateZeroPool(sizeof(*Disk));
    if(Disk == NULL)
    {
        return;
    }
    Disk->Signature = NVME_DISK_SIGNATURE;
    Disk->Controller = Controller;
    Disk->Nsid = Nsid;

    Disk->Media.MediaId = 1;
    Disk->Media.RemovableMedia = FALSE;
    Disk->Media.MediaPresent = TRUE;
    Disk->Media.LogicalPartition = FALSE;
    Disk->Media.ReadOnly = (((UINT8 *)Data)[NVME_NS_NSATTR] & BIT0) != 0;
    Disk->Media.WriteCaching = FALSE;
    Disk->Media.BlockSize = 1U << LbaShift;
    Disk->Media.IoAlign = 4;
    Disk->Media.LastBlock = Data->Nsze - 1;
    Disk->Media.LogicalBlocksPerPhysicalBlock = 1;

    Disk->BlockIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION2;
    Disk->BlockIo.Media = &Disk->Media;
    Disk->BlockIo.Reset = NvmeReset;
    Disk->BlockIo.ReadBlocks = NvmeReadBlocks;
    Disk->BlockIo.WriteBlocks = NvmeWriteBlocks;
    Disk->BlockIo.FlushBlocks = NvmeFlushBlocks;

    CopyMem(&Disk->DevicePath.Controller, &Controller->DevicePath.Controller, sizeof(Disk->DevicePath.Controller));
    NvmeNamespaceNode(Controller, Nsid, &Disk->DevicePath.Namespace);
    SetDevicePathEndNode(&Disk->DevicePath.End);

    Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiDevicePathProtocolGuid, &Disk->DevicePath, &gEfiBlockIoProtocolGuid, &Disk->BlockIo, NULL);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "NvmeDxe: namespace %u: %Lu MiB%a\n", Nsid, RShiftU64(MultU64x32(Data->Nsze, Disk->Media.BlockSize), 20), Disk->Media.ReadOnly ? ", read only" : ""));
}

STATIC VOID EFIAPI NvmeExitBootServices(IN EFI_EVENT Event,
                                        IN VOID *Context)
{
    NVME_CONTROLLER *Controller = Context;
    MmioWrite64(Controller->Regs + EMEX64_NVME_CC, 0);
}

EFI_STATUS EFIAPI NvmeDxeInitialize(IN EFI_HANDLE ImageHandle,
                                    IN EFI_SYSTEM_TABLE *SystemTable)
{
    NVME_CONTROLLER *Controller;
    NVME_ADMIN_CONTROLLER_DATA *Identify;
    NVME_ADMIN_NAMESPACE_DATA *Namespace;
    NVME_SQE Sqe;
    EFI_STATUS Status;
    EFI_EVENT Event;
    UINT64 Cfg;
    UINT32 Nsid;

    Cfg = Emex64PcieFind(EMEX64_NVME_DEVICE_ID);
    if(Cfg == 0)
    {
        DEBUG((DEBUG_INFO, "NvmeDxe: no NAND storage controller\n"));
        return EFI_UNSUPPORTED;
    }

    Controller = AllocateZeroPool(sizeof(*Controller));
    Identify = NvmePage();
    if((Controller == NULL) || (Identify == NULL))
    {
        return EFI_OUT_OF_RESOURCES;
    }
    Controller->Signature = NVME_CONTROLLER_SIGNATURE;
    Controller->Regs = Emex64PcieMapBar0(Cfg);
    Controller->Prps = NvmePage();
    if((Controller->Regs == 0) || (Controller->Prps == NULL))
    {
        return EFI_DEVICE_ERROR;
    }

    Status = NvmeEnable(Controller);
    if(EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "NvmeDxe: the controller could not be set up: %r\n", Status));
        return Status;
    }

    ZeroMem(&Sqe, sizeof(Sqe));
    Sqe.Opcode = NVME_ADMIN_IDENTIFY_CMD;
    Sqe.Cdw10 = 1;
    Status = NvmeCommand(Controller, 0, &Sqe, Identify, NVME_PAGE, NULL);
    if(EFI_ERROR(Status))
    {
        return Status;
    }
    Controller->Namespaces = MIN(Identify->Nn, NVME_NAMESPACES);
    DEBUG((DEBUG_INFO, "NvmeDxe: %.40a, %u namespace(s)\n", Identify->Mn, Controller->Namespaces));

    Controller->PassThruMode.Attributes = EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_PHYSICAL | EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_LOGICAL | EFI_NVM_EXPRESS_PASS_THRU_ATTRIBUTES_CMD_SET_NVM;
    Controller->PassThruMode.IoAlign = 4;
    Controller->PassThruMode.NvmeVersion = (UINT32)MmioRead64(Controller->Regs + EMEX64_NVME_VS);
    Controller->PassThru.Mode = &Controller->PassThruMode;
    Controller->PassThru.PassThru = NvmePassThru;
    Controller->PassThru.GetNextNamespace = NvmeGetNextNamespace;
    Controller->PassThru.BuildDevicePath = NvmeBuildDevicePath;
    Controller->PassThru.GetNamespace = NvmeGetNamespace;

    Controller->DevicePath.Controller.Header.Type = HARDWARE_DEVICE_PATH;
    Controller->DevicePath.Controller.Header.SubType = HW_VENDOR_DP;
    SetDevicePathNodeLength(&Controller->DevicePath.Controller.Header, sizeof(VENDOR_DEVICE_PATH));
    CopyGuid(&Controller->DevicePath.Controller.Guid, &gEmex64NvmeDevicePathGuid);
    SetDevicePathEndNode(&Controller->DevicePath.End);

    Namespace = (NVME_ADMIN_NAMESPACE_DATA *)Identify;
    for(Nsid = 1; Nsid <= Controller->Namespaces; Nsid++)
    {
        ZeroMem(&Sqe, sizeof(Sqe));
        Sqe.Opcode = NVME_ADMIN_IDENTIFY_CMD;
        Sqe.Nsid = Nsid;
        if(!EFI_ERROR(NvmeCommand(Controller, 0, &Sqe, Namespace, NVME_PAGE, NULL)))
        {
            CopyMem(&Controller->Eui[Nsid - 1], (UINT8 *)Namespace + NVME_NS_NGUID, sizeof(UINT64));
        }
    }

    Status = gBS->InstallMultipleProtocolInterfaces(&Controller->Handle, &gEfiDevicePathProtocolGuid, &Controller->DevicePath, &gEfiNvmExpressPassThruProtocolGuid, &Controller->PassThru, NULL);
    if(EFI_ERROR(Status))
    {
        return Status;
    }

    for(Nsid = 1; Nsid <= Controller->Namespaces; Nsid++)
    {
        ZeroMem(&Sqe, sizeof(Sqe));
        Sqe.Opcode = NVME_ADMIN_IDENTIFY_CMD;
        Sqe.Nsid = Nsid;
        if(!EFI_ERROR(NvmeCommand(Controller, 0, &Sqe, Namespace, NVME_PAGE, NULL)))
        {
            NvmeAddDisk(Controller, Nsid, Namespace);
        }
    }
    FreePages(Identify, 1);

    gBS->CreateEvent(EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, NvmeExitBootServices, Controller, &Event);
    return EFI_SUCCESS;
}
