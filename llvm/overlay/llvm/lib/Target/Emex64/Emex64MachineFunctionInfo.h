//===-- Emex64MachineFunctionInfo.h -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class Emex64MachineFunctionInfo : public MachineFunctionInfo
{
    int VarArgsFrameIndex = 0;

public:
    Emex64MachineFunctionInfo(const Function &F, const TargetSubtargetInfo *STI) {}

    MachineFunctionInfo *clone(BumpPtrAllocator &Allocator,
                               MachineFunction &DestMF,
                               const DenseMap<MachineBasicBlock *,
                               MachineBasicBlock *> &Src2DstMBB) const override
    {
        return DestMF.cloneInfo<Emex64MachineFunctionInfo>(*this);
    }
    
    int getVarArgsFrameIndex() const
    {
        return VarArgsFrameIndex;
    }

    void setVarArgsFrameIndex(int FI)
    {
        VarArgsFrameIndex = FI;
    }
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64MACHINEFUNCTIONINFO_H */
