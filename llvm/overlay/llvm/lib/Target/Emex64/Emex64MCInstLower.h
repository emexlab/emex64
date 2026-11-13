//===-- Emex64MCInstLower.h - MachineInstr to MCInst ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64MCINSTLOWER_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64MCINSTLOWER_H

#include "llvm/Support/Compiler.h"

namespace llvm {

class AsmPrinter;
class MCContext;
class MCInst;
class MCOperand;
class MCSymbol;
class MachineInstr;
class MachineOperand;

class Emex64MCInstLower
{
    MCContext &Ctx;
    AsmPrinter &Printer;

public:
    Emex64MCInstLower(MCContext &Ctx, AsmPrinter &Printer) : Ctx(Ctx), Printer(Printer) {}

    void lower(const MachineInstr *MI, MCInst &OutMI) const;
    bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;

private:
    MCOperand lowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64MCINSTLOWER_H */
