/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>
#include <Emex64Platform.h>

#define SEC_STACK_SIZE      SIZE_128KB
#define UEFI_REGION_SIZE    SIZE_16MB

#define EMEX64_ADDRESS_BITS 54

VOID EFIAPI ProcessLibraryConstructorList(VOID);
VOID EFIAPI SecStartup(VOID);

__attribute__((naked)) VOID EFIAPI _ModuleEntryPoint(VOID)
{
    __asm__ __volatile__(
        "ldq sp, %0\n\t"
        "sub sp, sp, 16\n\t"
        "bl SecStartup\n"
        "1:\n\t"
        "hlt\n\t"
        "b 1b"
        :
        : "i" (EMEX64_MC_SIZE));
}

STATIC VOID SecPrint(IN CONST CHAR8  *Format, ...)
{
    CHAR8 Buffer[128];
    VA_LIST Marker;
    UINTN Length;

    VA_START(Marker, Format);
    Length = AsciiVSPrint(Buffer, sizeof (Buffer), Format, Marker);
    VA_END(Marker);
    SerialPortWrite((UINT8 *)Buffer, Length);
}

VOID EFIAPI SecStartup(VOID)
{
    EFI_STATUS Status;

    UINT64 MemTop = MmioRead64(EMEX64_MC_SIZE) & ~(UINT64)EFI_PAGE_MASK;
    UINT64 FdBase = FixedPcdGet64(PcdEmex64FdBase);
    UINT64 FdSize = FixedPcdGet64(PcdEmex64FdSize);
    UINT64 FvBase = FixedPcdGet64(PcdEmex64FvBase);
    UINT64 FvSize = FixedPcdGet64(PcdEmex64FvSize);
    UINT64 UefiBase = MemTop - UEFI_REGION_SIZE;
    UINT64 StackBase = MemTop - SEC_STACK_SIZE;

    SerialPortInitialize();

    if(MemTop < FdBase + FdSize + UEFI_REGION_SIZE + SIZE_16MB)
    {
        SecPrint("SEC: not enough RAM, emex64 TianoCore needs %Lu MiB at least\r\n", (FdBase + FdSize + UEFI_REGION_SIZE + SIZE_16MB) >> 20);
        MmioWrite64(EMEX64_PLATFORM_POWER, 0);
        CpuDeadLoop();
    }

    InitializeCpuExceptionHandlers(NULL);

    EFI_HOB_HANDOFF_INFO_TABLE *HobList = HobConstructor((VOID *)(UINTN)UefiBase, UEFI_REGION_SIZE, (VOID *)(UINTN)UefiBase, (VOID *)(UINTN)StackBase);
    PrePeiSetHobList(HobList);
    ProcessLibraryConstructorList();

    BuildResourceDescriptorHob(EFI_RESOURCE_SYSTEM_MEMORY, EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE | EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE | EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE, 0, MemTop);
    BuildMemoryAllocationHob(FdBase, FdSize, EfiBootServicesData);
    BuildFvHob(FvBase, FvSize);
    BuildStackHob(StackBase, SEC_STACK_SIZE);
    BuildCpuHob(EMEX64_ADDRESS_BITS, 16);
    SetBootMode(BOOT_WITH_FULL_CONFIGURATION);

    GetPerformanceCounter();

    Status = LoadDxeCoreFromFv(NULL, SIZE_128KB);
    SecPrint("SEC: cannot load the DXE core (%r)\r\n", Status);
    CpuDeadLoop();
}
