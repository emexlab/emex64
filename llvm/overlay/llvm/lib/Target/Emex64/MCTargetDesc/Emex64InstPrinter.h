//===-- Emex64InstPrinter.h -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64INSTPRINTER_H
#define LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64INSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class Emex64InstPrinter : public MCInstPrinter
{
public:
    Emex64InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII, const MCRegisterInfo &MRI) : MCInstPrinter(MAI, MII, MRI) {}

    void printInst(const MCInst *MI, uint64_t Address, StringRef Annot, const MCSubtargetInfo &STI, raw_ostream &O) override;
    void printRegName(raw_ostream &O, MCRegister Reg) override;
    
    std::pair<const char *, uint64_t> getMnemonic(const MCInst &MI) const override;
    void printInstruction(const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI, raw_ostream &O);
    static const char *getRegisterName(MCRegister Reg);
    
    void printOperand(const MCInst *MI, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O);
    void printOperand(const MCInst *MI, uint64_t Address, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O)
    {
        printOperand(MI, OpNo, STI, O);
    }
    void printRaw(const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI, raw_ostream &O);
    void printMemOperand(const MCInst *MI, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O);
    void printCondCode(const MCInst *MI, unsigned OpNo, const MCSubtargetInfo &STI, raw_ostream &O);
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64INSTPRINTER_H */
