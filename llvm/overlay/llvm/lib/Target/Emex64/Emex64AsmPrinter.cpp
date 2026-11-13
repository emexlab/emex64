//===-- Emex64AsmPrinter.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64.h"
#include "Emex64MCInstLower.h"
#include "Emex64TargetMachine.h"
#include "MCTargetDesc/Emex64InstPrinter.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "llvm/MC/MCInstrInfo.h"
#include "TargetInfo/Emex64TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class Emex64AsmPrinter : public AsmPrinter
{
public:
    static char ID;
    
    explicit Emex64AsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer) : AsmPrinter(TM, std::move(Streamer), ID) {}
    StringRef getPassName(void) const override
    {
        return "emex64 Assembly Printer";
    }

    void emitInstruction(const MachineInstr *MI) override;
    bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &OS) override;
    bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &OS) override;

private:
    void emitBranchOnCF(const MachineInstr *MI);
    void emitBranch(const MCInst &Inst);
};

} /* namespace */

char Emex64AsmPrinter::ID = 0;

void Emex64AsmPrinter::emitBranchOnCF(const MachineInstr *MI)
{
    const MCExpr *Target = MCSymbolRefExpr::create(MI->getOperand(0).getMBB()->getSymbol(), OutContext);
    unsigned CC = MI->getOperand(1).getImm();
    unsigned Mask = CC & 0xF;
    bool NZ = CC & Emex64CC::NZ;

    unsigned Opc = 0;
    if(NZ)
    {
        switch(Mask)
        {
            case Emex64CC::Z:
                Opc = Emex64::BE;
                break;
            case Emex64CC::L:
                Opc = Emex64::BLT;
                break;
            case Emex64CC::G:
                Opc = Emex64::BGT;
                break;
            case Emex64CC::L | Emex64CC::Z:
                Opc = Emex64::BLE;
                break;
            case Emex64CC::G | Emex64CC::Z:
                Opc = Emex64::BGE;
                break;
            default: break;
        }
    }
    else if(Mask == Emex64CC::Z)
    {
        Opc = Emex64::BNE;
    }
    
    MCInst Inst;
    if(Opc)
    {
        Inst.setOpcode(Opc);
        Inst.addOperand(MCOperand::createExpr(Target));
    }
    else
    {
        Inst.setOpcode(NZ ? Emex64::BBNZ : Emex64::BBZ);
        Inst.addOperand(MCOperand::createReg(Emex64::CF));
        Inst.addOperand(MCOperand::createImm(Mask));
        Inst.addOperand(MCOperand::createExpr(Target));
    }
    emitBranch(Inst);
}

void Emex64AsmPrinter::emitBranch(const MCInst &In)
{
    const MCInstrDesc &Desc = TM.getMCInstrInfo()->get(In.getOpcode());
    MCInst Raw;
    Raw.setOpcode(Emex64::RAW);
    Raw.addOperand(MCOperand::createImm(Emex64II::getOpcodeByte(Desc.TSFlags)));
    unsigned E = std::min<unsigned>(In.getNumOperands(), Desc.getNumOperands());
    for(unsigned I = 0; I < E; ++I)
    {
        const MCOperand &MO = In.getOperand(I);
        uint8_t Type = Desc.operands()[I].OperandType;
        bool IsTarget = Type == MCOI::OPERAND_PCREL || Type == Emex64Op::OPERAND_CALLTARGET;
        if(IsTarget && MO.isExpr())
        {
            uint16_t S = Type == MCOI::OPERAND_PCREL ? Emex64S::PCREL : Emex64S::PLT;
            Raw.addOperand(MCOperand::createImm(Emex64Raw::PCRel8));
            Raw.addOperand(MCOperand::createExpr(setEmex64Specifier(MO.getExpr(), S, OutContext)));
            continue;
        }
        Raw.addOperand(MCOperand::createImm(Emex64Raw::Value));
        Raw.addOperand(MO);
    }
    EmitToStreamer(*OutStreamer, Raw);
}

