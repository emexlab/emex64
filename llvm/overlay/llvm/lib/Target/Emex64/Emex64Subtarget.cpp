//===-- Emex64Subtarget.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64Subtarget.h"

#define DEBUG_TYPE "emex64-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "Emex64GenSubtargetInfo.inc"

using namespace llvm;

Emex64Subtarget &Emex64Subtarget::initializeSubtargetDependencies(StringRef CPU,
                                                                  StringRef FS)
{
    if(CPU.empty())
    {
        CPU = "generic";
    }
    ParseSubtargetFeatures(CPU, CPU, FS);
    return *this;
}

Emex64Subtarget::Emex64Subtarget(const Triple &TT,
                                 StringRef CPU,
                                 StringRef FS,
                                 const TargetMachine &TM) : Emex64GenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS), InstrInfo(initializeSubtargetDependencies(CPU, FS)), FrameLowering(*this), TLInfo(TM, *this)
{
}
