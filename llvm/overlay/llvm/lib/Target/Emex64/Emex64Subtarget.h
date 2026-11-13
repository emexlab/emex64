//===-- Emex64Subtarget.h ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64SUBTARGET_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64SUBTARGET_H

#include "Emex64FrameLowering.h"
#include "Emex64ISelLowering.h"
#include "Emex64InstrInfo.h"
#include "Emex64SelectionDAGInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"

#define GET_SUBTARGETINFO_HEADER
#include "Emex64GenSubtargetInfo.inc"

namespace llvm {

class TargetMachine;

class Emex64Subtarget : public Emex64GenSubtargetInfo
{
    bool HasFPU = false;
    bool HasISA19 = false;
    bool HasISA20 = false;
    bool HasISA21 = false;
    bool HasPAuth = false;
    
    Emex64InstrInfo InstrInfo;
    Emex64FrameLowering FrameLowering;
    Emex64TargetLowering TLInfo;
    Emex64SelectionDAGInfo TSInfo;

    Emex64Subtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS);

public:
    Emex64Subtarget(const Triple &TT, StringRef CPU, StringRef FS, const TargetMachine &TM);

    void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

    bool hasFPU() const
    {
        return HasFPU;
    }

    bool hasISA19() const
    {
        return HasISA19;
    }

    bool hasISA20() const
    {
        return HasISA20;
    }

    bool hasISA21() const
    {
        return HasISA21;
    }

    bool hasPAuth() const
    {
        return HasPAuth;
    }
    
    const Emex64InstrInfo *getInstrInfo() const override
    {
        return &InstrInfo;
    }
    const Emex64FrameLowering *getFrameLowering() const override
    {
        return &FrameLowering;
    }
    const Emex64RegisterInfo *getRegisterInfo() const override
    {
        return &InstrInfo.getRegisterInfo();
    }
    const Emex64TargetLowering *getTargetLowering() const override
    {
        return &TLInfo;
    }
    const Emex64SelectionDAGInfo *getSelectionDAGInfo() const override
    {
        return &TSInfo;
    }
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64SUBTARGET_H */
