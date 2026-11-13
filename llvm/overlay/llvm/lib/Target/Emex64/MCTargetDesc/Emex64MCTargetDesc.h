//===-- Emex64MCTargetDesc.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64MCTARGETDESC_H
#define LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64MCTARGETDESC_H

#include "llvm/MC/MCInstrDesc.h"
#include <cstdint>
#include <memory>

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCExpr;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;

MCCodeEmitter *createEmex64MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx);
MCAsmBackend *createEmex64AsmBackend(const Target &T, const MCSubtargetInfo &STI, const MCRegisterInfo &MRI, const MCTargetOptions &Options);
std::unique_ptr<MCObjectTargetWriter> createEmex64ELFObjectWriter(uint8_t OSABI);

namespace Emex64Op {

enum OperandType : unsigned {
  OPERAND_CALLTARGET = MCOI::OPERAND_FIRST_TARGET,
};

} /* Emex64Op (namespace) */

namespace Emex64S {

enum : uint16_t {
    None = 0,
    PCREL = 1,      /* sym@pcrel */
    PLT = 2,        /* sym@plt */
    GOTPCREL = 3,   /* sym@gotpcrel */
    TPOFF = 4,      /* sym@tpoff */
    GOTTPOFF = 5,   /* sym@gottpoff */
    TLSGD = 6,      /* sym@tlsgd */
    DTPOFF = 7,     /* sym@dtpoff */
};

inline bool isPCRelative(unsigned S)
{
    return S == PCREL || S == PLT || S == GOTPCREL || S == GOTTPOFF || S == TLSGD;
}

} /* Emex64S (namespace) */

uint16_t getEmex64Specifier(const MCExpr *E);
const MCExpr *setEmex64Specifier(const MCExpr *E, uint16_t S, MCContext &Ctx);

namespace Emex64II {

inline unsigned getOpcodeByte(uint64_t TSFlags)
{
    return TSFlags & 0xFF;
}

inline unsigned getMaxArgs(uint64_t TSFlags)
{
    return (TSFlags >> 8) & 0x3F;
}

enum TOF : unsigned {
    MO_NO_FLAG = 0,
    MO_PCREL = 1,
    MO_PLT = 2,
    MO_GOTPCREL = 3,
    MO_TPOFF = 4,
    MO_GOTTPOFF = 5,
    MO_TLSGD = 6,
};

} /* Emex64II (namespace) */

namespace Emex64Coding {

enum : unsigned {
    End = 0,
    Reg = 1,
    Imm4 = 2,
    Imm8 = 3,
    Imm16 = 4,
    Imm32 = 5,
    Imm64 = 6,
    Addr64 = 7,
    RegExtended = 8,
    RegInc = 9,
    RegDec = 10,
    OffsetAdd = 11,
    OffsetSub = 12,
    RegFloat = 13,
};

} /* Emex64Coding (namespace) */

namespace Emex64CC {

enum : unsigned {
    Z = 1,
    L = 2,
    G = 4,
    U = 8,
    NZ = 16,

    EQ = NZ | Z,
    NE = Z,
    LT = NZ | L,
    GT = NZ | G,
    LE = NZ | L | Z,
    GE = NZ | G | Z,
    UN = NZ | U,
    ORD = U,
    UEQ = NZ | U | Z,
    ULT = NZ | U | L,
    UGT = NZ | U | G,
    ULE = NZ | U | L | Z,
    UGE = NZ | U | G | Z,
    ONE = NZ | L | G,
};

inline unsigned invert(unsigned CC)
{
    return CC ^ NZ;
}

} /* Emex64CC (namespace) */

} /* llvm (namespace) */

#define GET_REGINFO_ENUM
#include "Emex64GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "Emex64GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "Emex64GenSubtargetInfo.inc"

#endif /* LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64MCTARGETDESC_H */
