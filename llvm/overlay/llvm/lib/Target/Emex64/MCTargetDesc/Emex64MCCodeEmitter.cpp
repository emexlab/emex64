//===-- Emex64MCCodeEmitter.cpp -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Emex64FixupKinds.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include <algorithm>

using namespace llvm;

namespace {

class BitStream
{
    SmallVector<uint8_t, 32> Bytes;
    uint64_t Pos = 0;

public:
    void put(uint64_t V,
             unsigned N)
    {
        for(unsigned I = 0; I < N; ++I, ++Pos)
        {
            if((Pos >> 3) >= Bytes.size())
            {
                Bytes.push_back(0);
            }
            if((V >> I) & 1)
            {
                Bytes[Pos >> 3] |= 1u << (Pos & 7);
            }
        }
    }

    void align(void)
    {
        Pos = (Pos + 7) & ~7ULL;
        while((Pos >> 3) > Bytes.size())
        {
            Bytes.push_back(0);
        }
    }

    uint64_t bitPos() const
    {
        return Pos;
    }

    ArrayRef<uint8_t> bytes() const
    {
        return Bytes;
    }
};

class Emex64MCCodeEmitter : public MCCodeEmitter
{
    const MCInstrInfo &MCII;
    MCContext &Ctx;

public:
    Emex64MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx) : MCII(MCII), Ctx(Ctx) {}
    void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB, SmallVectorImpl<MCFixup> &Fixups, const MCSubtargetInfo &STI) const override;

private:
    void emitReg(BitStream &BS, MCRegister Reg) const;
    void emitImm(BitStream &BS, uint64_t V) const;
    void emitAddr64(BitStream &BS, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups) const;
    void emitValue(BitStream &BS, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups) const;
    void emitMem(BitStream &BS, const MCOperand &Base, const MCOperand &Off, SmallVectorImpl<MCFixup> &Fixups) const;
    void emitBranchTarget(BitStream &BS, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups, unsigned Bits = 32) const;
    void encodeRaw(BitStream &BS, const MCInst &MI, SmallVectorImpl<MCFixup> &Fixups) const;
};

} /* namespace */

void Emex64MCCodeEmitter::emitReg(BitStream &BS,
                                  MCRegister Reg) const
{
    unsigned Enc = Ctx.getRegisterInfo()->getEncodingValue(Reg);
    unsigned File = Enc >> 4;
    BS.put(File == 0 ? Emex64Coding::Reg : File == 1 ? Emex64Coding::RegExtended : Emex64Coding::RegFloat, 4);
    BS.put(Enc & 0xF, 4);
}

void Emex64MCCodeEmitter::emitImm(BitStream &BS,
                                  uint64_t V) const
{
    if(V < (1ULL << 4))
    {
        BS.put(Emex64Coding::Imm4, 4);
        BS.put(V, 4);
    }
    else if(V < (1ULL << 8))
    {
        BS.put(Emex64Coding::Imm8, 4);
        BS.put(V, 8);
    }
    else if(V < (1ULL << 16))
    {
        BS.put(Emex64Coding::Imm16, 4);
        BS.put(V, 16);
    }
    else if(V < (1ULL << 32))
    {
        BS.put(Emex64Coding::Imm32, 4);
        BS.put(V, 32);
    }
    else
    {
        BS.put(Emex64Coding::Imm64, 4);
        BS.put(V, 64);
    }
}

void Emex64MCCodeEmitter::emitAddr64(BitStream &BS,
                                     const MCOperand &MO,
                                     SmallVectorImpl<MCFixup> &Fixups) const
{
    BS.put(Emex64Coding::Addr64, 4);
    BS.align();
    if(MO.isExpr())
    {
        unsigned Byte = BS.bitPos() / 8;
        const MCExpr *E = MO.getExpr();
        bool PCRel = Emex64S::isPCRelative(getEmex64Specifier(E));
        if(PCRel)
        {
            E = MCBinaryExpr::createAdd(E, MCConstantExpr::create(Byte, Ctx), Ctx);
        }
        Fixups.push_back(MCFixup::create(Byte, E, FK_Data_8, PCRel));
        BS.put(0, 64);
    }
    else
    {
        BS.put(MO.getImm(), 64);
    }
}

