/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/RealTimeClockLib.h>
#include <Emex64Platform.h>

EFI_STATUS EFIAPI LibGetTime(OUT EFI_TIME *Time,
                             OUT EFI_TIME_CAPABILITIES *Capabilities)
{
    UINT64 Seconds;

    if(Time == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    do {
        Seconds = MmioRead64(EMEX64_RTC_SECONDS);
        Time->Minute = (UINT8)MmioRead64(EMEX64_RTC_MINUTES);
        Time->Hour = (UINT8)MmioRead64(EMEX64_RTC_HOURS);
        Time->Day = (UINT8)MmioRead64(EMEX64_RTC_DAY);
        Time->Month = (UINT8)MmioRead64(EMEX64_RTC_MONTH);
        Time->Year = (UINT16)(2000 + MmioRead64(EMEX64_RTC_YEAR));
    } while(Seconds != MmioRead64 (EMEX64_RTC_SECONDS));

    Time->Second = (UINT8)Seconds;
    Time->Nanosecond = 0;
    Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    Time->Daylight = 0;
    Time->Pad1 = 0;
    Time->Pad2 = 0;

    if(Capabilities != NULL)
    {
        Capabilities->Resolution = 1;
        Capabilities->Accuracy = 50000000;
        Capabilities->SetsToZero = FALSE;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI LibSetTime(IN EFI_TIME *Time)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI LibGetWakeupTime(OUT BOOLEAN *Enabled,
                                   OUT BOOLEAN *Pending,
                                   OUT EFI_TIME *Time)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI LibSetWakeupTime(IN BOOLEAN Enabled,
                                   OUT EFI_TIME *Time)
{
    return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI LibRtcInitialize(IN EFI_HANDLE ImageHandle,
                                   IN EFI_SYSTEM_TABLE *SystemTable)
{
    return EFI_SUCCESS;
}
