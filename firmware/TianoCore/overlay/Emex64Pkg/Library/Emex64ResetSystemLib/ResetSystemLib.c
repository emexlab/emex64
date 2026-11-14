/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/ResetSystemLib.h>
#include <Emex64Platform.h>

VOID EFIAPI ResetShutdown(VOID)
{
    MmioWrite64(EMEX64_PLATFORM_POWER, 0);
    CpuDeadLoop();
}

VOID EFIAPI ResetCold(VOID)
{
    MmioWrite64(EMEX64_PLATFORM_POWER, 1);
    ResetShutdown();
}

VOID EFIAPI ResetWarm(VOID)
{
    ResetCold();
}

VOID EFIAPI ResetPlatformSpecific(IN UINTN DataSize,
                                  IN VOID *ResetData)
{
    ResetCold();
}

VOID EFIAPI ResetSystem(IN EFI_RESET_TYPE ResetType,
                        IN EFI_STATUS ResetStatus,
                        IN UINTN DataSize,
                        IN VOID *ResetData OPTIONAL)
{
    switch(ResetType)
    {
        case EfiResetShutdown:
            ResetShutdown();
            break;
        default:
            ResetCold();
            break;
    }
}