void Emex64AsmPrinter::emitInstruction(const MachineInstr *MI)
{
    Emex64_MC::verifyInstructionPredicates(MI->getOpcode(), getSubtargetInfo().getFeatureBits());
    
    switch(MI->getOpcode())
    {
        case Emex64::BRCC:
            emitBranchOnCF(MI);
            return;
        case Emex64::ADDCrr:
        case Emex64::ADDCri:
        case Emex64::SUBCrr:
        case Emex64::SUBCri:
        {
            MCInst Clear;
            Clear.setOpcode(Emex64::MOVri);
            Clear.addOperand(MCOperand::createReg(Emex64::CF));
            Clear.addOperand(MCOperand::createImm(0));
            EmitToStreamer(*OutStreamer, Clear);
            
            unsigned Opc;
            switch(MI->getOpcode())
            {
                case Emex64::ADDCrr:
                    Opc = Emex64::ADCrr;
                    break;
                case Emex64::ADDCri:
                    Opc = Emex64::ADCri;
                    break;
                case Emex64::SUBCrr:
                    Opc = Emex64::SBBrr;
                    break;
                default:
                    Opc = Emex64::SBBri;
                    break;
            }
            
            Emex64MCInstLower Lower(OutContext, *this);
            MCInst Inst;
            Lower.lower(MI, Inst);
            Inst.setOpcode(Opc);
            EmitToStreamer(*OutStreamer, Inst);
            return;
        }
        case Emex64::FENCE:
        {
            MCInst Inst;
            Inst.setOpcode(Emex64::XCHG);
            Inst.addOperand(MCOperand::createReg(Emex64::CF));
            Inst.addOperand(MCOperand::createReg(Emex64::CF));
            Inst.addOperand(MCOperand::createReg(Emex64::SP));
            Inst.addOperand(MCOperand::createImm(0));
            EmitToStreamer(*OutStreamer, Inst);
            return;
        }
        default:
            break;
    }
    
    Emex64MCInstLower Lower(OutContext, *this);
    MCInst Inst;
    Lower.lower(MI, Inst);
    switch(MI->getOpcode())
    {
        case Emex64::B:
        case Emex64::BA:
        case Emex64::BE:
        case Emex64::BNE:
        case Emex64::BLT:
        case Emex64::BGT:
        case Emex64::BLE:
        case Emex64::BGE:
        case Emex64::BZ:
        case Emex64::BNZ:
        case Emex64::BBZ:
        case Emex64::BBNZ:
        case Emex64::CBLTrr:
        case Emex64::CBLTri:
        case Emex64::CBLTir:
        case Emex64::CBEQrr:
        case Emex64::CBEQri:
        case Emex64::CBEQir:
        case Emex64::CBNErr:
        case Emex64::CBNEri:
        case Emex64::CBNEir:
        case Emex64::CBGErr:
        case Emex64::CBGEri:
        case Emex64::CBGEir:
        case Emex64::CBLTUrr:
        case Emex64::CBLTUri:
        case Emex64::CBLTUir:
        case Emex64::CBGEUrr:
        case Emex64::CBGEUri:
        case Emex64::CBGEUir:
        case Emex64::BL:
            emitBranch(Inst);
            return;
        default:
            break;
    }

    EmitToStreamer(*OutStreamer, Inst);
}

bool Emex64AsmPrinter::PrintAsmOperand(const MachineInstr *MI,
                                       unsigned OpNo,
                                       const char *ExtraCode,
                                       raw_ostream &OS)
{
    if(ExtraCode && ExtraCode[0])
    {
        return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS);
    }
    const MachineOperand &MO = MI->getOperand(OpNo);
    switch(MO.getType())
    {
        case MachineOperand::MO_Register:
            OS << Emex64InstPrinter::getRegisterName(MO.getReg());
            return false;
        case MachineOperand::MO_Immediate:
            OS << MO.getImm();
            return false;
        case MachineOperand::MO_GlobalAddress:
            PrintSymbolOperand(MO, OS);
            return false;
        default:
            return true;
    }
}

bool Emex64AsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                             unsigned OpNo,
                                             const char *ExtraCode,
                                             raw_ostream &OS)
{
    if(ExtraCode && ExtraCode[0])
    {
        return true;
    }
    const MachineOperand &Base = MI->getOperand(OpNo);
    const MachineOperand &Off = MI->getOperand(OpNo + 1);
    bool HasBase = Base.isReg() && Base.getReg();
    if(!HasBase)
    {
        if(Off.isImm())
        {
            OS << Off.getImm();
        }
        else if (Off.isGlobal())
        {
            PrintSymbolOperand(Off, OS);
        }
        else
        {
            return true;
        }
        return false;
    }
    OS << '[' << Emex64InstPrinter::getRegisterName(Base.getReg());
    if(Off.isImm())
    {
        if(Off.getImm() < 0)
        {
            OS << " - " << -Off.getImm();
        }
        else
        {
            OS << " + " << Off.getImm();
        }
    }
    else if (Off.isGlobal())
    {
        OS << " + ";
        PrintSymbolOperand(Off, OS);
    }
    else
    {
        return true;
    }
    OS << ']';
    return false;
}

INITIALIZE_PASS(Emex64AsmPrinter, "emex64-asm-printer", "emex64 Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEmex64AsmPrinter()
{
    RegisterAsmPrinter<Emex64AsmPrinter> X(getTheEmex64Target());
}
