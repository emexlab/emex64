/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Cpu.h>
#include <Emex64Platform.h>

STATIC EFI_STATUS EFIAPI CpuFlushCpuDataCache(IN EFI_CPU_ARCH_PROTOCOL *This,
                                              IN EFI_PHYSICAL_ADDRESS Start,
                                              IN UINT64 Length,
                                              IN EFI_CPU_FLUSH_TYPE FlushType)
{
    /* emex64vm has no caches between the cores and RAM, and devices see RAM as it is (as of now) */
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI CpuEnableInterrupt(IN EFI_CPU_ARCH_PROTOCOL *This)
{
    EnableInterrupts();
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI CpuDisableInterrupt(IN EFI_CPU_ARCH_PROTOCOL *This)
{
    DisableInterrupts();
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI CpuGetInterruptState(IN EFI_CPU_ARCH_PROTOCOL *This,
                                              OUT BOOLEAN *State)
{
    if(State == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    *State = GetInterruptState ();
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI CpuInit(IN EFI_CPU_ARCH_PROTOCOL *This,
                                 IN EFI_CPU_INIT_TYPE InitType)
{
    return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS EFIAPI CpuRegisterInterruptHandler(IN EFI_CPU_ARCH_PROTOCOL *This,
                                                     IN EFI_EXCEPTION_TYPE InterruptType,
                                                     IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler)
{
    return RegisterCpuInterruptHandler(InterruptType, InterruptHandler);
}

STATIC EFI_STATUS EFIAPI CpuGetTimerValue(IN EFI_CPU_ARCH_PROTOCOL  *This,
                                          IN  UINT32 TimerIndex,
                                          OUT UINT64 *TimerValue,
                                          OUT UINT64 *TimerPeriod OPTIONAL)
{
    if(TimerValue == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    if(TimerIndex != 0)
    {
        return EFI_INVALID_PARAMETER;
    }

    *TimerValue = GetPerformanceCounter();
    if(TimerPeriod != NULL)
    {
        *TimerPeriod = DivU64x64Remainder(1000000000000000ULL, GetPerformanceCounterProperties (NULL, NULL), NULL);
    }

    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI CpuSetMemoryAttributes(IN EFI_CPU_ARCH_PROTOCOL  *This,
                                                IN EFI_PHYSICAL_ADDRESS BaseAddress,
                                                IN UINT64 Length,
                                                IN UINT64 Attributes)
{
    /* paging is off in firmware */
    return EFI_SUCCESS;
}

STATIC EFI_CPU_ARCH_PROTOCOL mCpu = {
    CpuFlushCpuDataCache,
    CpuEnableInterrupt,
    CpuDisableInterrupt,
    CpuGetInterruptState,
    CpuInit,
    CpuRegisterInterruptHandler,
    CpuGetTimerValue,
    CpuSetMemoryAttributes,
    1,                          /* NumberOfTimers */
    8                           /* DmaBufferAlignment */
};

EFI_STATUS EFIAPI CpuDxeInitialize(IN EFI_HANDLE ImageHandle,
                                   IN EFI_SYSTEM_TABLE  *SystemTable)
{
    EFI_STATUS Status;
    EFI_HANDLE Handle;

    DisableInterrupts();
    Status = InitializeCpuExceptionHandlers (NULL);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "emex64: %Lu core(s), CPUID model 0x%Lx, ISA %Lu\n", (MmioRead64(EMEX64_APIC_VERSION) >> 8) & 0xFF, MmioRead64(EMEX64_CPUID_MODEL), MmioRead64(EMEX64_CPUID_ISA)));

    Handle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiCpuArchProtocolGuid, &mCpu, NULL);
    ASSERT_EFI_ERROR(Status);
    return Status;
}
