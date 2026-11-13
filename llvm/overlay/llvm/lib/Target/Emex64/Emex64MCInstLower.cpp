//===-- Emex64MCInstLower.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64MCInstLower.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/Constants.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static uint16_t getSpecifier(unsigned TargetFlags)
{
    switch(TargetFlags)
    {
        case Emex64II::MO_PCREL:
            return Emex64S::PCREL;
        case Emex64II::MO_PLT:
            return Emex64S::PLT;
        case Emex64II::MO_GOTPCREL:
            return Emex64S::GOTPCREL;
        case Emex64II::MO_TPOFF:
            return Emex64S::TPOFF;
        case Emex64II::MO_GOTTPOFF:
            return Emex64S::GOTTPOFF;
        case Emex64II::MO_TLSGD:
            return Emex64S::TLSGD;
        default:
            return Emex64S::None;
  }
}

MCOperand Emex64MCInstLower::lowerSymbolOperand(const MachineOperand &MO,
                                                MCSymbol *Sym) const
{
    const MCExpr *Expr = MCSymbolRefExpr::create(Sym, getSpecifier(MO.getTargetFlags()), Ctx);
    if(!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    {
        Expr = MCBinaryExpr::createAdd(Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
    }
    return MCOperand::createExpr(Expr);
}

bool Emex64MCInstLower::lowerOperand(const MachineOperand &MO,
                                     MCOperand &MCOp) const
{
    switch(MO.getType())
    {
        case MachineOperand::MO_Register:
            if(MO.isImplicit())
            {
                return false;
            }
            MCOp = MCOperand::createReg(MO.getReg());
            return true;
        case MachineOperand::MO_Immediate:
            MCOp = MCOperand::createImm(MO.getImm());
            return true;
        case MachineOperand::MO_MachineBasicBlock:
            MCOp = MCOperand::createExpr(MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Ctx));
            return true;
        case MachineOperand::MO_GlobalAddress:
        {
            unsigned F = MO.getTargetFlags();
            bool Local = F == Emex64II::MO_PCREL || F == Emex64II::MO_PLT;
            MCOp = lowerSymbolOperand(MO, Local ? Printer.getSymbolPreferLocal(*MO.getGlobal()) : Printer.getSymbol(MO.getGlobal()));
            return true;
        }
        case MachineOperand::MO_ExternalSymbol:
            MCOp = lowerSymbolOperand(MO, Printer.GetExternalSymbolSymbol(MO.getSymbolName()));
            return true;
        case MachineOperand::MO_BlockAddress:
            MCOp = lowerSymbolOperand(MO, Printer.GetBlockAddressSymbol(MO.getBlockAddress()));
            return true;
        case MachineOperand::MO_JumpTableIndex:
            MCOp = lowerSymbolOperand(MO, Printer.GetJTISymbol(MO.getIndex()));
            return true;
        case MachineOperand::MO_ConstantPoolIndex:
            MCOp = lowerSymbolOperand(MO, Printer.GetCPISymbol(MO.getIndex()));
            return true;
        case MachineOperand::MO_RegisterMask:
            return false;
        default:
            report_fatal_error("emex64: unknown operand type");
    }
}

void Emex64MCInstLower::lower(const MachineInstr *MI,
                              MCInst &OutMI) const
{
    OutMI.setOpcode(MI->getOpcode());
    for(const MachineOperand &MO : MI->operands())
    {
        MCOperand MCOp;
        if(lowerOperand(MO, MCOp))
        {
            OutMI.addOperand(MCOp);
        }
    }
}
