//===-- Emex64RegisterInfo.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64REGISTERINFO_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64REGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "Emex64GenRegisterInfo.inc"

namespace llvm {

struct Emex64RegisterInfo : public Emex64GenRegisterInfo
{
    Emex64RegisterInfo();

    const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
    const uint32_t *getCallPreservedMask(const MachineFunction &MF, CallingConv::ID CC) const override;
    BitVector getReservedRegs(const MachineFunction &MF) const override;
    bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj, unsigned FIOperandNum, RegScavenger *RS = nullptr) const override;
    Register getFrameRegister(const MachineFunction &MF) const override;

    const TargetRegisterClass *getPointerRegClass(unsigned Kind = 0) const override
    {
        return &Emex64::GPRRegClass;
    }
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64REGISTERINFO_H */
