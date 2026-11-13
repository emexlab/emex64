//===-- Emex64InstrInfo.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64InstrInfo.h"
#include "Emex64.h"
#include "Emex64Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "Emex64GenInstrInfo.inc"

Emex64InstrInfo::Emex64InstrInfo(const Emex64Subtarget &STI) : Emex64GenInstrInfo(STI, RI, Emex64::ADJCALLSTACKDOWN, Emex64::ADJCALLSTACKUP), RI(), Subtarget(STI) {}

void Emex64InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator I,
                                  const DebugLoc &DL,
                                  Register DestReg,
                                  Register SrcReg,
                                  bool KillSrc,
                                  bool RenamableDest,
                                  bool RenamableSrc) const
{
    bool DstFP = Emex64::FPRRegClass.contains(DestReg);
    bool SrcFP = Emex64::FPRRegClass.contains(SrcReg);
    unsigned Opc = DstFP ? (SrcFP ? Emex64::FMOVff : Emex64::FMOVfr) : (SrcFP ? Emex64::FMOVrf : Emex64::MOVrr);
    BuildMI(MBB, I, DL, get(Opc), DestReg).addReg(SrcReg, getKillRegState(KillSrc));
}

void Emex64InstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator I,
                                          Register SrcReg,
                                          bool IsKill,
                                          int FI,
                                          const TargetRegisterClass *RC,
                                          Register VReg,
                                          MachineInstr::MIFlag Flags) const
{
    DebugLoc DL;
    if(I != MBB.end())
    {
        DL = I->getDebugLoc();
    }
    unsigned Opc = Emex64::FPRRegClass.hasSubClassEq(RC) ? Emex64::FSTQri : Emex64::STQri;
    BuildMI(MBB, I, DL, get(Opc)).addFrameIndex(FI).addImm(0).addReg(SrcReg, getKillRegState(IsKill)).setMIFlag(Flags);
}

void Emex64InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                           MachineBasicBlock::iterator I,
                                           Register DestReg,
                                           int FI,
                                           const TargetRegisterClass *RC,
                                           Register VReg,
                                           unsigned SubReg,
                                           MachineInstr::MIFlag Flags) const
{
    DebugLoc DL;
    if(I != MBB.end())
    {
        DL = I->getDebugLoc();
    }
    unsigned Opc = Emex64::FPRRegClass.hasSubClassEq(RC) ? Emex64::FLDQri : Emex64::LDQri;
    BuildMI(MBB, I, DL, get(Opc), DestReg).addFrameIndex(FI).addImm(0).setMIFlag(Flags);
}

Register Emex64InstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                              int &FrameIndex) const
{
    unsigned Opc = MI.getOpcode();
    if((Opc == Emex64::LDQri || Opc == Emex64::FLDQri) && MI.getOperand(1).isFI() && MI.getOperand(2).isImm() && MI.getOperand(2).getImm() == 0)
    {
        FrameIndex = MI.getOperand(1).getIndex();
        return MI.getOperand(0).getReg();
    }
    return Register();
}

Register Emex64InstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                             int &FrameIndex) const
{
    unsigned Opc = MI.getOpcode();
    if((Opc == Emex64::STQri || Opc == Emex64::FSTQri) && MI.getOperand(0).isFI() && MI.getOperand(1).isImm() && MI.getOperand(1).getImm() == 0 && MI.getOperand(2).isReg())
    {
        FrameIndex = MI.getOperand(0).getIndex();
        return MI.getOperand(2).getReg();
    }
    return Register();
}

static bool isCondBranch(unsigned Opc)
{
    switch(Opc)
    {
        case Emex64::BRCC:
        case Emex64::BZ:
        case Emex64::BNZ:
        case Emex64::BBZ:
        case Emex64::BBNZ:
        case Emex64::CBLTrr:
        case Emex64::CBLTri:
        case Emex64::CBLTir:
        case Emex64::CBEQrr:
        case Emex64::CBEQri:
        case Emex64::CBEQir:
        case Emex64::CBNErr:
        case Emex64::CBNEri:
        case Emex64::CBNEir:
        case Emex64::CBGErr:
        case Emex64::CBGEri:
        case Emex64::CBGEir:
        case Emex64::CBLTUrr:
        case Emex64::CBLTUri:
        case Emex64::CBLTUir:
        case Emex64::CBGEUrr:
        case Emex64::CBGEUri:
        case Emex64::CBGEUir:
            return true;
        default:
            return false;
    }
}

