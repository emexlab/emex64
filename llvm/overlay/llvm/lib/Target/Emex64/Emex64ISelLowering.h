//===-- Emex64ISelLowering.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64ISELLOWERING_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64ISELLOWERING_H

#include "Emex64.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class Emex64Subtarget;

class Emex64TargetLowering : public TargetLowering
{
    const Emex64Subtarget &Subtarget;

public:
    Emex64TargetLowering(const TargetMachine &TM, const Emex64Subtarget &STI);

    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
    void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results, SelectionDAG &DAG) const override;

    EVT getSetCCResultType(const DataLayout &DL,
                           LLVMContext &Context,
                           EVT VT) const override
    {
        return MVT::i64;
    }

    bool isFPImmLegal(const APFloat &Imm,
                      EVT VT,
                      bool ForCodeSize) const override
    {
        return true;
    }

    bool allowsMisalignedMemoryAccesses(EVT VT,
                                        unsigned AddrSpace,
                                        Align A,
                                        MachineMemOperand::Flags Flags,
                                        unsigned *Fast) const override
    {
        if(Fast)
        {
            *Fast = 1;
        }
        return true;
    }

    bool isFMAFasterThanFMulAndFAdd(const MachineFunction &MF, EVT VT) const override;

    bool shouldInsertFencesForAtomic(const Instruction *I) const override;
    AtomicExpansionKind shouldExpandAtomicRMWInIR(const AtomicRMWInst *AI) const override;
    MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr &MI, MachineBasicBlock *BB) const override;

    bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM, Type *Ty, unsigned AS, Instruction *I = nullptr) const override;

    ConstraintType getConstraintType(StringRef Constraint) const override;
    std::pair<unsigned, const TargetRegisterClass *> getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI, StringRef Constraint, MVT VT) const override;
    Register getRegisterByName(const char *RegName, LLT VT, const MachineFunction &MF) const override;

    Register getExceptionPointerRegister(const Constant *PersonalityFn) const override
    {
        return Emex64::R0;
    }
    Register getExceptionSelectorRegister(const Constant *PersonalityFn) const override
    {
        return Emex64::R1;
    }

private:
    SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg, const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL, SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const override;
    SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const override;
    bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context, const Type *RetTy) const override;
    SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs, const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL, SelectionDAG &DAG) const override;

    SDValue lowerAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerVASTART(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerBITCAST(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;

    SDValue emitCompare(SDValue LHS, SDValue RHS, ISD::CondCode CC, unsigned &E64CC, const SDLoc &DL, SelectionDAG &DAG) const;
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64ISELLOWERING_H */
