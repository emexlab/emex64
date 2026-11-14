/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>
#include <Emex64Platform.h>

STATIC EFI_SMBIOS_PROTOCOL *mSmbios;

STATIC EFI_STATUS AddRecord(IN VOID *Record,
                            IN CONST CHAR8 **Strings,
                            IN UINTN StringCount,
                            OUT EFI_SMBIOS_HANDLE *NewHandle OPTIONAL)
{
    EFI_SMBIOS_TABLE_HEADER *Header = Record;
    UINTN Size = Header->Length + 1;
    for(UINTN Index = 0; Index < StringCount; Index++)
    {
        Size += AsciiStrLen(Strings[Index]) + 1;
    }

    if(StringCount == 0)
    {
        Size++;
    }

    UINT8 *Buffer = AllocateZeroPool(Size);
    if(Buffer == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    CopyMem(Buffer, Record, Header->Length);
    UINTN Offset = Header->Length;
    for(UINTN Index = 0; Index < StringCount; Index++)
    {
        CopyMem(Buffer + Offset, Strings[Index], AsciiStrLen(Strings[Index]) + 1);
        Offset += AsciiStrLen(Strings[Index]) + 1;
    }

    EFI_SMBIOS_HANDLE Handle = SMBIOS_HANDLE_PI_RESERVED;
    EFI_STATUS Status = mSmbios->Add (mSmbios, NULL, &Handle, (EFI_SMBIOS_TABLE_HEADER *)Buffer);
    FreePool(Buffer);
    if(EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "SmbiosPlatformDxe: type %u not added: %r\n", Header->Type, Status));
    }
    else if(NewHandle != NULL)
    {
        *NewHandle = Handle;
    }

    return Status;
}

STATIC VOID ReadCpuidString(IN UINT64 Address,
                            IN UINTN  Size,
                            OUT CHAR8 *String)
{
    UINT64 Bytes = 0;
    for(UINTN Index = 0; Index < Size; Index++)
    {
        if((Index % 8) == 0)
        {
            Bytes = MmioRead64(Address + Index);
        }
        String[Index] = (CHAR8)(Bytes >> (8 * (Index % 8)));
    }
    String[Size] = '\0';
}

STATIC UINT16 MeasureSpeedMhz(VOID)
{
    /* a emulated core has no real clock speed */
    UINT64 Iterations = 1000000;
    EFI_TPL OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);
    UINT64 Start = GetPerformanceCounter();
    __asm__ __volatile__(
        "1:\n\t"
        "sub %0, %0, 1\n\t"
        "cmp %0, 0\n\t"
        "bne 1b"
        : "+r" (Iterations)
        :
        : "cf");
    UINT64 Ns = GetTimeInNanoSecond(GetPerformanceCounter() - Start);
    gBS->RestoreTPL(OldTpl);
    if(Ns == 0)
    {
        return 0;
    }

    return (UINT16)MIN(DivU64x64Remainder(3000000ULL * 1000, Ns, NULL), MAX_UINT16);
}

STATIC VOID AddFirmware(VOID)
{
    SMBIOS_TABLE_TYPE0 Type0;
    CHAR8 Version[64];
    CHAR8 Date[16];
    CONST CHAR8 *Strings[3];
    CONST CHAR8 *Months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    CONST CHAR8 *Built  = __DATE__;
    UINTN Month;
    
    for(Month = 0; Month < 12; Month++)
    {
        if(CompareMem(Months + 3 * Month, Built, 3) == 0)
        {
            break;
        }
    }

    AsciiSPrint(Date, sizeof(Date), "%02u/%c%c/%c%c%c%c", (UINT32)Month + 1, (Built[4] == ' ') ? '0' : Built[4], Built[5], Built[7], Built[8], Built[9], Built[10]);
    AsciiSPrint(Version, sizeof(Version), "%s", (CHAR16 *)PcdGetPtr(PcdFirmwareVersionString));
    
    ZeroMem(&Type0, sizeof(Type0));
    Type0.Hdr.Type                                       = SMBIOS_TYPE_BIOS_INFORMATION;
    Type0.Hdr.Length                                     = sizeof(Type0);
    Type0.Vendor                                         = 1;
    Type0.BiosVersion                                    = 2;
    Type0.BiosReleaseDate                                = 3;
    Type0.BiosSegment                                    = 0xE800;
    Type0.BiosSize                                       = (UINT8)((FixedPcdGet64(PcdEmex64FdSize) / SIZE_64KB) - 1);
    Type0.BiosCharacteristics.BiosCharacteristicsNotSupported = 0;
    Type0.BiosCharacteristics.BiosIsUpgradable           = 1;
    Type0.BiosCharacteristics.SelectableBootIsSupported  = 1;
    Type0.BIOSCharacteristicsExtensionBytes[1]           = BIT3 | BIT4;
    Type0.SystemBiosMajorRelease                         = 0;
    Type0.SystemBiosMinorRelease                         = 1;
    Type0.EmbeddedControllerFirmwareMajorRelease         = 0xFF;
    Type0.EmbeddedControllerFirmwareMinorRelease         = 0xFF;
    Type0.ExtendedBiosSize.Size                          = (UINT16)(FixedPcdGet64(PcdEmex64FdSize) >> 20);

    Strings[0] = "emexLabs";
    Strings[1] = Version;
    Strings[2] = Date;
    AddRecord(&Type0, Strings, 3, NULL);
}

