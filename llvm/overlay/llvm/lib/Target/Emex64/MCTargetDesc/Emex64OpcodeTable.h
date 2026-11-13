//===-- Emex64OpcodeTable.h -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64OPCODETABLE_H
#define LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64OPCODETABLE_H

#include "llvm/ADT/StringRef.h"
#include <cstdint>

namespace llvm {

namespace Emex64Raw {

enum Kind : int64_t {
    Value = 0,
    OffsetAdd = 1,
    OffsetSub = 2,
    RegInc = 3,
    RegDec = 4,
    PCRel = 5,
    PCRel8 = 6,
    PCRel16 = 7,
};

inline bool isPCRelKind(int64_t K)
{
    return K == PCRel || K == PCRel8 || K == PCRel16;
}

inline unsigned getNumKindOperands(int64_t K)
{
    return (K == OffsetAdd || K == OffsetSub) ? 2 : 1;
}

} /* Emex64Raw (namespace) */

struct Emex64OpcodeInfo {
    const char *Name;
    uint8_t MaxArgs;
    int8_t TargetArg;
};

inline const Emex64OpcodeInfo &getEmex64OpcodeInfo(unsigned Op)
{
    static const Emex64OpcodeInfo Table[256] = {
        /* ISA v15 (and below) */
        {"hlt", 0, -1}, {"nop", 0, -1}, {"mov", 2, -1}, {"movz", 2, -1},
        {"swp", 2, -1}, {"push", 16, -1}, {"pop", 16, -1}, {"ldb", 2, -1},
        {"ldw", 2, -1}, {"ldd", 2, -1}, {"ldq", 2, -1}, {"stb", 2, -1},
        {"stw", 2, -1}, {"std", 2, -1}, {"stq", 2, -1}, {"add", 3, -1},
        {"sub", 3, -1}, {"mul", 3, -1}, {"div", 3, -1}, {"idiv", 3, -1},
        {"mod", 3, -1}, {"not", 16, -1}, {"neg", 16, -1}, {"and", 3, -1},
        {"or", 3, -1}, {"xor", 3, -1}, {"shr", 3, -1}, {"shl", 3, -1},
        {"sar", 3, -1}, {"ror", 3, -1}, {"rol", 3, -1}, {"pdep", 3, -1},
        {"pext", 3, -1}, {"bswapw", 1, -1}, {"bswapd", 1, -1}, {"bswapq", 1, -1},
        {"inc", 16, -1}, {"dec", 16, -1}, {"b", 1, 0}, {"cmp", 2, -1},
        {"be", 1, 0}, {"bne", 1, 0}, {"blt", 1, 0}, {"bgt", 1, 0},
        {"ble", 1, 0}, {"bge", 1, 0}, {"bz", 2, 1}, {"bnz", 2, 1},
        {"blw", 16, 0}, {"wret", 0, -1}, {"iret", 0, -1}, {"bl", 1, 0},
        {"ret", 0, -1}, {"clr", 16, -1}, {"cmov", 2, -1}, {"cmovb", 2, -1},
        {"clar", 0, -1}, {"bbz", 3, 2}, {"bbnz", 3, 2},
        
        /* ISA v16 */
        {"rdrnd", 1, -1},
        {"movm", 3, -1}, {"cpym", 3, -1}, {"setm", 3, -1}, {"svc", 0, -1},

        /* ISA v17 */
        {"eret", 0, -1}, {"cswp", 2, -1}, {"xchg", 2, -1}, {"cas", 3, -1},

        /* ISA v18 */
        {"fadd", 3, -1}, {"fsub", 3, -1}, {"fmul", 3, -1}, {"fdiv", 3, -1},
        {"fsqrt", 2, -1}, {"fmin", 3, -1}, {"fmax", 3, -1}, {"fmadd", 4, -1},
        {"fneg", 2, -1}, {"fabs", 2, -1}, {"fcmp", 2, -1}, {"fadds", 3, -1},
        {"fsubs", 3, -1}, {"fmuls", 3, -1}, {"fdivs", 3, -1}, {"fsqrts", 2, -1},
        {"fmins", 3, -1}, {"fmaxs", 3, -1}, {"fmadds", 4, -1}, {"fnegs", 2, -1},
        {"fabss", 2, -1}, {"fcmps", 2, -1}, {"fcvtds", 2, -1}, {"fcvtsd", 2, -1},
        {"fcvtdi", 2, -1}, {"fcvtdu", 2, -1}, {"fcvtid", 2, -1}, {"fcvtud", 2, -1},
        {"fcvtsi", 2, -1}, {"fcvtsu", 2, -1}, {"fcvtis", 2, -1}, {"fcvtus", 2, -1},

        /* ISA v19 */
        {"sext", 3, -1}, {"cblt", 3, 2}, {"clz", 2, -1}, {"ctz", 2, -1},
        {"popcnt", 2, -1}, {"mulh", 3, -1}, {"umulh", 3, -1}, {"adc", 3, -1},
        {"sbb", 3, -1}, {"pacia", 2, -1}, {"autia", 2, -1}, {"pacda", 2, -1},
        {"autda", 2, -1}, {"xpac", 2, -1}, {"pacret", 0, -1}, {"reta", 0, -1},

        /* ISA v20 */
        {"rdtime", 1, -1}, {"wfe", 0, -1}, {"sev", 0, -1}, {"pause", 0, -1},
        {"cbeq", 3, 2}, {"cbne", 3, 2}, {"cbge", 3, 2}, {"cbltu", 3, 2},
        {"cbgeu", 3, 2}, {"ldsb", 2, -1}, {"ldsw", 2, -1}, {"ldsd", 2, -1},
        {"ldadd", 3, -1}, {"ldand", 3, -1}, {"ldor", 3, -1}, {"ldxor", 3, -1},
        {"swpadd", 2, -1}, {"ldaddd", 3, -1}, {"ldandd", 3, -1}, {"ldord", 3, -1},
        {"ldxord", 3, -1}, {"swpaddd", 2, -1}, {"tlbi", 2, -1},
        
        /* ISA v21 */
        {"addd", 3, -1},
        {"iaddd", 3, -1}, {"subd", 3, -1}, {"isubd", 3, -1}, {"muld", 3, -1},
        {"imuld", 3, -1}, {"shld", 3, -1}, {"ishld", 3, -1}, {"shrd", 3, -1},
        {"sard", 3, -1}, {"divd", 3, -1}, {"idivd", 3, -1}, {"modd", 3, -1},
    };
    return Table[Op & 0xFF];
}

inline int getEmex64Opcode(StringRef Name)
{
    for(unsigned I = 0; I < 256; ++I)
    {
        const Emex64OpcodeInfo &Info = getEmex64OpcodeInfo(I);
        if(Info.Name && Name.equals_insensitive(Info.Name))
        {
            return I;
        }
    }
    return -1;
}

} /* llvm (namespace) */

#endif /* LLVM_LIB_TARGET_EMEX64_MCTARGETDESC_EMEX64OPCODETABLE_H */
