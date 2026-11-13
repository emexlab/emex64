//===-- Emex64InstPrinter.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64InstPrinter.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "Emex64GenAsmWriter.inc"

void Emex64InstPrinter::printInst(const MCInst *MI,
                                  uint64_t Address,
                                  StringRef Annot,
                                  const MCSubtargetInfo &STI,
                                  raw_ostream &O)
{
    if(MI->getOpcode() == Emex64::RAW)
    {
        printRaw(MI, Address, STI, O);
    }
    else
    {
        printInstruction(MI, Address, STI, O);
    }
    printAnnotation(O, Annot);
}

void Emex64InstPrinter::printRaw(const MCInst *MI,
                                 uint64_t Address,
                                 const MCSubtargetInfo &STI,
                                 raw_ostream &O)
{
    unsigned Op = MI->getOperand(0).getImm();
    const Emex64OpcodeInfo &Info = getEmex64OpcodeInfo(Op);
    O << '\t' << (Info.Name ? Info.Name : "<bad>");
    for(unsigned I = 1, E = MI->getNumOperands(), N = 0; I < E; ++N)
    {
        O << (N ? ", " : " ");
        int64_t Kind = MI->getOperand(I++).getImm();
        switch(Kind)
        {
            case Emex64Raw::OffsetAdd:
            case Emex64Raw::OffsetSub:
                O << '[';
                printOperand(MI, I, STI, O);
                O << (Kind == Emex64Raw::OffsetAdd ? " + " : " - ");
                printOperand(MI, I + 1, STI, O);
                O << ']';
                I += 2;
                break;
            case Emex64Raw::RegInc:
            case Emex64Raw::RegDec:
                printOperand(MI, I++, STI, O);
                O << (Kind == Emex64Raw::RegInc ? "++" : "--");
                break;
            case Emex64Raw::PCRel8:
            case Emex64Raw::PCRel16:
            case Emex64Raw::PCRel:
            {
                const MCOperand &MO = MI->getOperand(I++);
                if(MO.isImm())
                {
                    O << format_hex(Address + MO.getImm(), 0);
                }
                else
                {
                    MAI.printExpr(O, *MO.getExpr());
                }
                break;
            }
            default:
            {
                const MCOperand &MO = MI->getOperand(I++);
                if(MO.isImm() && (int)N == Info.TargetArg)
                {
                    O << format_hex(static_cast<uint64_t>(MO.getImm()), 0);
                }
                else
                {
                    printOperand(MI, I - 1, STI, O);
                }
                break;
            }
        }
    }
}

void Emex64InstPrinter::printRegName(raw_ostream &O,
                                     MCRegister Reg)
{
    O << getRegisterName(Reg);
}

static void printImmediate(raw_ostream &O,
                           int64_t V)
{
    if(V >= -4096 && V <= 4096)
    {
        O << V;
    }
    else
    {
        O << format_hex(static_cast<uint64_t>(V), 0);
    }
}

void Emex64InstPrinter::printOperand(const MCInst *MI,
                                     unsigned OpNo,
                                     const MCSubtargetInfo &STI,
                                     raw_ostream &O)
{
    const MCOperand &Op = MI->getOperand(OpNo);
    if(Op.isReg())
    {
        printRegName(O, Op.getReg());
    }
    else if(Op.isImm())
    {
        printImmediate(O, Op.getImm());
    }
    else
    {
        assert(Op.isExpr() && "unknown operand kind");
        MAI.printExpr(O, *Op.getExpr());
    }
}

void Emex64InstPrinter::printMemOperand(const MCInst *MI,
                                        unsigned OpNo,
                                        const MCSubtargetInfo &STI,
                                        raw_ostream &O)
{
    const MCOperand &Base = MI->getOperand(OpNo);
    const MCOperand &Off = MI->getOperand(OpNo + 1);
    if(!Base.getReg())
    {
        printOperand(MI, OpNo + 1, STI, O);
        return;
    }
    if(Off.isImm() && Off.getImm() == 0)
    {
        printRegName(O, Base.getReg());
        return;
    }
    O << '[';
    printRegName(O, Base.getReg());
    if(Off.isImm())
    {
        int64_t V = Off.getImm();
        if(V < 0)
        {
            O << " - ";
            printImmediate(O, -V);
        }
        else
        {
            O << " + ";
            printImmediate(O, V);
        }
    }
    else
    {
        O << " + ";
        printOperand(MI, OpNo + 1, STI, O);
    }
    O << ']';
}

void Emex64InstPrinter::printCondCode(const MCInst *MI,
                                      unsigned OpNo,
                                      const MCSubtargetInfo &STI,
                                      raw_ostream &O)
{
    unsigned CC = MI->getOperand(OpNo).getImm();
    O << ((CC & Emex64CC::NZ) ? "nz:" : "z:");
    if(CC & Emex64CC::Z)
    {
        O << 'Z';
    }
    if(CC & Emex64CC::L)
    {
        O << 'L';
    }
    if(CC & Emex64CC::G)
    {
        O << 'G';
    }
    if(CC & Emex64CC::U)
    {
        O << 'U';
    }
}
