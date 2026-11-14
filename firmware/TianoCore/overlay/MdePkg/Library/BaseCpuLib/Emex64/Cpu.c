/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/CpuLib.h>

VOID EFIAPI CpuSleep(VOID)
{
    __asm__ __volatile__("hlt" ::: "memory");
}

VOID EFIAPI CpuFlushTlb(VOID)
{
    __asm__ __volatile__(".byte 0x8A, 0x00" ::: "memory");
}

VOID EFIAPI InitializeFloatingPointUnits(VOID)
{
    __asm__ __volatile__("mov fpc, 0" ::: "memory");
}
