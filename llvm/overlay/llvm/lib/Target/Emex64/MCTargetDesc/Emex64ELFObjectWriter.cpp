//===-- Emex64ELFObjectWriter.cpp -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Emex64FixupKinds.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class Emex64ELFObjectWriter : public MCELFObjectTargetWriter
{

public:
    explicit Emex64ELFObjectWriter(uint8_t OSABI) : MCELFObjectTargetWriter(true, OSABI, ELF::EM_EMEX64, true) {}

protected:
    unsigned getRelocType(const MCFixup &Fixup,
                          const MCValue &Target,
                          bool IsPCRel) const override
    {
        unsigned Spec = Target.getSpecifier();
        unsigned Kind = Fixup.getKind();

        switch(Spec)
        {
            case Emex64S::TPOFF:
            case Emex64S::GOTTPOFF:
            case Emex64S::TLSGD:
            case Emex64S::DTPOFF:
            if(auto *SA = static_cast<const MCSymbolELF *>(Target.getAddSym()))
            {
                SA->setType(ELF::STT_TLS);
            }
            break;
        default:
            break;
        }

        if(Kind == FK_Data_8)
        {
            switch(Spec)
            {
                case Emex64S::GOTPCREL:
                    return ELF::R_EMEX64_GOTPCREL64;
                case Emex64S::GOTTPOFF:
                    return ELF::R_EMEX64_GOTTPOFF64;
                case Emex64S::TLSGD:
                    return ELF::R_EMEX64_TLSGD64;
                case Emex64S::TPOFF:
                    return ELF::R_EMEX64_TPOFF64;
                case Emex64S::DTPOFF:
                    return ELF::R_EMEX64_DTPOFF64;
                case Emex64S::None:
                case Emex64S::PCREL:
                    return IsPCRel ? ELF::R_EMEX64_PC64 : ELF::R_EMEX64_ABS64;
                default:
                    break;
            }
        }
        else if(Spec == Emex64S::None || Spec == Emex64S::PCREL || Spec == Emex64S::PLT)
        {
            switch(Kind)
            {
                case FK_NONE:
                    return ELF::R_EMEX64_NONE;
                case FK_Data_4:
                    if(Spec == Emex64S::None)
                    {
                        return IsPCRel ? ELF::R_EMEX64_PC32 : ELF::R_EMEX64_ABS32;
                    }
                    break;
                case FK_Data_2:
                    if(!IsPCRel && Spec == Emex64S::None)
                    {
                        return ELF::R_EMEX64_ABS16;
                    }
                    break;
                case FK_Data_1:
                    if(!IsPCRel && Spec == Emex64S::None)
                    {
                        return ELF::R_EMEX64_ABS8;
                    }
                    break;
                case Emex64::fixup_emex64_pcrel32_b0:
                    return Spec == Emex64S::PLT ? ELF::R_EMEX64_PLT32 : ELF::R_EMEX64_BRANCH32;
                case Emex64::fixup_emex64_pcrel32_b4:
                    return Spec == Emex64S::PLT ? ELF::R_EMEX64_PLT32_B4 : ELF::R_EMEX64_BRANCH32_B4;
                default:
                    break;
            }
        }
        getContext().reportError(Fixup.getLoc(), "unsupported relocation");
        return ELF::R_EMEX64_NONE;
    }

    bool needsRelocateWithSymbol(const MCValue &,
                                 unsigned Type) const override
    {
        switch(Type)
        {
            case ELF::R_EMEX64_GOTPCREL64:
            case ELF::R_EMEX64_GOTTPOFF64:
            case ELF::R_EMEX64_TLSGD64:
            case ELF::R_EMEX64_TPOFF64:
            case ELF::R_EMEX64_DTPOFF64:
            case ELF::R_EMEX64_PLT32:
            case ELF::R_EMEX64_PLT32_B4:
                return true;
            default:
                return false;
        }
    }
};

} /* namespace */

std::unique_ptr<MCObjectTargetWriter> llvm::createEmex64ELFObjectWriter(uint8_t OSABI)
{
    return std::make_unique<Emex64ELFObjectWriter>(OSABI);
}
