//===-- Emex64RegisterInfo.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64RegisterInfo.h"
#include "Emex64.h"
#include "Emex64FrameLowering.h"
#include "Emex64Subtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/Function.h"

#define GET_REGINFO_TARGET_DESC
#include "Emex64GenRegisterInfo.inc"

using namespace llvm;

Emex64RegisterInfo::Emex64RegisterInfo() : Emex64GenRegisterInfo(Emex64::PC) {}

const MCPhysReg *Emex64RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const
{
    if(MF && MF->getFunction().hasFnAttribute("interrupt"))
    {
        return CSR_Emex64_AllRegs_SaveList;
    }
    return CSR_Emex64_SaveList;
}

const uint32_t *Emex64RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                                         CallingConv::ID CC) const
{
    return CSR_Emex64_RegMask;
}

BitVector Emex64RegisterInfo::getReservedRegs(const MachineFunction &MF) const
{
    BitVector Reserved(getNumRegs());
    Reserved.set(Emex64::PC);
    Reserved.set(Emex64::SP);
    Reserved.set(Emex64::FP);
    Reserved.set(Emex64::CF);
    Reserved.set(Emex64::FPC);
    Reserved.set(Emex64::ER15);
    return Reserved;
}

bool Emex64RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                             int SPAdj,
                                             unsigned FIOperandNum,
                                             RegScavenger *RS) const
{
    MachineInstr &MI = *II;
    MachineFunction &MF = *MI.getParent()->getParent();
    const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
    int FI = MI.getOperand(FIOperandNum).getIndex();
    
    Register FrameReg;
    int64_t Offset = TFI->getFrameIndexReference(MF, FI, FrameReg).getFixed();
    MachineOperand &OffMO = MI.getOperand(FIOperandNum + 1);
    assert(OffMO.isImm() && "a frame index must be followed by an offset");
    Offset += OffMO.getImm();
    
    MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
    OffMO.setImm(Offset);
    return false;
}

Register Emex64RegisterInfo::getFrameRegister(const MachineFunction &MF) const
{
    return Emex64::FP;
}
