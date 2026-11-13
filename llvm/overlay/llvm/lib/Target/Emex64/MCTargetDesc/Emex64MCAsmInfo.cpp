//===-- Emex64MCAsmInfo.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64MCAsmInfo.h"
#include "Emex64MCTargetDesc.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

static bool isRegisterName(StringRef Name)
{
    std::string N = Name.lower();
    StringRef S(N);
    if(S == "pc" || S == "sp" || S == "fp" || S == "cf" || S == "fpc" || S == "rr" || S == "tp")
    {
        return true;
    }
    unsigned Num, Limit;
    if(S.consume_front("er") || S.consume_front("fr"))
    {
        Limit = 16;
    }
    else if(S.consume_front("r"))
    {
        Limit = 10;
    }
    else
    {
        return false;
    }

    return !S.getAsInteger(10, Num) && Num < Limit;
}

void Emex64MCAsmInfo::anchor() {}

bool Emex64MCAsmInfo::isValidUnquotedName(StringRef Name) const
{
    return !isRegisterName(Name) && MCAsmInfoELF::isValidUnquotedName(Name);
}

Emex64MCAsmInfo::Emex64MCAsmInfo(const Triple &TT,
                                 const MCTargetOptions &Options) : MCAsmInfoELF(Options)
{
    IsLittleEndian = true;
    CodePointerSize = 8;
    CalleeSaveStackSlotSize = 8;
    CommentString = ";";
    SeparatorString = "%%";
    InternalSymbolPrefix = ".L";
    MinInstAlignment = 1;
    SupportsDebugInformation = true;
    ExceptionsType = ExceptionHandling::DwarfCFI;
    UsesELFSectionDirectiveForBSS = true;
    Data64bitsDirective = "\t.quad\t";
    
    static const AtSpecifier Specifiers[] = {
        { Emex64S::PCREL,       "pcrel" },
        { Emex64S::PLT,         "plt" },
        { Emex64S::GOTPCREL,    "gotpcrel" },
        { Emex64S::TPOFF,       "tpoff" },
        { Emex64S::GOTTPOFF,    "gottpoff" },
        { Emex64S::TLSGD,       "tlsgd" },
        { Emex64S::DTPOFF,      "dtpoff" },
    };
    
    initializeAtSpecifiers(Specifiers);
}
