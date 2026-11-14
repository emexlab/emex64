/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "BaseLibInternals.h"

#define CR_CRSTAT   7
#define CRSTAT_IE   BIT0

STATIC UINT64 ReadCrstat(VOID)
{
    UINT64  Value;
    __asm__ __volatile__("cmovb %0, %1" : "=r" (Value) : "i" (CR_CRSTAT));
    return Value;
}

STATIC VOID WriteCrstat(UINT64  Value)
{
    __asm__ __volatile__("cmov %0, %1" : : "i" (CR_CRSTAT), "r" (Value) : "memory");
}

VOID EFIAPI EnableInterrupts(VOID)
{
    WriteCrstat(ReadCrstat() | CRSTAT_IE);
}

VOID EFIAPI DisableInterrupts(VOID)
{
    WriteCrstat(ReadCrstat() & ~(UINT64)CRSTAT_IE);
}

BOOLEAN EFIAPI GetInterruptState(VOID)
{
    return(ReadCrstat() & CRSTAT_IE) != 0;
}

VOID EFIAPI CpuPause(VOID)
{
    __asm__ __volatile__(".byte 0x77" ::: "memory");
}

VOID EFIAPI CpuBreakpoint(VOID)
{
    __asm__ __volatile__(".byte 0xFF" ::: "memory");
}

VOID EFIAPI MemoryFence(VOID)
{
    __asm__ __volatile__("xchg cf, sp" ::: "memory", "cf");
}

VOID EFIAPI SpeculationBarrier(VOID)
{
    MemoryFence();
}

VOID EFIAPI InternalSwitchStack(IN SWITCH_STACK_ENTRY_POINT EntryPoint,
                                IN VOID *Context1 OPTIONAL,
                                IN VOID *Context2 OPTIONAL,
                                IN VOID *NewStack,
                                IN VA_LIST Marker)
{
    UINT64  Top = ((UINT64)(UINTN)NewStack - 16) & ~(UINT64)15;

    register UINT64  Arg0 __asm__("r0") = (UINT64)(UINTN)Context1;
    register UINT64  Arg1 __asm__("r1") = (UINT64)(UINTN)Context2;

    __asm__ __volatile__(
        "mov sp, %2\n\t"
        "mov fp, sp\n\t"
        "bl %3\n"
        "1:\n\t"
        "hlt\n\t"
        "b 1b"
        :
        : "r" (Arg0), "r" (Arg1), "r" (Top), "r" ((UINT64)(UINTN)EntryPoint)
        : "memory");

    UNREACHABLE();
}