void Emex64MCCodeEmitter::emitValue(BitStream &BS,
                                    const MCOperand &MO,
                                    SmallVectorImpl<MCFixup> &Fixups) const
{
    if(MO.isReg())
    {
        emitReg(BS, MO.getReg());
        return;
    }
    if(MO.isImm())
    {
        emitImm(BS, MO.getImm());
        return;
    }
    if(MO.isExpr())
    {
        int64_t V;
        if(MO.getExpr()->evaluateAsAbsolute(V))
        {
            emitImm(BS, V);
            return;
        }
        emitAddr64(BS, MO, Fixups);
        return;
    }
    llvm_unreachable("unexpected operand kind");
}

void Emex64MCCodeEmitter::emitMem(BitStream &BS,
                                  const MCOperand &Base,
                                  const MCOperand &Off,
                                  SmallVectorImpl<MCFixup> &Fixups) const
{
    if(!Base.isReg() || !Base.getReg())
    {
        emitValue(BS, Off, Fixups);
        return;
    }
    if(Off.isReg())
    {
        BS.put(Emex64Coding::OffsetAdd, 4);
        emitReg(BS, Base.getReg());
        emitReg(BS, Off.getReg());
        return;
    }
    if(Off.isImm())
    {
        int64_t V = Off.getImm();
        if(V == 0)
        {
            emitReg(BS, Base.getReg());
            return;
        }
        BS.put(V < 0 ? Emex64Coding::OffsetSub : Emex64Coding::OffsetAdd, 4);
        emitReg(BS, Base.getReg());
        emitImm(BS, V < 0 ? 0 - (uint64_t)V : (uint64_t)V);
        return;
    }
    BS.put(Emex64Coding::OffsetAdd, 4);
    emitReg(BS, Base.getReg());
    emitValue(BS, Off, Fixups);
}

void Emex64MCCodeEmitter::emitBranchTarget(BitStream &BS,
                                           const MCOperand &MO,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           unsigned Bits) const
{
    if(MO.isImm())
    {
        emitAddr64(BS, MO, Fixups);
        return;
    }
    if(MO.isReg())
    {
        emitReg(BS, MO.getReg());
        return;
    }
    BS.put(Bits == 8 ? Emex64Coding::Imm8 : Bits == 16 ? Emex64Coding::Imm16 : Emex64Coding::Imm32, 4);
    uint64_t Bit = BS.bitPos();
    unsigned Byte = Bit / 8;
    bool B4 = Bit & 7;
    const MCExpr *E = MCBinaryExpr::createAdd(MO.getExpr(), MCConstantExpr::create(Byte, Ctx), Ctx);
    unsigned Kind = Bits == 8 ? (B4 ? Emex64::fixup_emex64_pcrel8_b4 : Emex64::fixup_emex64_pcrel8_b0) : Bits == 16 ? (B4 ? Emex64::fixup_emex64_pcrel16_b4 : Emex64::fixup_emex64_pcrel16_b0) : (B4 ? Emex64::fixup_emex64_pcrel32_b4 : Emex64::fixup_emex64_pcrel32_b0);
    Fixups.push_back(MCFixup::create(Byte, E, Kind, /*PCRel=*/true));
    BS.put(0, Bits);
}

