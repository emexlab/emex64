//===-- Emex64ISelDAGToDAG.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64.h"
#include "Emex64Subtarget.h"
#include "Emex64TargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "emex64-isel"
#define PASS_NAME "emex64 DAG->DAG Pattern Instruction Selection"

namespace {

class Emex64DAGToDAGISel : public SelectionDAGISel
{
    const Emex64Subtarget *Subtarget = nullptr;

public:
    Emex64DAGToDAGISel() = delete;
    explicit Emex64DAGToDAGISel(Emex64TargetMachine &TM, CodeGenOptLevel OL) : SelectionDAGISel(TM, OL) {}

    bool runOnMachineFunction(MachineFunction &MF) override
    {
        Subtarget = &MF.getSubtarget<Emex64Subtarget>();
        return SelectionDAGISel::runOnMachineFunction(MF);
    }

    void Select(SDNode *N) override;

    bool SelectAddr(SDValue Addr, SDValue &Base, SDValue &Offset);
    bool SelectAddrRR(SDValue Addr, SDValue &Base, SDValue &Index);

    bool SelectInlineAsmMemoryOperand(const SDValue &Op, InlineAsm::ConstraintCode ConstraintID, std::vector<SDValue> &OutOps) override;

#include "Emex64GenDAGISel.inc"
};

class Emex64DAGToDAGISelLegacy : public SelectionDAGISelLegacy
{
public:
    static char ID;
    explicit Emex64DAGToDAGISelLegacy(Emex64TargetMachine &TM, CodeGenOptLevel OL) : SelectionDAGISelLegacy(ID, std::make_unique<Emex64DAGToDAGISel>(TM, OL)) {}
};

} /* namespace */

char Emex64DAGToDAGISelLegacy::ID = 0;

INITIALIZE_PASS(Emex64DAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

static bool isSymbol(SDValue V)
{
    if(V.getOpcode() != Emex64ISD::WRAPPER)
    {
        return false;
    }
    unsigned Opc = V.getOperand(0).getOpcode();
    return Opc == ISD::TargetGlobalAddress || Opc == ISD::TargetGlobalTLSAddress || Opc == ISD::TargetExternalSymbol || Opc == ISD::TargetBlockAddress || Opc == ISD::TargetJumpTable || Opc == ISD::TargetConstantPool;
}

static bool isPCSymbol(SDValue V)
{
    return V.getOpcode() == Emex64ISD::PCWRAPPER;
}

static SDValue symbolPlus(SelectionDAG &DAG,
                          SDValue W,
                          int64_t Off)
{
    SDValue S = W.getOperand(0);
    SDLoc DL(W);
    if(Off == 0)
    {
        return S;
    }
    if(auto *G = dyn_cast<GlobalAddressSDNode>(S))
    {
        unsigned F = G->getTargetFlags();
        if(F != Emex64II::MO_NO_FLAG && F != Emex64II::MO_PCREL)
        {
            return SDValue();
        }
        return DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i64, G->getOffset() + Off, F);
    }
    if(isa<ExternalSymbolSDNode>(S))
    {
        return SDValue();
    }
    if(auto *B = dyn_cast<BlockAddressSDNode>(S))
    {
        return DAG.getTargetBlockAddress(B->getBlockAddress(), MVT::i64, B->getOffset() + Off, B->getTargetFlags());
    }
    if(auto *C = dyn_cast<ConstantPoolSDNode>(S))
    {
        return DAG.getTargetConstantPool(C->getConstVal(), MVT::i64, C->getAlign(), C->getOffset() + Off, C->getTargetFlags());
    }
    return SDValue();
}

