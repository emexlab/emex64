//===-- Emex64TargetInfo.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/Emex64TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

Target &llvm::getTheEmex64Target()
{
    static Target TheEmex64Target;
    return TheEmex64Target;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeEmex64TargetInfo()
{
    RegisterTarget<Triple::emex64, false> X( getTheEmex64Target(), "emex64", "emexlab emex64 (64-bit)", "Emex64");
}