void Emex64MCCodeEmitter::encodeRaw(BitStream &BS,
                                    const MCInst &MI,
                                    SmallVectorImpl<MCFixup> &Fixups) const
{
    unsigned Op = MI.getOperand(0).getImm();
    const Emex64OpcodeInfo &Info = getEmex64OpcodeInfo(Op);
    BS.put(Op, 8);
    unsigned Args = 0;
    for(unsigned I = 1, E = MI.getNumOperands(); I < E; ++Args)
    {
        int64_t Kind = MI.getOperand(I++).getImm();
        switch(Kind)
        {
            case Emex64Raw::Value:
            {
                const MCOperand &MO = MI.getOperand(I++);
                if((int)Args == Info.TargetArg && !MO.isReg())
                {
                    emitAddr64(BS, MO, Fixups);
                }
                else
                {
                    emitValue(BS, MO, Fixups);
                }
                break;
            }
            case Emex64Raw::OffsetAdd:
            case Emex64Raw::OffsetSub:
                BS.put(Kind == Emex64Raw::OffsetAdd ? Emex64Coding::OffsetAdd : Emex64Coding::OffsetSub, 4);
                emitValue(BS, MI.getOperand(I), Fixups);
                emitValue(BS, MI.getOperand(I + 1), Fixups);
                I += 2;
                break;
            case Emex64Raw::RegInc:
            case Emex64Raw::RegDec:
            {
                unsigned Enc = Ctx.getRegisterInfo()->getEncodingValue(MI.getOperand(I++).getReg());
                BS.put(Kind == Emex64Raw::RegInc ? Emex64Coding::RegInc : Emex64Coding::RegDec, 4);
                BS.put(Enc & 0xF, 4);
                break;
            }
            case Emex64Raw::PCRel8:
            case Emex64Raw::PCRel16:
            case Emex64Raw::PCRel:
            {
                const MCOperand &MO = MI.getOperand(I++);
                if(MO.isExpr())
                {
                    emitBranchTarget(BS, MO, Fixups, Kind == Emex64Raw::PCRel8 ? 8 : Kind == Emex64Raw::PCRel16 ? 16 : 32);
                    break;
                }
                int64_t D = MO.getImm();
                if(D >= INT8_MIN && D <= INT8_MAX)
                {
                    BS.put(Emex64Coding::Imm8, 4);
                    BS.put(uint64_t(D), 8);
                }
                else if (D >= INT16_MIN && D <= INT16_MAX)
                {
                    BS.put(Emex64Coding::Imm16, 4);
                    BS.put(uint64_t(D), 16);
                }
                else
                {
                    BS.put(Emex64Coding::Imm32, 4);
                    BS.put(uint64_t(D), 32);
                }
                break;
            }
            default:
                llvm_unreachable("bad raw operand kind");
        }
    }
    if(Args != Info.MaxArgs)
    {
        BS.put(Emex64Coding::End, 4);
    }
    BS.align();
}

void Emex64MCCodeEmitter::encodeInstruction(const MCInst &MI,
                                            SmallVectorImpl<char> &CB,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const
{
    const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
    uint64_t TSFlags = Desc.TSFlags;
    if(Desc.isPseudo())
    {
        report_fatal_error("emex64: pseudo instruction reached the encoder");
    }
    
    BitStream BS;
    if(MI.getOpcode() == Emex64::RAW)
    {
        encodeRaw(BS, MI, Fixups);
        for(uint8_t B : BS.bytes())
        {
            CB.push_back(static_cast<char>(B));
        }
        return;
    }
    BS.put(Emex64II::getOpcodeByte(TSFlags), 8);

    unsigned Args = 0;
    unsigned NumDescOps = Desc.getNumOperands();
    unsigned E = std::min<unsigned>(MI.getNumOperands(), NumDescOps);
    for(unsigned I = 0; I < E; ++I)
    {
        if(Desc.getOperandConstraint(I, MCOI::TIED_TO) != -1)
        {
            continue;
        }
        uint8_t Type = Desc.operands()[I].OperandType;
        if(Type == MCOI::OPERAND_MEMORY)
        {
            emitMem(BS, MI.getOperand(I), MI.getOperand(I + 1), Fixups);
            ++I;
        }
        else if(Type == MCOI::OPERAND_PCREL)
        {
            emitBranchTarget(BS, MI.getOperand(I), Fixups);
        }
        else if(Type == Emex64Op::OPERAND_CALLTARGET)
        {
            const MCOperand &MO = MI.getOperand(I);
            if(MO.isReg())
            {
                emitReg(BS, MO.getReg());
            }
            else
            {
                emitAddr64(BS, MO, Fixups);
            }
        }
        else
        {
            emitValue(BS, MI.getOperand(I), Fixups);
        }
        ++Args;
    }
    
    unsigned MaxArgs = getEmex64OpcodeInfo(Emex64II::getOpcodeByte(TSFlags)).MaxArgs;
    assert(Args <= MaxArgs && "more operands than the opcode takes");
    if(Args != MaxArgs)
    {
        BS.put(Emex64Coding::End, 4);
    }
    BS.align();
    
    for(uint8_t B : BS.bytes())
    {
        CB.push_back(static_cast<char>(B));
    }
}

MCCodeEmitter *llvm::createEmex64MCCodeEmitter(const MCInstrInfo &MCII,
                                               MCContext &Ctx)
{
    return new Emex64MCCodeEmitter(MCII, Ctx);
}