static unsigned getInverseCB(unsigned Opc)
{
    switch(Opc)
    {
        case Emex64::CBLTrr:
            return Emex64::CBGErr;
        case Emex64::CBLTri:
            return Emex64::CBGEri;
        case Emex64::CBLTir:
            return Emex64::CBGEir;
        case Emex64::CBGErr:
            return Emex64::CBLTrr;
        case Emex64::CBGEri:
            return Emex64::CBLTri;
        case Emex64::CBGEir:
            return Emex64::CBLTir;
        case Emex64::CBEQrr:
            return Emex64::CBNErr;
        case Emex64::CBEQri:
            return Emex64::CBNEri;
        case Emex64::CBEQir:
            return Emex64::CBNEir;
        case Emex64::CBNErr:
            return Emex64::CBEQrr;
        case Emex64::CBNEri:
            return Emex64::CBEQri;
        case Emex64::CBNEir:
            return Emex64::CBEQir;
        case Emex64::CBLTUrr:
            return Emex64::CBGEUrr;
        case Emex64::CBLTUri:
            return Emex64::CBGEUri;
        case Emex64::CBLTUir:
            return Emex64::CBGEUir;
        case Emex64::CBGEUrr:
            return Emex64::CBLTUrr;
        case Emex64::CBGEUri:
            return Emex64::CBLTUri;
        case Emex64::CBGEUir:
            return Emex64::CBLTUir;
        default:
            return 0;
    }
}

static MachineBasicBlock *getCondTarget(const MachineInstr &MI)
{
    return MI.getOperand(MI.getOpcode() == Emex64::BRCC ? 0 : (MI.getOpcode() == Emex64::BZ || MI.getOpcode() == Emex64::BNZ) ? 1 : 2).getMBB();
}

static void getCond(const MachineInstr &MI,
                    SmallVectorImpl<MachineOperand> &Cond)
{
    unsigned Opc = MI.getOpcode();
    Cond.push_back(MachineOperand::CreateImm(Opc));
    switch(Opc)
    {
        case Emex64::BRCC:
            Cond.push_back(MI.getOperand(1));
            break;
        case Emex64::BZ:
        case Emex64::BNZ:
            Cond.push_back(MI.getOperand(0));
            break;
        default:
            Cond.push_back(MI.getOperand(0));
            Cond.push_back(MI.getOperand(1));
            break;
    }
}

bool Emex64InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                    MachineBasicBlock *&TBB,
                                    MachineBasicBlock *&FBB,
                                    SmallVectorImpl<MachineOperand> &Cond,
                                    bool AllowModify) const
{
    MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
    if(I == MBB.end())
    {
        return false;
    }
    if(!isUnpredicatedTerminator(*I))
    {
        return false;
    }
    
    MachineInstr *Last = &*I;
    MachineInstr *SecondLast = nullptr;
    if(I != MBB.begin())
    {
        --I;
        while(I != MBB.begin() && I->isDebugInstr())
        {
            --I;
        }
        if(isUnpredicatedTerminator(*I))
        {
            SecondLast = &*I;
        }
        if(SecondLast && I != MBB.begin())
        {
            MachineBasicBlock::iterator J = std::prev(I);
            if(isUnpredicatedTerminator(*J))
            {
                return true;
            }
        }
    }
    
    unsigned LastOpc = Last->getOpcode();
    if(!SecondLast)
    {
        if(LastOpc == Emex64::B)
        {
            TBB = Last->getOperand(0).getMBB();
            return false;
        }
        if(isCondBranch(LastOpc))
        {
            TBB = getCondTarget(*Last);
            getCond(*Last, Cond);
            return false;
        }
        return true;
    }
    
    unsigned SecondOpc = SecondLast->getOpcode();
    if(isCondBranch(SecondOpc) && LastOpc == Emex64::B)
    {
        TBB = getCondTarget(*SecondLast);
        getCond(*SecondLast, Cond);
        FBB = Last->getOperand(0).getMBB();
        return false;
    }
    if(SecondOpc == Emex64::B && LastOpc == Emex64::B)
    {
        TBB = SecondLast->getOperand(0).getMBB();
        if(AllowModify)
        {
            Last->eraseFromParent();
        }
        return false;
    }
    return true;
}

