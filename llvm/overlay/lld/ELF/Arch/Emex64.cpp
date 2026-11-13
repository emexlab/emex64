//===- Emex64.cpp ---------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "OutputSections.h"
#include "RelocScan.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {

class Emex64 final : public TargetInfo
{
public:
    Emex64(Ctx &);
    RelExpr getRelExpr(RelType type, const Symbol &s, const uint8_t *loc) const override;
    RelType getDynRel(RelType type) const override;
    int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
    void writeGotPlt(uint8_t *buf, const Symbol &s) const override;
    void writePlt(uint8_t *buf, const Symbol &sym, uint64_t pltEntryAddr) const override;
    void relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const override;
    template <class ELFT, class RelTy>
    void scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels, unsigned shard);

    void scanSection(InputSectionBase &sec,
                     unsigned shard) override
    {
        elf::scanSection1<Emex64, ELF64LE>(*this, sec, shard);
    }
};

} /* namespace */

Emex64::Emex64(Ctx &ctx) : TargetInfo(ctx)
{
    copyRel = R_EMEX64_COPY;
    gotRel = R_EMEX64_GLOB_DAT;
    pltRel = R_EMEX64_JUMP_SLOT;
    relativeRel = R_EMEX64_RELATIVE;
    symbolicRel = R_EMEX64_ABS64;
    tlsModuleIndexRel = R_EMEX64_DTPMOD64;
    tlsOffsetRel = R_EMEX64_DTPOFF64;
    tlsGotRel = R_EMEX64_TPOFF64;
    pltEntrySize = 16;
    ipltEntrySize = 16;
    pltHeaderSize = 0;
    gotPltHeaderEntriesNum = 0;
    defaultMaxPageSize = 8192;
    defaultCommonPageSize = 8192;
    defaultImageBase = 0x200000;
    trapInstr = {0x00, 0x00, 0x00, 0x00};
}

RelExpr Emex64::getRelExpr(RelType type,
                           const Symbol &s,
                           const uint8_t *loc) const
{
    switch(type)
    {
        case R_EMEX64_NONE:
            return R_NONE;
        case R_EMEX64_ABS64:
        case R_EMEX64_ABS32:
        case R_EMEX64_ABS16:
        case R_EMEX64_ABS8:
            return R_ABS;
        case R_EMEX64_PC64:
        case R_EMEX64_PC32:
        case R_EMEX64_BRANCH32:
        case R_EMEX64_BRANCH32_B4:
            return R_PC;
        case R_EMEX64_PLT32:
        case R_EMEX64_PLT32_B4:
            return R_PLT_PC;
        case R_EMEX64_PLT64:
            return R_PLT;
        case R_EMEX64_GOT64:
            return R_GOT;
        case R_EMEX64_GOTPC64:
            return R_GOTONLY_PC;
        case R_EMEX64_GOTPCREL64:
        case R_EMEX64_GOTTPOFF64:
            return R_GOT_PC;
        case R_EMEX64_TLSGD64:
            return R_TLSGD_PC;
        case R_EMEX64_TPOFF64:
            return R_TPREL;
        case R_EMEX64_DTPOFF64:
            return R_DTPREL;
        default:
            Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v << ") against symbol " << &s;
            return R_NONE;
    }
}

template <class ELFT, class RelTy> void Emex64::scanSectionImpl(InputSectionBase &sec,
                                                                Relocs<RelTy> rels,
                                                                unsigned shard)
{
    RelocScan rs(ctx, &sec, shard);
    sec.relocations.reserve(rels.size());
    
    for(auto it = rels.begin(); it != rels.end(); ++it)
    {
        const RelTy &rel = *it;
        uint32_t symIdx = rel.getSymbol(false);
        Symbol &sym = sec.getFile<ELFT>()->getSymbol(symIdx);
        uint64_t offset = rel.r_offset;
        RelType type = rel.getType(false);
        if(sym.isUndefined() && symIdx != 0 && rs.maybeReportUndefined(cast<Undefined>(sym), offset))
        {
            continue;
        }
        int64_t addend = rs.getAddend<ELFT>(rel, type);
        RelExpr expr;
        switch(type)
        {
            case R_EMEX64_NONE:
                continue;
            case R_EMEX64_ABS64:
            case R_EMEX64_ABS32:
            case R_EMEX64_ABS16:
            case R_EMEX64_ABS8:
                expr = R_ABS;
                break;
            case R_EMEX64_PC64:
            case R_EMEX64_PC32:
            case R_EMEX64_BRANCH32:
            case R_EMEX64_BRANCH32_B4:
                rs.processR_PC(type, offset, addend, sym);
                continue;
            case R_EMEX64_PLT32:
            case R_EMEX64_PLT32_B4:
                rs.processR_PLT_PC(type, offset, addend, sym);
                continue;
            case R_EMEX64_PLT64:
                expr = R_PLT;
                break;
            case R_EMEX64_GOT64:
                expr = R_GOT;
                break;
            case R_EMEX64_GOTPC64:
                ctx.in.gotPlt->hasGotPltOffRel.store(true, std::memory_order_relaxed);
                expr = R_GOTONLY_PC;
                break;
            case R_EMEX64_GOTPCREL64:
                expr = R_GOT_PC;
                break;
            case R_EMEX64_TPOFF64:
                if(rs.checkTlsLe(offset, sym, type))
                {
                    continue;
                }
                expr = R_TPREL;
                break;
            case R_EMEX64_GOTTPOFF64:
                rs.handleTlsIe<false>(R_GOT_PC, type, offset, addend, sym);
                continue;
            case R_EMEX64_TLSGD64:
                rs.handleTlsGd(R_TLSGD_PC, R_NONE, R_NONE, type, offset, addend, sym);
                continue;
            case R_EMEX64_DTPOFF64:
                sec.addReloc({R_DTPREL, type, offset, addend, &sym});
                continue;
            default:
                Err(ctx) << getErrorLoc(ctx, sec.content().data() + offset) << "unknown relocation (" << type.v << ") against symbol " << &sym;
                continue;
        }
        rs.process(expr, type, offset, sym, addend);
    }
}

