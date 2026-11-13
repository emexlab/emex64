//===-- Emex64FrameLowering.cpp -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64FrameLowering.h"
#include "Emex64InstrInfo.h"
#include "Emex64Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDwarf.h"

using namespace llvm;

static bool signsReturnAddress(const MachineFunction &MF)
{
    if(!MF.getSubtarget<Emex64Subtarget>().hasPAuth())
    {
        return false;
    }
    const Function &F = MF.getFunction();
    if(!F.hasFnAttribute("sign-return-address"))
    {
        return false;
    }
    StringRef Scope = F.getFnAttribute("sign-return-address").getValueAsString();
    if(Scope == "all")
    {
        return true;
    }
    if(Scope != "non-leaf")
    {
        return false;
    }
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    return MFI.hasCalls() || MFI.getStackSize() != 0;
}

void Emex64FrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const
{
    MachineFrameInfo &MFI = MF.getFrameInfo();
    uint64_t StackSize = MFI.getStackSize();
    const Emex64InstrInfo &TII = *STI.getInstrInfo();
    MachineBasicBlock::iterator MBBI = MBB.begin();
    DebugLoc DL;

    if(signsReturnAddress(MF))
    {
        BuildMI(MBB, MBBI, DL, TII.get(Emex64::PACRET)).setMIFlag(MachineInstr::FrameSetup);
    }

    if(StackSize == 0)
    {
        return;
    }

    BuildMI(MBB, MBBI, DL, TII.get(Emex64::SUBri), Emex64::SP)
        .addReg(Emex64::SP)
        .addImm(StackSize + 8)
        .setMIFlag(MachineInstr::FrameSetup);
    
    const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();
    if(!MF.needsFrameMoves() || CSI.empty())
    {
        return;
    }

    SmallSet<int, 16> CSFrameIndices;
    for(const CalleeSavedInfo &CS : CSI)
    {
        CSFrameIndices.insert(CS.getFrameIdx());
    }

    int FI;
    while(MBBI != MBB.end() && TII.isStoreToStackSlot(*MBBI, FI) && CSFrameIndices.count(FI))
    {
        ++MBBI;
    }
    const MCRegisterInfo *MRI = MF.getContext().getRegisterInfo();
    for(const CalleeSavedInfo &CS : CSI)
    {
        int64_t Offset = MFI.getObjectOffset(CS.getFrameIdx()) - 16;
        unsigned CFIIndex = MF.addFrameInst(MCCFIInstruction::createOffset(nullptr, MRI->getDwarfRegNum(CS.getReg(), true), Offset));
        BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION)).addCFIIndex(CFIIndex).setMIFlag(MachineInstr::FrameSetup);
    }
}

void Emex64FrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const
{
    if(!signsReturnAddress(MF))
    {
        return;
    }

    MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
    if(MBBI == MBB.end() || MBBI->getOpcode() != Emex64::RET)
    {
        return;
    }

    const Emex64InstrInfo &TII = *STI.getInstrInfo();
    BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII.get(Emex64::RETA)).copyImplicitOps(*MBBI);
    MBB.erase(MBBI);
}

bool Emex64FrameLowering::hasReservedCallFrame(const MachineFunction &MF) const
{
    return !MF.getFrameInfo().hasVarSizedObjects();
}

MachineBasicBlock::iterator Emex64FrameLowering::eliminateCallFramePseudoInstr(MachineFunction &MF,
                                                                               MachineBasicBlock &MBB,
                                                                               MachineBasicBlock::iterator I) const
{
    if(!hasReservedCallFrame(MF))
    {
        const Emex64InstrInfo &TII = *STI.getInstrInfo();
        int64_t Amount = I->getOperand(0).getImm();
        if(Amount != 0)
        {
            Amount = alignTo(Amount, getStackAlign());
            unsigned Opc = I->getOpcode() == Emex64::ADJCALLSTACKDOWN ? Emex64::SUBri : Emex64::ADDri;
            BuildMI(MBB, I, I->getDebugLoc(), TII.get(Opc), Emex64::SP).addReg(Emex64::SP).addImm(Amount);
        }
    }
    return MBB.erase(I);
}

StackOffset Emex64FrameLowering::getFrameIndexReference(const MachineFunction &MF,
                                                        int FI,
                                                        Register &FrameReg) const
{
    FrameReg = Emex64::FP;
    return StackOffset::getFixed(MF.getFrameInfo().getObjectOffset(FI));
}