STATIC VOID AddSystem(VOID)
{
    SMBIOS_TABLE_TYPE1 Type1;
    SMBIOS_TABLE_TYPE3 Type3;
    CONST CHAR8 *Strings[5];

    ZeroMem(&Type1, sizeof (Type1));
    Type1.Hdr.Type      = SMBIOS_TYPE_SYSTEM_INFORMATION;
    Type1.Hdr.Length    = sizeof (Type1);
    Type1.Manufacturer  = 1;
    Type1.ProductName   = 2;
    Type1.Version       = 3;
    Type1.SerialNumber  = 0;
    Type1.WakeUpType    = SystemWakeupTypePowerSwitch;
    Type1.SKUNumber     = 0;
    Type1.Family        = 4;

    Strings[0] = "emexLabs";
    Strings[1] = "emex64vm";
    Strings[2] = "1.0";
    Strings[3] = "emex64";
    AddRecord(&Type1, Strings, 4, NULL);

    ZeroMem(&Type3, sizeof(Type3));
    Type3.Hdr.Type              = SMBIOS_TYPE_SYSTEM_ENCLOSURE;
    Type3.Hdr.Length            = OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements);
    Type3.Manufacturer          = 1;
    Type3.Type                  = MiscChassisTypeOther;
    Type3.BootupState           = ChassisStateSafe;
    Type3.PowerSupplyState      = ChassisStateSafe;
    Type3.ThermalState          = ChassisStateSafe;
    Type3.SecurityStatus        = ChassisSecurityStatusNone;
    Type3.NumberofPowerCords    = 1;

    Strings[0] = "emexLabs";
    AddRecord(&Type3, Strings, 1, NULL);
}

STATIC VOID AddProcessor(VOID)
{
    SMBIOS_TABLE_TYPE4 Type4;
    CHAR8 Vendor[9];
    CHAR8 Brand[65];
    CHAR8 Version[96];
    CONST CHAR8 *Strings[3];
    UINT8 Cores;
    UINT16 Mhz;
    UINT64 Isa;

    ReadCpuidString(EMEX64_CPUID_VENDOR, 8, Vendor);
    ReadCpuidString(EMEX64_CPUID_BRAND, 64, Brand);
    Isa = MmioRead64(EMEX64_CPUID_ISA);
    Cores = (UINT8)(MmioRead64(EMEX64_APIC_VERSION) >> 8);
    Mhz = MeasureSpeedMhz();

    AsciiSPrint(Version, sizeof(Version), "%a (LA64 v%Lu)", Brand, Isa & 0xFFFF);

    ZeroMem(&Type4, sizeof(Type4));
    Type4.Hdr.Type                  = SMBIOS_TYPE_PROCESSOR_INFORMATION;
    Type4.Hdr.Length                = sizeof (Type4);
    Type4.Socket                    = 1;
    Type4.ProcessorType             = CentralProcessor;
    Type4.ProcessorFamily           = ProcessorFamilyOther;
    Type4.ProcessorManufacturer     = 2;
    *(UINT64 *)&Type4.ProcessorId   = MmioRead64(EMEX64_CPUID_MODEL) | LShiftU64(Isa & 0xFFFF, 32);
    Type4.ProcessorVersion          = 3;
    Type4.MaxSpeed                  = Mhz;
    Type4.CurrentSpeed              = Mhz;
    Type4.Status                    = 0x41;
    Type4.ProcessorUpgrade          = ProcessorUpgradeNone;
    Type4.L1CacheHandle             = 0xFFFF;   /* this is a vm x3 */
    Type4.L2CacheHandle             = 0xFFFF;
    Type4.L3CacheHandle             = 0xFFFF;
    Type4.CoreCount                 = Cores;
    Type4.EnabledCoreCount          = Cores;
    Type4.ThreadCount               = Cores;
    Type4.ProcessorCharacteristics  = BIT2 | BIT3;
    Type4.ProcessorFamily2          = ProcessorFamilyOther;
    Type4.CoreCount2                = Cores;
    Type4.EnabledCoreCount2         = Cores;
    Type4.ThreadCount2              = Cores;

    Strings[0] = "CPU 0";
    Strings[1] = Vendor;
    Strings[2] = Version;
    AddRecord(&Type4, Strings, 3, NULL);

    DEBUG((DEBUG_INFO, "SmbiosPlatformDxe: %a, %u core(s), about %u million instructions a second\n", Version, Cores, Mhz));
}