unsigned Emex64InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                       int *BytesRemoved) const
{
    unsigned Count = 0;
    MachineBasicBlock::iterator I = MBB.end();
    while(I != MBB.begin())
    {
        --I;
        if(I->isDebugInstr())
        {
            continue;
        }
        if(I->getOpcode() != Emex64::B && !isCondBranch(I->getOpcode()))
        {
            break;
        }
        I->eraseFromParent();
        I = MBB.end();
        ++Count;
    }
    if(BytesRemoved)
    {
        *BytesRemoved = 0;
    }
    return Count;
}

unsigned Emex64InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                       MachineBasicBlock *TBB,
                                       MachineBasicBlock *FBB,
                                       ArrayRef<MachineOperand> Cond,
                                       const DebugLoc &DL,
                                       int *BytesAdded) const
{
    assert(TBB && "insertBranch must not be told to insert a fallthrough");
    if(BytesAdded)
    {
        *BytesAdded = 0;
    }

    if(Cond.empty())
    {
        BuildMI(&MBB, DL, get(Emex64::B)).addMBB(TBB);
        return 1;
    }
    
    unsigned Opc = Cond[0].getImm();
    MachineInstrBuilder MIB = BuildMI(&MBB, DL, get(Opc));
    switch(Opc)
    {
        case Emex64::BRCC:
            MIB.addMBB(TBB).addImm(Cond[1].getImm());
            break;
        case Emex64::BZ:
        case Emex64::BNZ:
            MIB.add(Cond[1]).addMBB(TBB);
            break;
        default:
            MIB.add(Cond[1]).add(Cond[2]).addMBB(TBB);
            break;
    }

    if(!FBB)
    {
        return 1;
    }

    BuildMI(&MBB, DL, get(Emex64::B)).addMBB(FBB);
    return 2;
}

bool Emex64InstrInfo::reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const
{
    switch(Cond[0].getImm())
    {
        case Emex64::BRCC:
            Cond[1].setImm(Emex64CC::invert(Cond[1].getImm()));
            return false;
        case Emex64::BZ:
            Cond[0].setImm(Emex64::BNZ);
            return false;
        case Emex64::BNZ:
            Cond[0].setImm(Emex64::BZ);
            return false;
        case Emex64::BBZ:
            Cond[0].setImm(Emex64::BBNZ);
            return false;
        case Emex64::BBNZ:
            Cond[0].setImm(Emex64::BBZ);
            return false;
    }
    
    if(unsigned Inv = getInverseCB(Cond[0].getImm()))
    {
        if(Subtarget.hasISA20())
        {
            Cond[0].setImm(Inv);
            return false;
        }
    }
    
    return true;
}

unsigned Emex64InstrInfo::getInstSizeInBytes(const MachineInstr &MI) const
{
    if(MI.isInlineAsm())
    {
        const MachineFunction *MF = MI.getParent()->getParent();
        return getInlineAsmLength(MI.getOperand(0).getSymbolName(), MF->getTarget().getMCAsmInfo());
    }
    if(MI.isMetaInstruction() || MI.getOpcode() == Emex64::ADJCALLSTACKDOWN || MI.getOpcode() == Emex64::ADJCALLSTACKUP)
    {
        return 0;
    }
    if(MI.getOpcode() == Emex64::BRCC)
    {
        return 1 + 2 + 2 + 5;
    }
    unsigned Size = 1;
    for(const MachineOperand &MO : MI.explicit_operands())
    {
        Size += (MO.isReg() || MO.isFI()) ? 2 : 10;
    }
    return Size;
}
