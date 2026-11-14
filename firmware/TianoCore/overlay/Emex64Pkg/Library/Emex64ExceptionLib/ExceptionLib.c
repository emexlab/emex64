/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Emex64Platform.h>

VOID Emex64TrapEntry(VOID);

STATIC EFI_CPU_INTERRUPT_HANDLER  mExceptionHandlers[EXCEPT_EMEX64_MAX_EXCEPTIONS];
STATIC EFI_CPU_INTERRUPT_HANDLER  mIrqHandlers[EXCEPT_EMEX64_MAX_IRQS];
STATIC BOOLEAN                    mDumping;

STATIC CONST CHAR8 *mExceptionNames[EXCEPT_EMEX64_MAX_EXCEPTIONS] = {
    "none",
    "bad access",
    "permission",
    "bad instruction",
    "bad arithmetic",
    "page fault",
    "KTRR violation",
    "svc",
    "PAC"
};

STATIC VOID TrapPrint(IN CONST CHAR8 *Format, ...)
{
    CHAR8 Buffer[160];
    VA_LIST Marker;
    UINTN Length;

    VA_START(Marker, Format);
    Length = AsciiVSPrint(Buffer, sizeof (Buffer), Format, Marker);
    VA_END(Marker);
    SerialPortWrite((UINT8 *)Buffer, Length);
}

STATIC UINT64 ReadCr(IN UINTN Cr)
{
    UINT64 Value;
    switch(Cr)
    {
        case EMEX64_CR_CRSTAT:
            __asm__ __volatile__("cmovb %0, 7" : "=r" (Value));
            break;
        default:
            Value = 0;
            break;
    }
    return Value;
}

VOID EFIAPI DumpCpuContext(IN EFI_EXCEPTION_TYPE ExceptionType,
                           IN EFI_SYSTEM_CONTEXT SystemContext)
{
    EFI_SYSTEM_CONTEXT_EMEX64 *Ctx;
    UINT64 *Regs;
    UINT64 *Frame;
    UINTN Index;
    UINT64 Access;

    STATIC CONST CHAR8  *Names[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "rr",
        "er0", "er1", "er2", "er3", "er4", "er5", "er6", "er7", "er8", "er9",
        "er10", "er11", "er12", "er13", "er14", "er15", "sp", "fp", "cf", "fpc",
        "crepc", "crstat", "crcause", "crtval"
    };

    Ctx = SystemContext.SystemContextEmex64;
    Regs = (UINT64 *)Ctx;

    if(((UINT64)ExceptionType & BIT63) != 0)
    {
        TrapPrint("\n!!!! emex64 interrupt %Lu with no handler !!!!\n", (UINT64)ExceptionType & ~BIT63);
    }
    else
    {
        Access = (Ctx->CRCAUSE >> 8) & 0xF;
        TrapPrint("\n!!!! emex64 exception %Lu (%a)%a !!!!\n", (UINT64)ExceptionType, (ExceptionType < EXCEPT_EMEX64_MAX_EXCEPTIONS) ? mExceptionNames[ExceptionType] : "unknown", (Access == 1) ? ", reading" : (Access == 2) ? ", writing" : (Access == 3) ? ", fetching" : "");
    }

    for(Index = 0; Index < ARRAY_SIZE (Names); Index++)
    {
        TrapPrint("%8a = 0x%016Lx%a", Names[Index], Regs[Index], ((Index % 3) == 2) ? "\n" : "  ");
    }

    TrapPrint("\n");

    Frame = (UINT64 *)(UINTN)Ctx->FP;
    for(Index = 0; Index < 16 && Frame != NULL && ((UINTN)Frame & 7) == 0 && (UINT64)(UINTN)Frame < EMEX64_MMIO_BASE; Index++)
    {
        TrapPrint("  called from 0x%016Lx\n", Frame[2]);
        if(Frame[1] <= (UINT64)(UINTN)Frame)
        {
            break;
        }
        Frame = (UINT64 *)(UINTN)Frame[1];
    }
}

VOID EFIAPI Emex64TrapDispatch(IN UINT64 Cause,
                               IN EFI_SYSTEM_CONTEXT_EMEX64 *Context)
{
    EFI_SYSTEM_CONTEXT SystemContext;
    EFI_EXCEPTION_TYPE Type;
    UINTN Index;

    SystemContext.SystemContextEmex64 = Context;

    if((Cause & BIT63) != 0)
    {
        Index = (UINTN)(Cause & 0xFF);
        Type = (EFI_EXCEPTION_TYPE)EXCEPT_EMEX64_IRQ(Index);
        if((Index < EXCEPT_EMEX64_MAX_IRQS) && (mIrqHandlers[Index] != NULL))
        {
            mIrqHandlers[Index](Type, SystemContext);
            return;
        }
    }
    else
    {
        Index = (UINTN)(Cause & 0xFF);
        Type = (EFI_EXCEPTION_TYPE)Index;
        if((Index < EXCEPT_EMEX64_MAX_EXCEPTIONS) && (mExceptionHandlers[Index] != NULL))
        {
            mExceptionHandlers[Index](Type, SystemContext);
            return;
        }
    }

    if(mDumping)
    {
        TrapPrint("\n!!!! trap while dumping a trap !!!!\n");
        CpuDeadLoop();
    }

    mDumping = TRUE;
    DumpCpuContext(Type, SystemContext);
    CpuDeadLoop();
}

EFI_STATUS EFIAPI InitializeCpuExceptionHandlers(IN EFI_VECTOR_HANDOFF_INFO *VectorInfo OPTIONAL)
{
    UINT64 Vector;
    UINT64 Stat;

    Vector = (UINT64)(UINTN)Emex64TrapEntry;
    __asm__ __volatile__ ("cmov 8, %0" : : "r" (Vector) : "memory");

    Stat = ReadCr (EMEX64_CR_CRSTAT);
    if((Stat & EMEX64_CRSTAT_TRAPS) == 0)
    {
        Stat = (Stat | EMEX64_CRSTAT_TRAPS) & ~(UINT64)(EMEX64_CRSTAT_IE | EMEX64_CRSTAT_PIE);
        __asm__ __volatile__ ("cmov 7, %0" : : "r" (Stat) : "memory");
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI RegisterCpuInterruptHandler(IN EFI_EXCEPTION_TYPE InterruptType,
                                              IN EFI_CPU_INTERRUPT_HANDLER InterruptHandler)
{
    EFI_CPU_INTERRUPT_HANDLER *Slot;
    UINTN Index;

    if(((UINT64)InterruptType & BIT63) != 0)
    {
        Index = (UINTN)((UINT64)InterruptType & ~BIT63);
        if(Index >= EXCEPT_EMEX64_MAX_IRQS)
        {
            return EFI_UNSUPPORTED;
        }

        Slot = &mIrqHandlers[Index];
    }
    else
    {
        if((InterruptType < 0) || (InterruptType >= EXCEPT_EMEX64_MAX_EXCEPTIONS))
        {
            return EFI_UNSUPPORTED;
        }
        Slot = &mExceptionHandlers[InterruptType];
    }

    if((InterruptHandler != NULL) && (*Slot != NULL))
    {
        return EFI_ALREADY_STARTED;
    }

    if((InterruptHandler == NULL) && (*Slot == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    *Slot = InterruptHandler;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI InitializeSeparateExceptionStacks(IN VOID *Buffer,
                                                    IN OUT UINTN *BufferSize)
{
    return EFI_SUCCESS;
}
