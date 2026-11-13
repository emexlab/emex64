//===-- Emex64AsmBackend.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Emex64FixupKinds.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  
class Emex64AsmBackend : public MCAsmBackend
{
    uint8_t OSABI;

public:
    Emex64AsmBackend(uint8_t OSABI) : MCAsmBackend(llvm::endianness::little), OSABI(OSABI) {}
    void applyFixup(const MCFragment &F, const MCFixup &Fixup, const MCValue &Target, uint8_t *Data, uint64_t Value, bool IsResolved) override;
    
    std::unique_ptr<MCObjectTargetWriter> createObjectTargetWriter() const override
    {
        return createEmex64ELFObjectWriter(OSABI);
    }
    
    MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override;
    std::optional<bool> evaluateFixup(const MCFragment &F, MCFixup &Fixup, MCValue &Target, uint64_t &Value) override;
    
    bool mayNeedRelaxation(unsigned Opcode, ArrayRef<MCOperand> Operands, const MCSubtargetInfo &STI) const override;
    bool fixupNeedsRelaxationAdvanced(const MCFragment &F, const MCFixup &Fixup, const MCValue &Target, uint64_t Value, bool Resolved) const override;
    void relaxInstruction(MCInst &Inst, const MCSubtargetInfo &STI) const override;
    
    bool writeNopData(raw_ostream &OS,
                      uint64_t Count,
                      const MCSubtargetInfo *STI) const override
    {
        for(uint64_t I = 0; I < Count; ++I)
        {
            OS << '\x01';
        }
        return true;
    }
};

} /* namespace */

MCFixupKindInfo Emex64AsmBackend::getFixupKindInfo(MCFixupKind Kind) const
{
    static const MCFixupKindInfo Infos[Emex64::NumTargetFixupKinds] = {
        {"fixup_emex64_pcrel8_b0", 0, 8, 0},
        {"fixup_emex64_pcrel8_b4", 4, 8, 0},
        {"fixup_emex64_pcrel16_b0", 0, 16, 0},
        {"fixup_emex64_pcrel16_b4", 4, 16, 0},
        {"fixup_emex64_pcrel32_b0", 0, 32, 0},
        {"fixup_emex64_pcrel32_b4", 4, 32, 0},
    };
    if(Kind < FirstTargetFixupKind)
    {
        return MCAsmBackend::getFixupKindInfo(Kind);
    }
    assert(unsigned(Kind - FirstTargetFixupKind) < Emex64::NumTargetFixupKinds && "invalid fixup kind");
    return Infos[Kind - FirstTargetFixupKind];
}

std::optional<bool> Emex64AsmBackend::evaluateFixup(const MCFragment &F,
                                                    MCFixup &Fixup,
                                                    MCValue &Target,
                                                    uint64_t &Value)
{
    switch(Target.getSpecifier())
    {
        case Emex64S::GOTPCREL:
        case Emex64S::TPOFF:
        case Emex64S::GOTTPOFF:
        case Emex64S::TLSGD:
        case Emex64S::DTPOFF:
            Value = 0;
            return false;
        default:
            return {};
    }
}

static int findRelaxableKind(ArrayRef<MCOperand> Ops)
{
    for(unsigned I = 1, E = Ops.size(); I < E;)
    {
        if(!Ops[I].isImm())
        {
            return -1;
        }
        int64_t K = Ops[I].getImm();
        if(K == Emex64Raw::PCRel8 || K == Emex64Raw::PCRel16)
        {
            return I;
        }
        I += 1 + Emex64Raw::getNumKindOperands(K);
    }
    return -1;
}

bool Emex64AsmBackend::mayNeedRelaxation(unsigned Opcode,
                                         ArrayRef<MCOperand> Operands,
                                         const MCSubtargetInfo &STI) const
{
    return Opcode == Emex64::RAW && findRelaxableKind(Operands) >= 0;
}

bool Emex64AsmBackend::fixupNeedsRelaxationAdvanced(const MCFragment &F,
                                                    const MCFixup &Fixup,
                                                    const MCValue &Target,
                                                    uint64_t Value,
                                                    bool Resolved) const
{
    unsigned Bits = Emex64::getBranchFixupBits(Fixup.getKind());
    if(Bits == 0 || Bits == 32)
    {
        return false;
    }
    if(!Resolved)
    {
        return true;
    }
    return !isIntN(Bits, static_cast<int64_t>(Value));
}

void Emex64AsmBackend::relaxInstruction(MCInst &Inst,
                                        const MCSubtargetInfo &STI) const
{
    int I = findRelaxableKind(Inst.getOperands());
    assert(I >= 0 && "nothing to relax");
    MCOperand &K = Inst.getOperand(I);
    K.setImm(K.getImm() == Emex64Raw::PCRel8 ? Emex64Raw::PCRel16 : Emex64Raw::PCRel);
}

void Emex64AsmBackend::applyFixup(const MCFragment &F,
                                  const MCFixup &Fixup,
                                  const MCValue &Target,
                                  uint8_t *Data,
                                  uint64_t Value,
                                  bool IsResolved)
{
    maybeAddReloc(F, Fixup, Target, Value, IsResolved);

    unsigned Kind = Fixup.getKind();
    switch(Kind)
    {
        case FK_Data_1:
        case FK_Data_2:
        case FK_Data_4:
        case FK_Data_8:
        {
            unsigned N = Kind == FK_Data_1 ? 1 : Kind == FK_Data_2 ? 2 : Kind == FK_Data_4 ? 4 : 8;
            for(unsigned I = 0; I < N; ++I)
            {
                Data[I] |= uint8_t(Value >> (8 * I));
            }
            return;
        }
        default:
            break;
    }

    unsigned Bits = Emex64::getBranchFixupBits(Kind);
    if(!Bits)
    {
        llvm_unreachable("unknown fixup kind");
    }
    if(!IsResolved)
    {
        if(Bits != 32)
        {
            getContext().reportError(Fixup.getLoc(), "a short branch needs a relocation");
        }
        return;
    }
    int64_t SV = static_cast<int64_t>(Value);
    if(!isIntN(Bits, SV))
    {
        getContext().reportError(Fixup.getLoc(), "branch target out of range");
        return;
    }
    unsigned Shift = Emex64::isBranchFixupB4(Kind) ? 4 : 0;
    unsigned NumBytes = (Shift + Bits + 7) / 8;
    uint64_t Mask = maskTrailingOnes<uint64_t>(Bits);
    uint64_t Cur = 0;
    for(unsigned I = 0; I < NumBytes; ++I)
    {
        Cur |= uint64_t(Data[I]) << (8 * I);
    }
    Cur &= ~(Mask << Shift);
    Cur |= (uint64_t(Value) & Mask) << Shift;
    for(unsigned I = 0; I < NumBytes; ++I)
    {
        Data[I] = uint8_t(Cur >> (8 * I));
    }
}

MCAsmBackend *llvm::createEmex64AsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options)
{
    uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(STI.getTargetTriple().getOS());
    return new Emex64AsmBackend(OSABI);
}
