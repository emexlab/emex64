//===-- Emex64InstrInfo.h ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64INSTRINFO_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64INSTRINFO_H

#include "Emex64RegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "Emex64GenInstrInfo.inc"

namespace llvm {

class Emex64Subtarget;

class Emex64InstrInfo : public Emex64GenInstrInfo
{
    const Emex64RegisterInfo RI;
    const Emex64Subtarget &Subtarget;

public:
    explicit Emex64InstrInfo(const Emex64Subtarget &STI);
    const Emex64RegisterInfo &getRegisterInfo() const
    {
        return RI;
    }
    
    void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, const DebugLoc &DL, Register DestReg, Register SrcReg, bool KillSrc, bool RenamableDest = false, bool RenamableSrc = false) const override;
    void storeRegToStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register SrcReg, bool IsKill, int FI, const TargetRegisterClass *RC, Register VReg, MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
    void loadRegFromStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register DestReg, int FI, const TargetRegisterClass *RC, Register VReg, unsigned SubReg = 0, MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
    
    Register isLoadFromStackSlot(const MachineInstr &MI, int &FrameIndex) const override;
    Register isStoreToStackSlot(const MachineInstr &MI, int &FrameIndex) const override;

    bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB, MachineBasicBlock *&FBB, SmallVectorImpl<MachineOperand> &Cond, bool AllowModify) const override;
    unsigned removeBranch(MachineBasicBlock &MBB, int *BytesRemoved = nullptr) const override;
    unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond, const DebugLoc &DL, int *BytesAdded = nullptr) const override;
    bool reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;
    unsigned getInstSizeInBytes(const MachineInstr &MI) const override;
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64INSTRINFO_H */
