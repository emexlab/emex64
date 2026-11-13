//===-- Emex64TargetMachine.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64TARGETMACHINE_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64TARGETMACHINE_H

#include "Emex64Subtarget.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include <optional>

namespace llvm {

class Emex64TargetMachine : public CodeGenTargetMachineImpl
{
    std::unique_ptr<TargetLoweringObjectFile> TLOF;
    mutable StringMap<std::unique_ptr<Emex64Subtarget>> SubtargetMap;

public:
    Emex64TargetMachine(const Target &T, const Triple &TT, StringRef CPU, StringRef FS, const TargetOptions &Options, std::optional<Reloc::Model> RM, std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT);
    ~Emex64TargetMachine() override;

    const Emex64Subtarget *getSubtargetImpl(const Function &F) const override;
    const Emex64Subtarget *getSubtargetImpl() const = delete;

    TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

    TargetLoweringObjectFile *getObjFileLowering(void) const override
    {
        return TLOF.get();
    }

    MachineFunctionInfo *createMachineFunctionInfo(BumpPtrAllocator &Allocator, const Function &F, const TargetSubtargetInfo *STI) const override;
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64TARGETMACHINE_H */
