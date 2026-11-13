//===-- Emex64SelectionDAGInfo.h --------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64SELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64SELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

#define GET_SDNODE_ENUM
#include "Emex64GenSDNodeInfo.inc"

namespace llvm {

class Emex64SelectionDAGInfo : public SelectionDAGGenTargetInfo
{
public:
    Emex64SelectionDAGInfo();

    SDValue EmitTargetCodeForMemcpy(SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src, SDValue Size, Align DstAlign, Align SrcAlign, bool IsVolatile, bool AlwaysInline, MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo) const override;
    SDValue EmitTargetCodeForMemmove(SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src, SDValue Size, Align DstAlign, Align SrcAlign, bool IsVolatile, MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo) const override;
    SDValue EmitTargetCodeForMemset(SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst, SDValue Src, SDValue Size, Align Alignment, bool IsVolatile, bool AlwaysInline, MachinePointerInfo DstPtrInfo) const override;
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64SELECTIONDAGINFO_H */
