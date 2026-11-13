//===-- Emex64FrameLowering.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64FRAMELOWERING_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class Emex64Subtarget;

class Emex64FrameLowering : public TargetFrameLowering
{
    const Emex64Subtarget &STI;

public:
    explicit Emex64FrameLowering(const Emex64Subtarget &STI): TargetFrameLowering(StackGrowsDown, Align(8), 0, Align(8)), STI(STI) {}

    void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
    void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

    MachineBasicBlock::iterator eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const override;
    bool hasReservedCallFrame(const MachineFunction &MF) const override;
    StackOffset getFrameIndexReference(const MachineFunction &MF, int FI, Register &FrameReg) const override;

protected:
    bool hasFPImpl(const MachineFunction &MF) const override
    {
        return true;
    }
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64FRAMELOWERING_H */
