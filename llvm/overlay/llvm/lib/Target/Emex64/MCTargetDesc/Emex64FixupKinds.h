//===-- Emex64FixupKinds.h --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64FIXUPKINDS_H
#define LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace Emex64 {

enum Fixups {
    fixup_emex64_pcrel8_b0 = FirstTargetFixupKind,
    fixup_emex64_pcrel8_b4,
    fixup_emex64_pcrel16_b0,
    fixup_emex64_pcrel16_b4,
    fixup_emex64_pcrel32_b0,
    fixup_emex64_pcrel32_b4,

    LastTargetFixupKind,
    NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

inline unsigned getBranchFixupBits(unsigned Kind)
{
    switch(Kind)
    {
        case fixup_emex64_pcrel8_b0:
        case fixup_emex64_pcrel8_b4:
            return 8;
        case fixup_emex64_pcrel16_b0:
        case fixup_emex64_pcrel16_b4:
            return 16;
        case fixup_emex64_pcrel32_b0:
        case fixup_emex64_pcrel32_b4:
            return 32;
        default:
            return 0;
    }
}
inline bool isBranchFixupB4(unsigned Kind)
{
    return Kind == fixup_emex64_pcrel8_b4 || Kind == fixup_emex64_pcrel16_b4 || Kind == fixup_emex64_pcrel32_b4;
}

} /* namespace (Emex64) */
} /* namespace (llvm) */

#endif /* LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64FIXUPKINDS_H */
