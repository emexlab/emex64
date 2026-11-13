//===-- Emex64MCTargetDesc.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64MCTargetDesc.h"
#include "Emex64InstPrinter.h"
#include "Emex64MCAsmInfo.h"
#include "TargetInfo/Emex64TargetInfo.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "Emex64GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "Emex64GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "Emex64GenRegisterInfo.inc"

using namespace llvm;

uint16_t llvm::getEmex64Specifier(const MCExpr *E)
{
    switch(E->getKind())
    {
        case MCExpr::SymbolRef:
        return cast<MCSymbolRefExpr>(E)->getSpecifier();
        case MCExpr::Binary:
        {
            auto *B = cast<MCBinaryExpr>(E);
            if(uint16_t S = getEmex64Specifier(B->getLHS()))
            {
                return S;
            }
            return getEmex64Specifier(B->getRHS());
        }
        case MCExpr::Unary:
            return getEmex64Specifier(cast<MCUnaryExpr>(E)->getSubExpr());
        default:
            return Emex64S::None;
    }
}

const MCExpr *llvm::setEmex64Specifier(const MCExpr *E,
                                       uint16_t S,
                                       MCContext &Ctx)
{
    switch(E->getKind())
    {
        case MCExpr::SymbolRef:
        {
            auto *R = cast<MCSymbolRefExpr>(E);
            return MCSymbolRefExpr::create(&R->getSymbol(), S, Ctx, R->getLoc());
        }
        case MCExpr::Binary:
        {
            auto *B = cast<MCBinaryExpr>(E);
            return MCBinaryExpr::create(B->getOpcode(), setEmex64Specifier(B->getLHS(), S, Ctx), B->getRHS(), Ctx, B->getLoc());
        }
        default:
            return E;
    }
}

static MCInstrInfo *createEmex64MCInstrInfo()
{
    MCInstrInfo *X = new MCInstrInfo();
    InitEmex64MCInstrInfo(X);
    return X;
}

static MCRegisterInfo *createEmex64MCRegisterInfo(const Triple &TT)
{
    MCRegisterInfo *X = new MCRegisterInfo();
    InitEmex64MCRegisterInfo(X, Emex64::PC, 0, 0, Emex64::PC);
    return X;
}

static MCSubtargetInfo *createEmex64MCSubtargetInfo(const Triple &TT,
                                                    StringRef CPU,
                                                    StringRef FS)
{
    if(CPU.empty())
    {
        CPU = "generic";
    }
    return createEmex64MCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

static MCInstPrinter *createEmex64MCInstPrinter(const Triple &T,
                                                unsigned SyntaxVariant,
                                                const MCAsmInfo &MAI,
                                                const MCInstrInfo &MII,
                                                const MCRegisterInfo &MRI)
{
    return new Emex64InstPrinter(MAI, MII, MRI);
}

static MCAsmInfo *createEmex64MCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &Options)
{
    MCAsmInfo *MAI = new Emex64MCAsmInfo(TT, Options);
    unsigned FP = MRI.getDwarfRegNum(Emex64::FP, true);
    unsigned PC = MRI.getDwarfRegNum(Emex64::PC, true);
    MAI->addInitialFrameState(MCCFIInstruction::cfiDefCfa(nullptr, FP, 16));
    MAI->addInitialFrameState(MCCFIInstruction::createOffset(nullptr, PC, 0));
    MAI->addInitialFrameState(MCCFIInstruction::createOffset(nullptr, FP, -8));
    return MAI;
}

namespace {

class Emex64MCInstrAnalysis : public MCInstrAnalysis
{
public:
    explicit Emex64MCInstrAnalysis(const MCInstrInfo *Info) : MCInstrAnalysis(Info) {}
};

} /* namespace */

static MCInstrAnalysis *createEmex64InstrAnalysis(const MCInstrInfo *Info)
{
    return new Emex64MCInstrAnalysis(Info);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEmex64TargetMC()
{
    Target &T = getTheEmex64Target();
    TargetRegistry::RegisterMCAsmInfo(T, createEmex64MCAsmInfo);
    TargetRegistry::RegisterMCInstrInfo(T, createEmex64MCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(T, createEmex64MCRegisterInfo);
    TargetRegistry::RegisterMCSubtargetInfo(T, createEmex64MCSubtargetInfo);
    TargetRegistry::RegisterMCCodeEmitter(T, createEmex64MCCodeEmitter);
    TargetRegistry::RegisterMCAsmBackend(T, createEmex64AsmBackend);
    TargetRegistry::RegisterMCInstPrinter(T, createEmex64MCInstPrinter);
    TargetRegistry::RegisterMCInstrAnalysis(T, createEmex64InstrAnalysis);
}