STATIC VOID AddMemory(VOID)
{
    SMBIOS_TABLE_TYPE16 Type16;
    SMBIOS_TABLE_TYPE17 Type17;
    SMBIOS_TABLE_TYPE19 Type19;
    SMBIOS_TABLE_TYPE32 Type32;
    CONST CHAR8 *Strings[2];
    EFI_SMBIOS_HANDLE ArrayHandle;
    UINT64 Size;
    UINT64 Kb;
    UINT64 Mb;

    Size = MmioRead64(EMEX64_MC_SIZE);
    Kb = Size >> 10;
    Mb = Size >> 20;

    ZeroMem(&Type16, sizeof(Type16));
    Type16.Hdr.Type = SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
    Type16.Hdr.Length = sizeof(Type16);
    Type16.Location = MemoryArrayLocationSystemBoard;
    Type16.Use = MemoryArrayUseSystemMemory;
    Type16.MemoryErrorCorrection = MemoryErrorCorrectionNone;
    Type16.MaximumCapacity = (Kb < 0x80000000ULL) ? (UINT32)Kb : 0x80000000;
    Type16.ExtendedMaximumCapacity = Size;
    Type16.MemoryErrorInformationHandle = 0xFFFE;
    Type16.NumberOfMemoryDevices = 1;
    ArrayHandle = 0xFFFE;
    AddRecord(&Type16, NULL, 0, &ArrayHandle);

    ZeroMem(&Type17, sizeof(Type17));
    Type17.Hdr.Type                     = SMBIOS_TYPE_MEMORY_DEVICE;
    Type17.Hdr.Length                   = sizeof(Type17);
    Type17.MemoryArrayHandle            = ArrayHandle;
    Type17.MemoryErrorInformationHandle = 0xFFFE;
    Type17.TotalWidth                   = 64;
    Type17.DataWidth                    = 64;
    Type17.Size                         = (Mb < 0x7FFF) ? (UINT16)Mb : 0x7FFF;
    Type17.ExtendedSize                 = (UINT32)Mb;
    Type17.FormFactor                   = MemoryFormFactorOther;
    Type17.DeviceLocator                = 1;
    Type17.MemoryType                   = MemoryTypeOther;
    Type17.Manufacturer                 = 2;
    Strings[0] = "RAM";
    Strings[1] = "emexLabs";
    AddRecord(&Type17, Strings, 2, NULL);

    ZeroMem(&Type19, sizeof(Type19));
    Type19.Hdr.Type             = SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
    Type19.Hdr.Length           = sizeof (Type19);
    Type19.MemoryArrayHandle    = ArrayHandle;
    Type19.PartitionWidth       = 1;
    if(Kb < 0xFFFFFFFFULL)
    {
        Type19.StartingAddress = 0;
        Type19.EndingAddress = (UINT32)(Kb - 1);
    }
    else
    {
        Type19.StartingAddress          = 0xFFFFFFFF;
        Type19.EndingAddress            = 0xFFFFFFFF;
        Type19.ExtendedStartingAddress  = 0;
        Type19.ExtendedEndingAddress    = Size - 1;
    }

    AddRecord(&Type19, NULL, 0, NULL);

    ZeroMem(&Type32, sizeof (Type32));
    Type32.Hdr.Type = SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION;
    Type32.Hdr.Length = sizeof (Type32);
    Type32.BootStatus = BootInformationStatusNoError;
    AddRecord(&Type32, NULL, 0, NULL);
}

EFI_STATUS EFIAPI SmbiosPlatformDxeInitialize(IN EFI_HANDLE ImageHandle,
                                              IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&mSmbios);
    if(EFI_ERROR (Status))
    {
        return Status;
    }

    AddFirmware();
    AddSystem();
    AddProcessor();
    AddMemory();
    return EFI_SUCCESS;
}
