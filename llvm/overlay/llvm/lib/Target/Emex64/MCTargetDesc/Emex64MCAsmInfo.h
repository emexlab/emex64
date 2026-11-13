//===-- Emex64MCAsmInfo.h - emex64 asm properties ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64MCASMINFO_H
#define LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class Emex64MCAsmInfo : public MCAsmInfoELF
{
    void anchor() override;

public:
    explicit Emex64MCAsmInfo(const Triple &TT, const MCTargetOptions &Options);
    bool isValidUnquotedName(StringRef Name) const override;
};

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64MCASMINFO_H */
