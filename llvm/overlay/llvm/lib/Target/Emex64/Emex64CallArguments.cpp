//===-- Emex64CallArguments.cpp -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64.h"
#include "Emex64Subtarget.h"
#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"

using namespace llvm;

#define DEBUG_TYPE "emex64-call-arguments"

namespace {

class Emex64CallArguments : public MachineFunctionPass
{
public:
    static char ID;

    Emex64CallArguments() : MachineFunctionPass(ID) {}

    StringRef getPassName() const override
    {
        return "emex64 call arguments";
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

private:
    bool foldInto(MachineInstr &Call);
};

} /* namespace */

char Emex64CallArguments::ID = 0;

static const MCRegister ArgRegs[Emex64Opc::CallArgs] = {
    Emex64::R0, Emex64::R1, Emex64::R2, Emex64::R3, Emex64::R4, Emex64::R5
};

static int argumentOf(Register Reg)
{
    for(unsigned I = 0; I < Emex64Opc::CallArgs; ++I)
    {
        if(Reg == ArgRegs[I])
        {
            return I;
        }
    }
    return -1;
}

static bool isPlainValue(const MachineOperand &MO)
{
    if(MO.isImm())
    {
        return true;
    }
    return (MO.isGlobal() || MO.isSymbol() || MO.isBlockAddress() || MO.isJTI() || MO.isCPI()) && MO.getTargetFlags() == Emex64II::MO_NO_FLAG;
}

static int foldableMove(const MachineInstr &MI,
                        SmallVectorImpl<Register> &Reads)
{
    switch(MI.getOpcode())
    {
        case Emex64::MOVrr:
            if(!MI.getOperand(1).isReg())
            {
                return -1;
            }
            Reads.push_back(MI.getOperand(1).getReg());
            break;
        case Emex64::MOVri:
            if(!isPlainValue(MI.getOperand(1)))
            {
                return -1;
            }
            break;
        case Emex64::MOVrm:
        case Emex64::MOVrmm:
        {
            const MachineOperand &Base = MI.getOperand(1);
            const MachineOperand &Off = MI.getOperand(2);
            if(!Base.isReg() || !(Off.isReg() || isPlainValue(Off)))
            {
                return -1;
            }
            if(Base.getReg())
            {
                Reads.push_back(Base.getReg());
            }
            if(Off.isReg())
            {
                Reads.push_back(Off.getReg());
            }
            break;
        }
        default:
            return -1;
    }
    if(!MI.getOperand(0).isReg())
    {
        return -1;
    }
    return argumentOf(MI.getOperand(0).getReg());
}

static MachineOperand valueKind(int64_t Kind)
{
    return MachineOperand::CreateImm(Kind);
}

static MachineOperand use(Register Reg)
{
    return MachineOperand::CreateReg(Reg, false);
}

static void appendValue(MachineInstr &Call,
                        MachineFunction &MF,
                        const MachineInstr &Move)
{
    if(Move.getOpcode() == Emex64::MOVrr)
    {
        Call.addOperand(MF, valueKind(Emex64Raw::Value));
        Call.addOperand(MF, use(Move.getOperand(1).getReg()));
        return;
    }
    if(Move.getOpcode() == Emex64::MOVri)
    {
        Call.addOperand(MF, valueKind(Emex64Raw::Value));
        Call.addOperand(MF, Move.getOperand(1));
        return;
    }

    const MachineOperand &Base = Move.getOperand(1);
    const MachineOperand &Off = Move.getOperand(2);
    if(!Base.getReg())
    {
        Call.addOperand(MF, valueKind(Emex64Raw::Value));
        Call.addOperand(MF, Off);
        return;
    }
    if(Off.isReg())
    {
        Call.addOperand(MF, valueKind(Emex64Raw::OffsetAdd));
        Call.addOperand(MF, use(Base.getReg()));
        Call.addOperand(MF, use(Off.getReg()));
        return;
    }
    if(Off.isImm())
    {
        int64_t V = Off.getImm();
        if(V == 0)
        {
            Call.addOperand(MF, valueKind(Emex64Raw::Value));
            Call.addOperand(MF, use(Base.getReg()));
            return;
        }
        Call.addOperand(MF, valueKind(V < 0 ? Emex64Raw::OffsetSub : Emex64Raw::OffsetAdd));
        Call.addOperand(MF, use(Base.getReg()));
        Call.addOperand(MF, MachineOperand::CreateImm(V < 0 ? (int64_t)(0 - (uint64_t)V) : V));
        return;
    }
    Call.addOperand(MF, valueKind(Emex64Raw::OffsetAdd));
    Call.addOperand(MF, use(Base.getReg()));
    Call.addOperand(MF, Off);
}

bool Emex64CallArguments::foldInto(MachineInstr &Call)
{
    MachineBasicBlock &MBB = *Call.getParent();
    MachineFunction &MF = *MBB.getParent();

    for(unsigned I = 1, E = Call.getNumOperands(); I < E; ++I)
    {
        if(Call.getOperand(I).isImm())
        {
            return false;
        }
    }

    Register Target;
    if(Call.getOpcode() == Emex64::BLr)
    {
        Target = Call.getOperand(0).getReg();
    }

    MachineInstr *Moves[Emex64Opc::CallArgs] = {};
    SmallSet<Register, 8> ReadBehind;
    unsigned Folded = 0;
    int Last = -1;

    for(MachineBasicBlock::iterator I = Call.getIterator(); I != MBB.begin();)
    {
        --I;
        if(I->isDebugInstr())
        {
            continue;
        }

        SmallVector<Register, 2> Reads;
        int Arg = foldableMove(*I, Reads);
        if(Arg < 0 || Moves[Arg])
        {
            break;
        }
        Register Dst = ArgRegs[Arg];
        if(Dst == Target || ReadBehind.count(Dst))
        {
            break;
        }

        Moves[Arg] = &*I;
        ++Folded;
        Last = std::max(Last, Arg);
        for(Register R : Reads)
        {
            ReadBehind.insert(R);
        }
    }

    if(Folded == 0 || (unsigned)(Last + 1) - Folded > Folded)
    {
        return false;
    }

    for(int Arg = 0; Arg <= Last; ++Arg)
    {
        if(Moves[Arg])
        {
            appendValue(Call, MF, *Moves[Arg]);
        }
        else
        {
            Call.addOperand(MF, valueKind(Emex64Raw::Value));
            Call.addOperand(MF, use(ArgRegs[Arg]));
        }
    }
    for(MachineInstr *Move : Moves)
    {
        if(Move)
        {
            Move->eraseFromParent();
        }
    }
    return true;
}

bool Emex64CallArguments::runOnMachineFunction(MachineFunction &MF)
{
    if(!MF.getSubtarget<Emex64Subtarget>().hasISA22())
    {
        return false;
    }

    bool Changed = false;
    for(MachineBasicBlock &MBB : MF)
    {
        SmallVector<MachineInstr *, 8> Calls;
        for(MachineInstr &MI : MBB)
        {
            if(MI.getOpcode() == Emex64::BL || MI.getOpcode() == Emex64::BLr)
            {
                Calls.push_back(&MI);
            }
        }
        for(MachineInstr *Call : Calls)
        {
            Changed |= foldInto(*Call);
        }
    }
    return Changed;
}

FunctionPass *llvm::createEmex64CallArgumentsPass()
{
    return new Emex64CallArguments();
}