bool Emex64DAGToDAGISel::SelectAddr(SDValue Addr,
                                    SDValue &Base,
                                    SDValue &Offset)
{
    SDLoc DL(Addr);
    SDValue NoReg = CurDAG->getRegister(Register(), MVT::i64);
    
    if(auto *FI = dyn_cast<FrameIndexSDNode>(Addr))
    {
        Base = CurDAG->getTargetFrameIndex(FI->getIndex(), MVT::i64);
        Offset = CurDAG->getTargetConstant(0, DL, MVT::i64);
        return true;
    }
    
    if(isSymbol(Addr))
    {
        Base = NoReg;
        Offset = Addr.getOperand(0);
        return true;
    }
    
    if(isPCSymbol(Addr))
    {
        Base = CurDAG->getRegister(Emex64::PC, MVT::i64);
        Offset = Addr.getOperand(0);
        return true;
    }
    
    if(auto *C = dyn_cast<ConstantSDNode>(Addr))
    {
        Base = NoReg;
        Offset = CurDAG->getTargetConstant(C->getZExtValue(), DL, MVT::i64);
        return true;
    }
    
    if(CurDAG->isBaseWithConstantOffset(Addr))
    {
        SDValue B = Addr.getOperand(0);
        int64_t Off = cast<ConstantSDNode>(Addr.getOperand(1))->getSExtValue();
        if(auto *FI = dyn_cast<FrameIndexSDNode>(B))
        {
            Base = CurDAG->getTargetFrameIndex(FI->getIndex(), MVT::i64);
            Offset = CurDAG->getSignedTargetConstant(Off, DL, MVT::i64);
            return true;
        }
        if(isSymbol(B) || isPCSymbol(B))
        {
            SDValue S = symbolPlus(*CurDAG, B, Off);
            if(S)
            {
                Base = isPCSymbol(B) ? CurDAG->getRegister(Emex64::PC, MVT::i64) : NoReg;
                Offset = S;
                return true;
            }
        }
        Base = B;
        Offset = CurDAG->getSignedTargetConstant(Off, DL, MVT::i64);
        return true;
    }
    
    if(Addr.getOpcode() == ISD::ADD)
    {
        SDValue A = Addr.getOperand(0), B = Addr.getOperand(1);
        if(isSymbol(A))
        {
            std::swap(A, B);
        }
        if(isSymbol(B) && A.getOpcode() != ISD::FrameIndex)
        {
            Base = A;
            Offset = B.getOperand(0);
            return true;
        }
    }
    
    Base = Addr;
    Offset = CurDAG->getTargetConstant(0, DL, MVT::i64);
    return true;
}

bool Emex64DAGToDAGISel::SelectAddrRR(SDValue Addr,
                                      SDValue &Base,
                                      SDValue &Index)
{
    if(Addr.getOpcode() != ISD::ADD)
    {
        return false;
    }
    SDValue A = Addr.getOperand(0), B = Addr.getOperand(1);
    auto isSimple = [](SDValue V)
    {
        return V.getOpcode() == ISD::Constant || V.getOpcode() == ISD::FrameIndex || isSymbol(V) || isPCSymbol(V);
    };
    if(isSimple(A) || isSimple(B))
    {
        return false;
    }
    Base = A;
    Index = B;
    return true;
}

bool Emex64DAGToDAGISel::SelectInlineAsmMemoryOperand(const SDValue &Op,
                                                      InlineAsm::ConstraintCode ConstraintID,
                                                      std::vector<SDValue> &OutOps)
{
    SDValue Base, Offset;
    switch(ConstraintID)
    {
        case InlineAsm::ConstraintCode::m:
        case InlineAsm::ConstraintCode::o:
            SelectAddr(Op, Base, Offset);
            OutOps.push_back(Base);
            OutOps.push_back(Offset);
            return false;
        default:
            return true;
    }
}

void Emex64DAGToDAGISel::Select(SDNode *N)
{
    if(N->isMachineOpcode())
    {
        N->setNodeId(-1);
        return;
    }
    SDLoc DL(N);
    
    switch(N->getOpcode())
    {
        case ISD::FrameIndex:
        {
            int FI = cast<FrameIndexSDNode>(N)->getIndex();
            SDValue TFI = CurDAG->getTargetFrameIndex(FI, MVT::i64);
            CurDAG->SelectNodeTo(N, Emex64::MOVrm, MVT::i64, TFI, CurDAG->getTargetConstant(0, DL, MVT::i64));
            return;
        }
        case ISD::ADD:
        {
            SDValue Base, Offset;
            SDValue A = N->getOperand(0), B = N->getOperand(1);
            if((A.getOpcode() == ISD::FrameIndex && isa<ConstantSDNode>(B)))
            {
                SelectAddr(SDValue(N, 0), Base, Offset);
                CurDAG->SelectNodeTo(N, Emex64::MOVrm, MVT::i64, Base, Offset);
                return;
            }
            break;
        }
        default:
            break;
    }

    SelectCode(N);
}

FunctionPass *llvm::createEmex64ISelDag(Emex64TargetMachine &TM,
                                        CodeGenOptLevel OptLevel)
{
    return new Emex64DAGToDAGISelLegacy(TM, OptLevel);
}
