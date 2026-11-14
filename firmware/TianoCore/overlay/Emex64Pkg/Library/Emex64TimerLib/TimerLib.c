/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Emex64Platform.h>

STATIC UINT64 ReadCount(VOID)
{
    if((MmioRead64(EMEX64_TIMER_CTRL) & EMEX64_TIMER_CTRL_ENABLE) == 0)
    {
        MmioWrite64(EMEX64_TIMER_COMPARE, MAX_UINT64);
        MmioWrite64(EMEX64_TIMER_CTRL, EMEX64_TIMER_CTRL_ENABLE);
    }
    return MmioRead64(EMEX64_TIMER_COUNT);
}

STATIC UINT64 Frequency(VOID)
{
    UINT64 Freq;
    Freq = MmioRead64(EMEX64_TIMER_FREQ);
    return (Freq != 0) ? Freq : 1000000000ULL;
}

STATIC VOID WaitTicks(IN UINT64 Ticks)
{
    UINT64 Start = ReadCount();
    while(ReadCount() - Start < Ticks)
    {
        CpuPause();
    }
}

UINTN EFIAPI MicroSecondDelay(IN UINTN MicroSeconds)
{
    WaitTicks(DivU64x32(MultU64x64(MicroSeconds, Frequency()), 1000000));
    return MicroSeconds;
}

UINTN EFIAPI NanoSecondDelay(IN UINTN NanoSeconds)
{
    WaitTicks(DivU64x32(MultU64x64(NanoSeconds, Frequency()), 1000000000));
    return NanoSeconds;
}

UINT64 EFIAPI GetPerformanceCounter(VOID)
{
    return ReadCount();
}

UINT64 EFIAPI GetPerformanceCounterProperties(OUT UINT64 *StartValue OPTIONAL,
                                              OUT UINT64 *EndValue OPTIONAL)
{
    if(StartValue != NULL)
    {
        *StartValue = 0;
    }

    if(EndValue != NULL)
    {
        *EndValue = MAX_UINT64;
    }

    return Frequency ();
}

UINT64 EFIAPI GetTimeInNanoSecond(IN UINT64 Ticks)
{
  UINT64 Freq;
  UINT64 Remainder;
  UINT64 Seconds;
  Freq = Frequency();
  Seconds = DivU64x64Remainder(Ticks, Freq, &Remainder);
  return MultU64x32(Seconds, 1000000000) + DivU64x64Remainder(MultU64x32(Remainder, 1000000000), Freq, NULL);
}
