//===-- Emex64.h ------------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_EMEX64_H
#define LLVM_LIB_TARGET_EMEX64_EMEX64_H

#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "llvm/Support/CodeGen.h"

namespace llvm {

class Emex64TargetMachine;
class FunctionPass;
class PassRegistry;

FunctionPass *createEmex64ISelDag(Emex64TargetMachine &TM, CodeGenOptLevel OptLevel);

void initializeEmex64AsmPrinterPass(PassRegistry &);
void initializeEmex64DAGToDAGISelLegacyPass(PassRegistry &);

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_EMEX64_H */
