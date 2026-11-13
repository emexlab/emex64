//===-- Emex64TargetMachine.cpp -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64TargetMachine.h"
#include "Emex64.h"
#include "Emex64MachineFunctionInfo.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "TargetInfo/Emex64TargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {

class Emex64TargetObjectFile : public TargetLoweringObjectFileELF
{
public:
    const MCExpr *getDebugThreadLocalSymbol(const MCSymbol *Sym) const override
    {
        return MCSymbolRefExpr::create(Sym, Emex64S::DTPOFF, getContext());
    }
};
} /* namespace */

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEmex64Target()
{
    RegisterTargetMachine<Emex64TargetMachine> X(getTheEmex64Target());
    PassRegistry &PR = *PassRegistry::getPassRegistry();
    initializeEmex64AsmPrinterPass(PR);
    initializeEmex64DAGToDAGISelLegacyPass(PR);
}

Emex64TargetMachine::Emex64TargetMachine(const Target &T,
                                         const Triple &TT,
                                         StringRef CPU,
                                         StringRef FS,
                                         const TargetOptions &Options,
                                         std::optional<Reloc::Model> RM,
                                         std::optional<CodeModel::Model> CM,
                                         CodeGenOptLevel OL,
                                         bool JIT) : CodeGenTargetMachineImpl(T, TT.computeDataLayout(), TT, CPU, FS, Options, RM.value_or(Reloc::Static), getEffectiveCodeModel(CM, CodeModel::Small), OL), TLOF(std::make_unique<Emex64TargetObjectFile>())
{
    initAsmInfo();
}

Emex64TargetMachine::~Emex64TargetMachine() = default;

const Emex64Subtarget *Emex64TargetMachine::getSubtargetImpl(const Function &F) const
{
    Attribute CPUAttr = F.getFnAttribute("target-cpu");
    Attribute FSAttr = F.getFnAttribute("target-features");
    std::string CPU = CPUAttr.isValid() ? CPUAttr.getValueAsString().str() : TargetCPU;
    std::string FS = FSAttr.isValid() ? FSAttr.getValueAsString().str() : TargetFS;
    auto &I = SubtargetMap[CPU + FS];
    if(!I)
    {
        I = std::make_unique<Emex64Subtarget>(TargetTriple, CPU, FS, *this);
    }
    return I.get();
}

MachineFunctionInfo *Emex64TargetMachine::createMachineFunctionInfo(BumpPtrAllocator &Allocator,
                                                                    const Function &F,
                                                                    const TargetSubtargetInfo *STI) const
{
    return Emex64MachineFunctionInfo::create<Emex64MachineFunctionInfo>(Allocator, F, STI);
}

namespace {

class Emex64PassConfig : public TargetPassConfig
{
public:
    Emex64PassConfig(Emex64TargetMachine &TM, PassManagerBase &PM) : TargetPassConfig(TM, PM) {}
    
    Emex64TargetMachine &getEmex64TargetMachine() const
    {
        return getTM<Emex64TargetMachine>();
    }
    
    void addIRPasses() override
    {
        addPass(createAtomicExpandLegacyPass());
        TargetPassConfig::addIRPasses();
    }
    
    bool addInstSelector() override
    {
        addPass(createEmex64ISelDag(getEmex64TargetMachine(), getOptLevel()));
        return false;
    }
};

} /* namespace */

TargetPassConfig *Emex64TargetMachine::createPassConfig(PassManagerBase &PM)
{
    return new Emex64PassConfig(*this, PM);
}
