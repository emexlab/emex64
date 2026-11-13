//===-- Emex64SelectionDAGInfo.cpp ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64SelectionDAGInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"

#define GET_SDNODE_DESC
#include "Emex64GenSDNodeInfo.inc"

using namespace llvm;

Emex64SelectionDAGInfo::Emex64SelectionDAGInfo() : SelectionDAGGenTargetInfo(Emex64GenSDNodeInfo) {}

static bool leaveToGeneric(SDValue Size)
{
    auto *C = dyn_cast<ConstantSDNode>(Size);
    return C && C->getZExtValue() <= 16;
}

static SDValue asQuad(SelectionDAG &DAG,
                      const SDLoc &DL,
                      SDValue V)
{
    if(V.getValueType() == MVT::i64)
    {
        return V;
    }
    return DAG.getZExtOrTrunc(V, DL, MVT::i64);
}

SDValue Emex64SelectionDAGInfo::EmitTargetCodeForMemcpy(SelectionDAG &DAG,
                                                        const SDLoc &DL,
                                                        SDValue Chain,
                                                        SDValue Dst,
                                                        SDValue Src,
                                                        SDValue Size,
                                                        Align DstAlign,
                                                        Align SrcAlign,
                                                        bool IsVolatile,
                                                        bool AlwaysInline,
                                                        MachinePointerInfo DstPtrInfo,
                                                        MachinePointerInfo SrcPtrInfo) const
{
    if(leaveToGeneric(Size))
    {
        return SDValue();
    }
    return DAG.getNode(Emex64ISD::CPYM, DL, MVT::Other, Chain, Dst, Src, asQuad(DAG, DL, Size));
}

SDValue Emex64SelectionDAGInfo::EmitTargetCodeForMemmove(SelectionDAG &DAG,
                                                         const SDLoc &DL,
                                                         SDValue Chain,
                                                         SDValue Dst,
                                                         SDValue Src,
                                                         SDValue Size,
                                                         Align DstAlign,
                                                         Align SrcAlign,
                                                         bool IsVolatile,
                                                         MachinePointerInfo DstPtrInfo,
                                                         MachinePointerInfo SrcPtrInfo) const
{
    if(leaveToGeneric(Size))
    {
        return SDValue();
    }
    return DAG.getNode(Emex64ISD::MOVM, DL, MVT::Other, Chain, Dst, Src, asQuad(DAG, DL, Size));
}

SDValue Emex64SelectionDAGInfo::EmitTargetCodeForMemset(SelectionDAG &DAG,
                                                        const SDLoc &DL,
                                                        SDValue Chain,
                                                        SDValue Dst,
                                                        SDValue Src,
                                                        SDValue Size,
                                                        Align Alignment,
                                                        bool IsVolatile,
                                                        bool AlwaysInline,
                                                        MachinePointerInfo DstPtrInfo) const
{
    if(leaveToGeneric(Size))
    {
        return SDValue();
    }
    return DAG.getNode(Emex64ISD::SETM, DL, MVT::Other, Chain, Dst, asQuad(DAG, DL, Src), asQuad(DAG, DL, Size));
}