RelType Emex64::getDynRel(RelType type) const
{
    if(type == R_EMEX64_ABS64)
    {
        return type;
    }
    return R_EMEX64_NONE;
}

int64_t Emex64::getImplicitAddend(const uint8_t *buf,
                                  RelType type) const
{
  switch(type)
  {
    case R_EMEX64_ABS64:
    case R_EMEX64_PC64:
    case R_EMEX64_RELATIVE:
    case R_EMEX64_GLOB_DAT:
    case R_EMEX64_JUMP_SLOT:
    case R_EMEX64_TPOFF64:
    case R_EMEX64_DTPMOD64:
    case R_EMEX64_DTPOFF64:
        return read64le(buf);
    case R_EMEX64_ABS32:
    case R_EMEX64_PC32:
        return SignExtend64<32>(read32le(buf));
    default:
        return 0;
    }
}

void Emex64::writeGotPlt(uint8_t *buf,
                         const Symbol &s) const
{
    write64le(buf, 0);
}

void Emex64::writePlt(uint8_t *buf,
                      const Symbol &sym,
                      uint64_t pltEntryAddr) const
{
    const uint8_t ldq[] = {0x0A, 0x78, 0x1B, 0x70};
    memcpy(buf, ldq, sizeof(ldq));
    write64le(buf + 4, sym.getGotPltVA(ctx) - pltEntryAddr);
    const uint8_t b[] = {0x26, 0x78, 0x01, 0x01};
    memcpy(buf + 12, b, sizeof(b));
}

static void writeBits32(uint8_t *loc,
                        uint64_t val,
                        unsigned shift)
{
    uint64_t cur = 0;
    unsigned n = (shift + 32 + 7) / 8;
    for(unsigned i = 0; i < n; ++i)
    {
        cur |= uint64_t(loc[i]) << (8 * i);
    }
    cur &= ~(0xFFFFFFFFULL << shift);
    cur |= (val & 0xFFFFFFFFULL) << shift;
    for(unsigned i = 0; i < n; ++i)
    {
        loc[i] = uint8_t(cur >> (8 * i));
    }
}

void Emex64::relocate(uint8_t *loc,
                      const Relocation &rel,
                      uint64_t val) const
{
    switch(rel.type)
    {
        case R_EMEX64_ABS8:
            checkIntUInt(ctx, loc, val, 8, rel);
            *loc = val;
            break;
        case R_EMEX64_ABS16:
            checkIntUInt(ctx, loc, val, 16, rel);
            write16le(loc, val);
            break;
        case R_EMEX64_ABS32:
            checkIntUInt(ctx, loc, val, 32, rel);
            write32le(loc, val);
            break;
        case R_EMEX64_PC32:
            checkInt(ctx, loc, val, 32, rel);
            write32le(loc, val);
            break;
        case R_EMEX64_BRANCH32:
        case R_EMEX64_PLT32:
            checkInt(ctx, loc, val, 32, rel);
            writeBits32(loc, val, 0);
            break;
        case R_EMEX64_BRANCH32_B4:
        case R_EMEX64_PLT32_B4:
            checkInt(ctx, loc, val, 32, rel);
            writeBits32(loc, val, 4);
            break;
        case R_EMEX64_ABS64:
        case R_EMEX64_PC64:
        case R_EMEX64_PLT64:
        case R_EMEX64_GOT64:
        case R_EMEX64_GOTPC64:
        case R_EMEX64_GOTPCREL64:
        case R_EMEX64_GOTTPOFF64:
        case R_EMEX64_TLSGD64:
        case R_EMEX64_RELATIVE:
        case R_EMEX64_GLOB_DAT:
        case R_EMEX64_JUMP_SLOT:
        case R_EMEX64_TPOFF64:
        case R_EMEX64_DTPMOD64:
        case R_EMEX64_DTPOFF64:
            write64le(loc, val);
            break;
        default:
            Err(ctx) << getErrorLoc(ctx, loc) << "unrecognized relocation " << rel.type;
    }
}

void elf::setEmex64TargetInfo(Ctx &ctx)
{
    ctx.target.reset(new Emex64(ctx));
}
