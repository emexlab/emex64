//===-- Emex64Disassembler.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "TargetInfo/Emex64TargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDecoderOps.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "emex64-disassembler"

namespace {

class Emex64Disassembler : public MCDisassembler
{
public:
    Emex64Disassembler(const MCSubtargetInfo &STI, MCContext &Ctx) : MCDisassembler(STI, Ctx) {}
    
    DecodeStatus getInstruction(MCInst &MI, uint64_t &Size, ArrayRef<uint8_t> Bytes, uint64_t Address, raw_ostream &CStream) const override;
};

class BitReader {
    ArrayRef<uint8_t> Bytes;

public:
    uint64_t Pos = 0;
    bool Overrun = false;
    explicit BitReader(ArrayRef<uint8_t> B) : Bytes(B) {}
    uint64_t get(unsigned N)
    {
        uint64_t V = 0;
        for(unsigned I = 0; I < N; ++I, ++Pos)
        {
            if((Pos >> 3) >= Bytes.size())
            {
                Overrun = true;
                return 0;
            }
            V |= uint64_t((Bytes[Pos >> 3] >> (Pos & 7)) & 1) << I;
        }
        return V;
    }
    void align()
    {
        Pos = (Pos + 7) & ~7ULL;
    }
};

const MCPhysReg MainRegs[16] = {
    Emex64::PC,     Emex64::SP,     Emex64::FP,     Emex64::CF,     Emex64::FPC,        Emex64::R0,
    Emex64::R1,     Emex64::R2,     Emex64::R3,     Emex64::R4,
    Emex64::R5,     Emex64::R6,     Emex64::R7,     Emex64::R8,
    Emex64::R9,     Emex64::RR
};
const MCPhysReg ExtRegs[16] = {
    Emex64::ER0,    Emex64::ER1,    Emex64::ER2,    Emex64::ER3,
    Emex64::ER4,    Emex64::ER5,    Emex64::ER6,    Emex64::ER7,
    Emex64::ER8,    Emex64::ER9,    Emex64::ER10,   Emex64::ER11,
    Emex64::ER12,   Emex64::ER13,   Emex64::ER14,   Emex64::ER15
};
const MCPhysReg FltRegs[16] = {
    Emex64::FR0,    Emex64::FR1,    Emex64::FR2,    Emex64::FR3,
    Emex64::FR4,    Emex64::FR5,    Emex64::FR6,    Emex64::FR7,
    Emex64::FR8,    Emex64::FR9,    Emex64::FR10,   Emex64::FR11,
    Emex64::FR12,   Emex64::FR13,   Emex64::FR14,   Emex64::FR15
};

const unsigned ImmBits[8] = {0, 0, 4, 8, 16, 32, 64, 64};

bool decodeSimple(BitReader &BR,
                  unsigned Coding,
                  MCInst &MI)
{
    switch(Coding)
    {
        case Emex64Coding::Reg:
            MI.addOperand(MCOperand::createReg(MainRegs[BR.get(4)]));
            return true;
        case Emex64Coding::RegExtended:
            MI.addOperand(MCOperand::createReg(ExtRegs[BR.get(4)]));
            return true;
        case Emex64Coding::RegFloat:
            MI.addOperand(MCOperand::createReg(FltRegs[BR.get(4)]));
            return true;
        case Emex64Coding::Addr64:
            BR.align();
            [[fallthrough]];
        case Emex64Coding::Imm4:
        case Emex64Coding::Imm8:
        case Emex64Coding::Imm16:
        case Emex64Coding::Imm32:
        case Emex64Coding::Imm64:
            MI.addOperand(MCOperand::createImm(BR.get(ImmBits[Coding])));
            return true;
            case Emex64Coding::RegInc:
        case Emex64Coding::RegDec:
            MI.addOperand(MCOperand::createReg(MainRegs[BR.get(4)]));
            return true;
        default:
            return false;
    }
}

} /* namespace */

MCDisassembler::DecodeStatus
Emex64Disassembler::getInstruction(MCInst &MI,
                                   uint64_t &Size,
                                   ArrayRef<uint8_t> Bytes,
                                   uint64_t Address,
                                   raw_ostream &CStream) const
{
    Size = 1;
    if(Bytes.empty())
    {
        return Fail;
    }
    unsigned Op = Bytes[0];
    const Emex64OpcodeInfo &Info = getEmex64OpcodeInfo(Op);
    if(!Info.Name)
    {
        return Fail;
    }
    
    BitReader BR(Bytes);
    BR.get(8);
    MI.setOpcode(Emex64::RAW);
    MI.addOperand(MCOperand::createImm(Op));
    
    for(unsigned Arg = 0; Arg < Info.MaxArgs; ++Arg)
    {
        unsigned Coding = BR.get(4);
        if(Coding == Emex64Coding::End)
        {
            break;
        }
        if(Coding == Emex64Coding::OffsetAdd || Coding == Emex64Coding::OffsetSub)
        {
            MI.addOperand(MCOperand::createImm(Coding == Emex64Coding::OffsetAdd ? Emex64Raw::OffsetAdd : Emex64Raw::OffsetSub));
            if(!decodeSimple(BR, BR.get(4), MI) || !decodeSimple(BR, BR.get(4), MI))
            {
                return Fail;
            }
            continue;
        }
        bool Relative = (int)Arg == Info.TargetArg && Coding >= Emex64Coding::Imm4 && Coding <= Emex64Coding::Imm32;
        if(Relative)
        {
            unsigned Bits = ImmBits[Coding];
            int64_t D = SignExtend64(BR.get(Bits), Bits);
            MI.addOperand(MCOperand::createImm(Emex64Raw::PCRel));
            MI.addOperand(MCOperand::createImm(D));
            continue;
        }
        MI.addOperand(MCOperand::createImm(Coding == Emex64Coding::RegInc ? Emex64Raw::RegInc : Coding == Emex64Coding::RegDec ? Emex64Raw::RegDec : Emex64Raw::Value));
        if(!decodeSimple(BR, Coding, MI))
        {
            return Fail;
        }
    }
    
    if(BR.Overrun)
    {
        return Fail;
    }
    
    Size = (BR.Pos + 7) / 8;
    return Success;
}

static MCDisassembler *createEmex64Disassembler(const Target &T,
                                                const MCSubtargetInfo &STI,
                                                MCContext &Ctx)
{
    return new Emex64Disassembler(STI, Ctx);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeEmex64Disassembler()
{
    TargetRegistry::RegisterMCDisassembler(getTheEmex64Target(), createEmex64Disassembler);
}
