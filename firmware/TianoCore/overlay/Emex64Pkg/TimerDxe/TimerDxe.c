/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Cpu.h>
#include <Protocol/Timer.h>
#include <Emex64Platform.h>

#define DEFAULT_TIMER_PERIOD    100000

STATIC EFI_TIMER_NOTIFY mTimerNotifyFunction;
STATIC UINT64 mTimerPeriod;
STATIC EFI_CPU_ARCH_PROTOCOL *mCpu;

STATIC VOID EFIAPI TimerInterruptHandler(IN EFI_EXCEPTION_TYPE InterruptType,
                                         IN EFI_SYSTEM_CONTEXT SystemContext)
{
    EFI_TPL OriginalTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);

    MmioWrite64(EMEX64_APIC_TIMER_STATUS, EMEX64_TIMER_STATUS_IRQ);
    if(mTimerNotifyFunction != NULL)
    {
        mTimerNotifyFunction(mTimerPeriod);
    }

    gBS->RestoreTPL(OriginalTpl);
}

STATIC EFI_STATUS EFIAPI TimerDriverRegisterHandler(IN EFI_TIMER_ARCH_PROTOCOL *This,
                                                    IN EFI_TIMER_NOTIFY NotifyFunction)
{
    if((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    if((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL))
    {
        return EFI_ALREADY_STARTED;
    }

    mTimerNotifyFunction = NotifyFunction;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI TimerDriverSetTimerPeriod(IN EFI_TIMER_ARCH_PROTOCOL *This,
                                                   IN UINT64 TimerPeriod)
{
    MmioWrite64(EMEX64_APIC_TIMER_CTRL, 0);
    MmioWrite64(EMEX64_APIC_TIMER_STATUS, EMEX64_TIMER_STATUS_IRQ);

    UINT64 Ticks;
    if(TimerPeriod != 0)
    {
        Ticks = DivU64x32(MultU64x64 (TimerPeriod, MmioRead64 (EMEX64_APIC_TIMER_FREQ)), 10000000);
        if(Ticks == 0)
        {
            Ticks = 1;
        }

        MmioWrite64(EMEX64_APIC_TIMER_COUNT, 0);
        MmioWrite64(EMEX64_APIC_TIMER_COMPARE, Ticks);
        MmioWrite64(EMEX64_APIC_TIMER_CTRL, EMEX64_TIMER_CTRL_ENABLE | EMEX64_TIMER_CTRL_IRQ | EMEX64_TIMER_CTRL_PERIODIC);
    }

    mTimerPeriod = TimerPeriod;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI TimerDriverGetTimerPeriod(IN EFI_TIMER_ARCH_PROTOCOL *This,
                                                   OUT UINT64 *TimerPeriod)
{
    if(TimerPeriod == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    *TimerPeriod = mTimerPeriod;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI TimerDriverGenerateSoftInterrupt(IN EFI_TIMER_ARCH_PROTOCOL *This)
{
    EFI_TPL OriginalTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);
    if(mTimerNotifyFunction != NULL)
    {
        mTimerNotifyFunction(0);
    }

    gBS->RestoreTPL(OriginalTpl);
    return EFI_SUCCESS;
}

STATIC EFI_TIMER_ARCH_PROTOCOL mTimer = {
    TimerDriverRegisterHandler,
    TimerDriverSetTimerPeriod,
    TimerDriverGetTimerPeriod,
    TimerDriverGenerateSoftInterrupt
};

STATIC VOID EFIAPI TimerExitBootServices(IN EFI_EVENT Event,
                                         IN VOID *Context)
{
    MmioWrite64(EMEX64_APIC_TIMER_CTRL, 0);
    MmioWrite64(EMEX64_APIC_TIMER_STATUS, EMEX64_TIMER_STATUS_IRQ);
}

EFI_STATUS EFIAPI TimerDxeInitialize(IN EFI_HANDLE ImageHandle,
                                     IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
    ASSERT_EFI_ERROR(Status);

    MmioWrite64(EMEX64_APIC_TIMER_CTRL, 0);
    MmioWrite64(EMEX64_APIC_TIMER_STATUS, EMEX64_TIMER_STATUS_IRQ);

    Status = mCpu->RegisterInterruptHandler(mCpu, EXCEPT_EMEX64_IRQ_LAPIC_TIMER, TimerInterruptHandler);
    ASSERT_EFI_ERROR(Status);

    TimerDriverSetTimerPeriod(&mTimer, DEFAULT_TIMER_PERIOD);

    EFI_EVENT Event;
    Status = gBS->CreateEvent(EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, TimerExitBootServices, NULL, &Event);
    ASSERT_EFI_ERROR(Status);

    EFI_HANDLE Handle = NULL;
    Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiTimerArchProtocolGuid, &mTimer, NULL);
    ASSERT_EFI_ERROR(Status);
    return Status;
}
