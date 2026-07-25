/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/ETLinker/emit.h>
#include <EmexToolchain/ETLinker/linker.h>

/*
 * barely ported start
 *
 * note: todo more we have to port those parts
 *       in the future too, for example for
 *       linker relaxation.
 */

static Boolean obj_register_symbols(linker_invocation_t *inv,
                                    linker_object_t *o)
{
    if(o->idx_symtab < 0)
    {
        return true;
    }

    ELF64_Shdr *symsh = &o->shdrs[o->idx_symtab];
    ELF64_Sym *syms = (ELF64_Sym *)(o->content + symsh->sh_offset);
    EFSize nsyms  = symsh->sh_size / sizeof(ELF64_Sym);
    const char *strtab = (o->idx_strtab >= 0) ? (char *)(o->content + o->shdrs[o->idx_strtab].sh_offset) : NULL;

    for(EFSize i = 0; i < nsyms; i++)
    {
        ELF64_Sym *sym = &syms[i];
        UInt8 bind = sym->st_info >> 4;

        const char *name = strtab + sym->st_name;
        if(bind != kELFSymbolTableBindingGlobal ||
           sym->st_shndx == kELFSectionHeaderNumberUndefined ||
           !strtab ||
           !name || !*name)
        {
            continue;
        }

        UInt64 addr = 0;

        if(sym->st_shndx == kELFSectionHeaderNumberAbsolute)
        {
            addr = sym->st_value;
        }
        else if((SInt32)sym->st_shndx == o->idx_text)
        {
            addr = o->base_text + sym->st_value;
        }
        else if((SInt32)sym->st_shndx == o->idx_data)
        {
            addr = o->base_data + sym->st_value;
        }
        else if((SInt32)sym->st_shndx == o->idx_bss)
        {
            addr = o->base_bss  + sym->st_value;
        }
        else
        {
            addr = sym->st_value;
        }

        if(!linker_symbol_append_definition(inv, name, EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(o->file)), kEFStringEncodingUTF8), addr))
        {
            return false;
        }
    }
    return true;
}

static void obj_unregister_all_symbols(linker_invocation_t *inv)
{
    linker_symbol_t *sym = inv->sym;
    while(sym != NULL)
    {
        linker_symbol_t *next = sym->next;
        linker_symbol_dealloc(sym);
        sym = next;
    }
    inv->sym = NULL;
}

static UInt64 sym_resolve(linker_invocation_t *inv,
                          const linker_object_t *o,
                          UInt32 sym_idx,
                          Boolean *success)
{
    *success = true;

    if(o->idx_symtab < 0)
    {
        return 0;
    }

    ELF64_Shdr *symsh = &o->shdrs[o->idx_symtab];
    ELF64_Sym *syms = (ELF64_Sym *)(o->content + symsh->sh_offset);
    EFSize nsyms = symsh->sh_size / sizeof(ELF64_Sym);
    const char *strtab = (o->idx_strtab >= 0) ? (char *)(o->content + o->shdrs[o->idx_strtab].sh_offset) : NULL;

    if(sym_idx >= nsyms)
    {
        return 0;
    }

    ELF64_Sym *sym = &syms[sym_idx];
    (void)(sym->st_info >> 4);

    if((sym->st_info & 0xf) == kELFSymbolTableTypeSection)
    {
        if((SInt32)sym->st_shndx == o->idx_text)
        {
            return o->base_text;
        }
        if((SInt32)sym->st_shndx == o->idx_data)
        {
            return o->base_data;
        }
        if((SInt32)sym->st_shndx == o->idx_bss)
        {
            return o->base_bss;
        }
        return 0;
    }

    if(strtab)
    {
        const char *name = strtab + sym->st_name;
        linker_symbol_t *gsym = linker_symbol_lookup(inv, name);
        if(gsym && gsym->defined)
        {
            return gsym->addr;
        }

        if(sym->st_shndx != kELFSectionHeaderNumberUndefined)
        {
            if((SInt32)sym->st_shndx == o->idx_text)
            {
                return o->base_text + sym->st_value;
            }
            if((SInt32)sym->st_shndx == o->idx_data)
            {
                return o->base_data + sym->st_value;
            }
            if((SInt32)sym->st_shndx == o->idx_bss)
            {
                return o->base_bss + sym->st_value;
            }
        }

        diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "undefined symbol '%s', needed by '%s'", name, EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(o->file)), kEFStringEncodingUTF8));
        *success = false;
        return 0;
    }
    return 0;
}

static Boolean obj_apply_relocs(linker_invocation_t *inv,
                                const linker_object_t *o,
                                EFBitWalkerRef vb)
{
    if(o->idx_rela_text >= 0)
    {
        ELF64_Shdr *rs = &o->shdrs[o->idx_rela_text];
        ELF64_Rela *rela = (ELF64_Rela *)(o->content + rs->sh_offset);
        EFSize cnt = rs->sh_size / sizeof(ELF64_Rela);

        for(EFSize i = 0; i < cnt; i++)
        {
            UInt32 type = (UInt32)ELF32_R_TYPE(rela[i].r_info);
            UInt32 sym_idx = (UInt32)ELF32_R_SYM(rela[i].r_info);
            UInt64 offset = rela[i].r_offset;
            SInt64  addend = rela[i].r_addend;

            if(type != R_EMEX64_ABS64)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "unsupported relocation type %u in .rela.text", type);
                return false;
            }

            Boolean success = false;
            UInt64 sym_addr = sym_resolve(inv, o, sym_idx, &success);
            if(!success)
            {
                /* error printed by callee */
                return false;
            }
            UInt64 value = sym_addr + (UInt64)addend;

            EFBitWalkerSeek(vb, o->base_text + offset, 0);
            EFBitWalkerWrite(vb, value, 64);
        }
    }

    if(o->idx_rela_data >= 0)
    {
        ELF64_Shdr *rs = &o->shdrs[o->idx_rela_data];
        ELF64_Rela *rela = (ELF64_Rela *)(o->content + rs->sh_offset);
        EFSize cnt = rs->sh_size / sizeof(ELF64_Rela);

        for(EFSize i = 0; i < cnt; i++)
        {
            UInt32 type = (UInt32)ELF32_R_TYPE(rela[i].r_info);
            UInt32 sym_idx = (UInt32)ELF32_R_SYM(rela[i].r_info);
            UInt64 offset = rela[i].r_offset;
            SInt64 addend = rela[i].r_addend;

            if(type != R_EMEX64_ABS64)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "unsupported relocation type %u in .rela.data", type);
                return false;
            }

            Boolean success = false;
            UInt64 sym_addr = sym_resolve(inv, o, sym_idx, &success);
            if(!success)
            {
                /* error printed by callee */
                return false;
            }
            UInt64 value = sym_addr + (UInt64)addend;

            EFBitWalkerSeek(vb, o->base_data + offset, 0);
            EFBitWalkerWrite(vb, value, 64);
        }
    }

    return true;
}

/* barely ported end */

typedef struct {
    UInt8 *data;
    EFSize len;
    EFSize cap;
} buf_t;

static Boolean buf_reserve(buf_t *b,
                           EFSize extra)
{
    if(b->len + extra <= b->cap)
    {
        return true;
    }
    EFSize ncap = b->cap ? b->cap * 2 : 64;
    while(ncap < b->len + extra)
    {
        ncap *= 2;
    }
    UInt8 *nd = realloc(b->data, ncap);
    if(!nd)
    {
        return false;
    }
    b->data = nd;
    b->cap  = ncap;
    return true;
}

static Boolean buf_append(buf_t *b,
                          const void *src,
                          EFSize n)
{
    if(!buf_reserve(b, n))
    {
        return false;
    }
    memcpy(b->data + b->len, src, n);
    b->len += n;
    return true;
}

static Boolean buf_append_u8(buf_t *b,
                             UInt8 v)
{
    return buf_append(b, &v, 1);
}

static Boolean __attribute__((unused)) buf_append_u64(buf_t *b,
                                                      UInt64 v)
{
    /* little-endian */
    UInt8 tmp[8];
    for(SInt32 i = 0; i < 8; i++)
    {
        tmp[i] = v & 0xff; v >>= 8;
    }
    return buf_append(b, tmp, 8);
}

static EFSize strtab_intern(buf_t *strtab,
                            const char *s)
{
    EFSize i = 0;
    while(i < strtab->len)
    {
        if (strcmp((char*)(strtab->data + i), s) == 0) return i;
        i += strlen((char*)(strtab->data + i)) + 1;
    }
    EFSize off = strtab->len;
    buf_append(strtab, s, strlen(s) + 1);
    return off;
}

static Boolean __linker_link_relocatable(linker_invocation_t *inv,
                                         EFFileRef output)
{
    buf_t text = {0}, data = {0};
    buf_t rela_text = {0}, rela_data = {0};
    buf_t sym_buf = {0}, strtab_buf = {0}, shstrtab_buf = {0};

    buf_append_u8(&strtab_buf, 0);
    buf_append_u8(&shstrtab_buf, 0);
    ELF64_Sym null_sym = {0};
    buf_append(&sym_buf, &null_sym, sizeof(null_sym));

    const char *pathCStr = EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(output)), kEFStringEncodingUTF8);

    const char *out_name = (output && pathCStr) ? pathCStr : "linked.o";
    const char *slash = strrchr(out_name, '/');
    const char *file_name = slash ? slash + 1 : out_name;

    /* making sure the file entry matches the new output path */
    ELF64_Sym file_sym = {
        .st_name = (UInt32)strtab_intern(&strtab_buf, file_name),
        .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeFile),
        .st_other = kELFSymbolVisibilityDefault,
        .st_shndx = kELFSectionHeaderNumberAbsolute,
        .st_value = 0,
        .st_size = 0,
    };
    buf_append(&sym_buf, &file_sym, sizeof(file_sym));

    /* section symbols */
    ELF64_Sym sec_text = { .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeSection), .st_shndx = kELFSectionHeaderIndexText };
    ELF64_Sym sec_data = { .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeSection), .st_shndx = kELFSectionHeaderIndexData };
    ELF64_Sym sec_bss = { .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeSection), .st_shndx = kELFSectionHeaderIndexBSS };
    buf_append(&sym_buf, &sec_text, sizeof(sec_text));
    buf_append(&sym_buf, &sec_data, sizeof(sec_data));
    buf_append(&sym_buf, &sec_bss, sizeof(sec_bss));

    UInt32 first_global = (UInt32)(sym_buf.len / sizeof(ELF64_Sym));

    const UInt64 text_region_base = (UInt64)BOOT_HEADER_SIZE;
    const UInt64 data_region_base = inv->out_text_off;
    const UInt64 bss_region_base = inv->out_data_off;

    /* copy all data and text sections */
    for(linker_object_t *o = inv->obj; o; o = o->next)
    {
        if(o->idx_text >= 0)
        {
            ELF64_Shdr *sh = &o->shdrs[o->idx_text];
            buf_append(&text, o->content + sh->sh_offset, sh->sh_size);
        }
        if(o->idx_data >= 0)
        {
            ELF64_Shdr *sh = &o->shdrs[o->idx_data];
            buf_append(&data, o->content + sh->sh_offset, sh->sh_size);
        }
    }

    /* add all defined global symbols from the objects */
    for(linker_object_t *o = inv->obj; o; o = o->next)
    {
        if(o->idx_symtab < 0)
        {
            continue;
        }

        ELF64_Shdr *symsh = &o->shdrs[o->idx_symtab];
        ELF64_Sym  *syms  = (ELF64_Sym *)(o->content + symsh->sh_offset);
        EFSize nsyms = symsh->sh_size / sizeof(ELF64_Sym);
        const char *strtab = (o->idx_strtab >= 0) ? (char *)(o->content + o->shdrs[o->idx_strtab].sh_offset) : NULL;

        for(EFSize i = 0; i < nsyms; i++)
        {
            ELF64_Sym *sym = &syms[i];
            UInt8 bind = sym->st_info >> 4;
            UInt8 type = sym->st_info & 0x0F;

            const char *name = strtab + sym->st_name;
            if(sym->st_shndx == kELFSectionHeaderNumberUndefined ||
               !strtab ||
               !name || !*name)
            {
                continue;
            }

            /* check if we already have this symbol */
            EFSize n = sym_buf.len / sizeof(ELF64_Sym);
            ELF64_Sym *out_syms = (ELF64_Sym *)sym_buf.data;
            Boolean already_exists = false;
            for(EFSize s = first_global; s < n; s++)
            {
                if(strcmp((char*)(strtab_buf.data + out_syms[s].st_name), name) == 0)
                {
                    already_exists = true;
                    break;
                }
            }
            if(already_exists)
            {
                continue;
            }

            UInt64 value = 0;
            UInt16 shndx = kELFSectionHeaderNumberAbsolute;

            if((SInt32)sym->st_shndx == o->idx_text)
            {
                value = (o->base_text + sym->st_value) - text_region_base;
                shndx = kELFSectionHeaderIndexText;
            }
            else if((SInt32)sym->st_shndx == o->idx_data)
            {
                if(inv->options.use_old_magic)
                {
                    value = (o->base_text + sym->st_value) - text_region_base;
                    shndx = kELFSectionHeaderIndexText;
                }
                else
                {
                    value = (o->base_data + sym->st_value) - data_region_base;
                    shndx = kELFSectionHeaderIndexData;
                }
            }
            else if((SInt32)sym->st_shndx == o->idx_bss)
            {
                value = (o->base_bss + sym->st_value) - bss_region_base;
                shndx = kELFSectionHeaderIndexBSS;
            }
            else
            {
                value = sym->st_value;
                shndx = kELFSectionHeaderNumberAbsolute;
            }

            ELF64_Sym new_sym = {
                .st_name = (UInt32)strtab_intern(&strtab_buf, name),
                .st_info = ELF_SYM_INFO(bind, type),
                .st_other = kELFSymbolVisibilityDefault,
                .st_shndx = shndx,
                .st_value = value,
                .st_size = 0,
            };
            buf_append(&sym_buf, &new_sym, sizeof(new_sym));
        }
    }

    /* process relocations */
    for(linker_object_t *o = inv->obj; o; o = o->next)
    {
        #define PROCESS_RELA_SECTION(rela_shdr_idx, out_buf, base_addr, region_base) do { \
            if(o->rela_shdr_idx >= 0) \
            { \
                ELF64_Shdr *rs = &o->shdrs[o->rela_shdr_idx]; \
                ELF64_Rela *rela = (ELF64_Rela *)(o->content + rs->sh_offset); \
                EFSize cnt = rs->sh_size / sizeof(ELF64_Rela); \
                for(EFSize i = 0; i < cnt; i++) \
                { \
                    UInt32 type = ELF32_R_TYPE(rela[i].r_info); \
                    UInt32 in_sym = ELF32_R_SYM(rela[i].r_info); \
                    SInt64 addend = rela[i].r_addend; \
                    if(type != R_EMEX64_ABS64) \
                    { \
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "unsupported relocation type %u", type); \
                        continue; \
                    } \
                    UInt64 offset = ((base_addr) + rela[i].r_offset) - (region_base); \
                    \
                    \
                    const char *name = NULL; \
                    if(o->idx_symtab >= 0 && o->idx_strtab >= 0) \
                    { \
                        ELF64_Shdr *symsh = &o->shdrs[o->idx_symtab]; \
                        ELF64_Sym *syms = (ELF64_Sym *)(o->content + symsh->sh_offset); \
                        if(in_sym < symsh->sh_size / sizeof(ELF64_Sym)) \
                        { \
                            name = (char*)(o->content + o->shdrs[o->idx_strtab].sh_offset) + syms[in_sym].st_name; \
                        } \
                    } \
                    if(!name || !*name) \
                    { \
                        name = "<unknown>"; \
                    } \
                    \
                    /* Find or create the matching output symbol (by name) */ \
                    EFSize n = sym_buf.len / sizeof(ELF64_Sym); \
                    ELF64_Sym *out_syms = (ELF64_Sym *)sym_buf.data; \
                    UInt32 out_sym_idx = 0; \
                    for(EFSize s = first_global; s < n; s++) \
                    { \
                        if(strcmp((char*)(strtab_buf.data + out_syms[s].st_name), name) == 0) \
                        { \
                            out_sym_idx = (UInt32)s; break; \
                        } \
                    } \
                    if(out_sym_idx == 0) \
                    { \
                        ELF64_Sym usym = { \
                            .st_name = (UInt32)strtab_intern(&strtab_buf, name), \
                            .st_info = ELF_SYM_INFO(kELFSymbolTableBindingGlobal, kELFSymbolTableTypeNoType), \
                            .st_other = kELFSymbolVisibilityDefault, \
                            .st_shndx = kELFSectionHeaderNumberUndefined, \
                            .st_value = 0, \
                            .st_size = 0, \
                        }; \
                        out_sym_idx = (UInt32)(sym_buf.len / sizeof(ELF64_Sym)); \
                        buf_append(&sym_buf, &usym, sizeof(usym)); \
                    } \
                    \
                    ELF64_Rela new_rela = { \
                        .r_offset = offset, \
                        .r_info   = ELF64_R_INFO(out_sym_idx, R_EMEX64_ABS64), \
                        .r_addend = addend, \
                    }; \
                    buf_append(&(out_buf), &new_rela, sizeof(new_rela)); \
                } \
            } \
        } while(0)

        PROCESS_RELA_SECTION(idx_rela_text, rela_text, o->base_text, text_region_base);
        PROCESS_RELA_SECTION(idx_rela_data, rela_data, o->base_data, data_region_base);

        #undef PROCESS_RELA_SECTION
    }

    /* build shstrtab */
    UInt32 shname_text = (UInt32)strtab_intern(&shstrtab_buf, ".text");
    UInt32 shname_data = (UInt32)strtab_intern(&shstrtab_buf, ".data");
    UInt32 shname_bss = (UInt32)strtab_intern(&shstrtab_buf, ".bss");
    UInt32 shname_rela_text = (UInt32)strtab_intern(&shstrtab_buf, ".rela.text");
    UInt32 shname_rela_data = (UInt32)strtab_intern(&shstrtab_buf, ".rela.data");
    UInt32 shname_symtab = (UInt32)strtab_intern(&shstrtab_buf, ".symtab");
    UInt32 shname_strtab = (UInt32)strtab_intern(&shstrtab_buf, ".strtab");
    UInt32 shname_shstrtab  = (UInt32)strtab_intern(&shstrtab_buf, ".shstrtab");

    /* calculate file layout */
    EFSize ehdr_size = sizeof(ELF64_Ehdr);
    EFSize text_off = ehdr_size;
    EFSize data_off = text_off + text.len;
    EFSize rela_text_off = data_off + data.len;
    EFSize rela_data_off = rela_text_off + rela_text.len;
    EFSize sym_off = rela_data_off + rela_data.len;
    EFSize str_off = sym_off + sym_buf.len;
    EFSize shstr_off = str_off + strtab_buf.len;
    EFSize shdr_off = shstr_off + shstrtab_buf.len;

    /* write relocatable elf file ^^ */
    EFAUTOREL EFBitWalkerRef vb = EFFileCopyBitWalker(EFGetAllocator(output), output, kEFEndianLittle);
    if(!vb)
    {
        free(text.data);
        free(data.data);
        free(rela_text.data);
        free(rela_data.data);
        free(sym_buf.data);
        free(strtab_buf.data);
        free(shstrtab_buf.data);
        return false;
    }

    ELF64_Ehdr ehdr = {0};
    memcpy(ehdr.e_ident, ident, EI_NIDENT);
    ehdr.e_type = kELFTypeRel;
    ehdr.e_machine = ELF_MAGIC_EMEX64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_shoff = shdr_off;
    ehdr.e_ehsize = sizeof(ELF64_Ehdr);
    ehdr.e_shentsize = sizeof(ELF64_Shdr);
    ehdr.e_shnum = kELFSectionHeaderIndexCount;
    ehdr.e_shstrndx = kELFSectionHeaderIndexShstrtab;

    EFBitWalkerWriteBuffer(vb, (const char*)&ehdr, sizeof(ehdr));
    EFBitWalkerWriteBuffer(vb, (const char*)text.data, text.len);
    EFBitWalkerWriteBuffer(vb, (const char*)data.data, data.len);
    EFBitWalkerWriteBuffer(vb, (const char*)rela_text.data, rela_text.len);
    EFBitWalkerWriteBuffer(vb, (const char*)rela_data.data, rela_data.len);
    EFBitWalkerWriteBuffer(vb, (const char*)sym_buf.data, sym_buf.len);
    EFBitWalkerWriteBuffer(vb, (const char*)strtab_buf.data, strtab_buf.len);
    EFBitWalkerWriteBuffer(vb, (const char*)shstrtab_buf.data, shstrtab_buf.len);

    ELF64_Shdr shdrs[kELFSectionHeaderIndexCount] = {
        [kELFSectionHeaderIndexText] = (ELF64_Shdr){
            .sh_name = shname_text,
            .sh_type = kELFSectionHeaderTypeProgbits,
            .sh_flags = kELFSectionFlagAlloc | kELFSectionFlagExec | inv->options.use_old_magic ? kELFSectionFlagWrite : 0,
            .sh_offset = text_off,
            .sh_size = text.len,
            .sh_addralign = 1,
        },
        [kELFSectionHeaderIndexData] = (ELF64_Shdr){
            .sh_name = shname_data,
            .sh_type = kELFSectionHeaderTypeProgbits,
            .sh_flags = kELFSectionFlagAlloc | kELFSectionFlagWrite,
            .sh_offset = data_off,
            .sh_size = data.len,
            .sh_addralign = 1,
        },
        [kELFSectionHeaderIndexBSS] = (ELF64_Shdr){
            .sh_name = shname_bss,
            .sh_type = kELFSectionHeaderTypeNobits,
            .sh_flags = kELFSectionFlagAlloc | kELFSectionFlagWrite,
            .sh_offset = data_off + data.len,
            .sh_size = inv->out_bss_off - inv->out_data_off,
            .sh_addralign = 1,
        },
        [kELFSectionHeaderIndexRelaText] = (ELF64_Shdr){
            .sh_name = shname_rela_text,
            .sh_type = kELFSectionHeaderTypeRelative,
            .sh_offset = rela_text_off,
            .sh_size = rela_text.len,
            .sh_link = kELFSectionHeaderIndexSymtab,
            .sh_info = kELFSectionHeaderIndexText,
            .sh_addralign = 8,
            .sh_entsize  = sizeof(ELF64_Rela),
        },
        [kELFSectionHeaderIndexRelaData] = (ELF64_Shdr){
            .sh_name = shname_rela_data,
            .sh_type = kELFSectionHeaderTypeRelative,
            .sh_offset = rela_data_off,
            .sh_size = rela_data.len,
            .sh_link = kELFSectionHeaderIndexSymtab,
            .sh_info = kELFSectionHeaderIndexData,
            .sh_addralign = 8,
            .sh_entsize  = sizeof(ELF64_Rela),
        },
        [kELFSectionHeaderIndexSymtab] = (ELF64_Shdr){
            .sh_name = shname_symtab,
            .sh_type = kELFSectionHeaderTypeSymtab,
            .sh_offset = sym_off,
            .sh_size = sym_buf.len,
            .sh_link = kELFSectionHeaderIndexStrtab,
            .sh_info = first_global,
            .sh_addralign = 8,
            .sh_entsize  = sizeof(ELF64_Sym),
        },
        [kELFSectionHeaderIndexStrtab] = (ELF64_Shdr){
            .sh_name = shname_strtab,
            .sh_type = kELFSectionHeaderTypeStrtab,
            .sh_offset = str_off,
            .sh_size = strtab_buf.len,
            .sh_addralign = 1,
        },
        [kELFSectionHeaderIndexShstrtab] = (ELF64_Shdr){
            .sh_name = shname_shstrtab,
            .sh_type = kELFSectionHeaderTypeStrtab,
            .sh_offset = shstr_off,
            .sh_size = shstrtab_buf.len,
            .sh_addralign = 1,
        },
    };

    EFBitWalkerWriteBuffer(vb, (const char *)shdrs, sizeof(shdrs));

    EFBitWalkerSync(vb);

    /* cleanup */
    free(text.data);
    free(data.data);
    free(rela_text.data);
    free(rela_data.data);
    free(sym_buf.data);
    free(strtab_buf.data);
    free(shstrtab_buf.data);

    return true;
}

static void emit_boot_header(EFBitWalkerRef vb,
                             UInt64 entry)
{
    EFBitWalkerSeek(vb, 0, 0);
    EFBitWalkerWrite(vb, kE64OpcodeB, 8);
    EFBitWalkerWrite(vb, kE64ParameterCodingAddr64, 3);
    EFBitWalkerAlignByte(vb);
    EFBitWalkerWrite(vb, entry, 64);
}

static Boolean __linker_link_firmware(linker_invocation_t *inv,
                                      EFFileRef output)
{
    linker_symbol_t *gsym = linker_symbol_lookup(inv, inv->options.entry_name);
    if(gsym == NULL || !gsym->defined)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "entry symbol '%s' not found", inv->options.entry_name);
        return false;
    }

    if(gsym->addr == BOOT_HEADER_SIZE)
    {
        inv->needs_fw_hdr = false;

        /* needs to be entirely relocated */
        obj_unregister_all_symbols(inv);
        linker_layout(inv);

        /* re-applying linker scripts */
        if(!linker_script_apply(inv, inv->out_bss_off, 0, inv->out_text_off, inv->out_bss_off > inv->out_data_off ? inv->out_data_off : inv->out_bss_off))
        {
            linker_invocation_dealloc(inv);
            return false;
        }

        /* reregister them */
        linker_object_t *obj = inv->obj;
        while(obj != NULL)
        {
            if(!obj_register_symbols(inv, obj))
            {
                return false;
            }
            obj = obj->next;
        }
    }

    EFAUTOREL EFBitWalkerRef vb = EFFileCopyBitWalker(EFGetAllocator(output), output, kEFEndianLittle);
    if(vb == NULL)
    {
        return false;
    }

    /* copy .text sections */
    linker_object_t *obj = inv->obj;
    while(obj != NULL)
    {
        if(obj->idx_text < 0)
        {
            obj = obj->next;
            continue;
        }
        ELF64_Shdr *sh = &obj->shdrs[obj->idx_text];
        EFBitWalkerSeek(vb, obj->base_text, 0);
        EFBitWalkerWriteBuffer(vb, obj->content + sh->sh_offset, sh->sh_size);
        obj = obj->next;
    }

    /* copy .data sections */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(obj->idx_data < 0)
        {
            obj = obj->next;
            continue;
        }
        ELF64_Shdr *sh = &obj->shdrs[obj->idx_data];
        EFBitWalkerSeek(vb, obj->base_data, 0);
        EFBitWalkerWriteBuffer(vb, obj->content + sh->sh_offset, sh->sh_size);
        obj = obj->next;
    }

    /* .bss: already zeroed by calloc */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(!obj_apply_relocs(inv, obj, vb))
        {
            return false;
        }
        obj = obj->next;
    }

    UInt64 entry_addr = 0;
    if(inv->needs_fw_hdr)
    {
        gsym = linker_symbol_lookup(inv, inv->options.entry_name);
        if(!gsym || !gsym->defined)
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "entry symbol '%s' not found", inv->options.entry_name);
            return false;
        }
        entry_addr = gsym->addr;
        emit_boot_header(vb, entry_addr);
    }

    EFBitWalkerSync(vb);

    UInt64 total_text = inv->out_text_off - (inv->needs_fw_hdr ? BOOT_HEADER_SIZE : 0);
    UInt64 total_data = inv->out_data_off - inv->out_text_off;

    if(inv->options.verbose)
    {
        const char *pathCStr = EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(output)), kEFStringEncodingUTF8);

        fprintf(stderr, "emex64ld: linked object(s) → %s\n", pathCStr);
        if(inv->options.emit_mode == kEmitModeFirmware)
        {
            fprintf(stderr, "  .fw_hdr  %8lu bytes @ 0x%08lx\n", inv->needs_fw_hdr ? (unsigned long)BOOT_HEADER_SIZE : 0, (unsigned long)0);
        }
        fprintf(stderr, "  .text    %8lu bytes @ 0x%08lx\n", (unsigned long)total_text, inv->needs_fw_hdr ? (unsigned long)BOOT_HEADER_SIZE : 0);
        fprintf(stderr, "  .data    %8lu bytes @ 0x%08lx\n", (unsigned long)total_data, (unsigned long)inv->out_text_off);
        fprintf(stderr, "  .bss     %8lu bytes @ 0x%08lx (virtual)\n", (unsigned long)(inv->out_bss_off - inv->out_data_off), (unsigned long)inv->out_data_off);
        fprintf(stderr, "  entry    %s @ 0x%08lx\n", inv->options.entry_name, (unsigned long)entry_addr);
    }

    return true;
}

Boolean linker_link(linker_options_t options,
                    linker_diagnostic_consumer_t *diagnostic_consumer,
                    EFFileRef *input_file,
                    UInt64 input_file_cnt,
                    EFFileRef *linker_script_file,
                    UInt64 linker_script_file_cnt,
                    EFFileRef output)
{
    linker_invocation_t *inv = linker_invocation_alloc(options, diagnostic_consumer);
    if(inv == NULL)
    {
        return false;
    }

    for(UInt64 i = 0; i < input_file_cnt; i++)
    {
        if(!linker_load_object(inv, input_file[i]))
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "object file \'%s\' couldn't be loaded", EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(input_file[i])), kEFStringEncodingUTF8));
            linker_invocation_dealloc(inv);
            return false;
        }
    }
    linker_layout(inv);

    for(UInt64 i = 0; i < linker_script_file_cnt; i++)
    {
        if(!linker_script_parse(inv, linker_script_file[i]))
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "linker script file \'%s\' is problematic", EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(input_file[i])), kEFStringEncodingUTF8));
            linker_invocation_dealloc(inv);
            return false;
        }
    }

    if(!linker_script_apply(inv, inv->out_bss_off, BOOT_HEADER_SIZE, inv->out_text_off, inv->out_bss_off > inv->out_data_off ? inv->out_data_off : inv->out_bss_off))
    {
        linker_invocation_dealloc(inv);
        return false;
    }

    linker_object_t *obj = inv->obj;
    while(obj != NULL)
    {
        if(!obj_register_symbols(inv, obj))
        {
            linker_invocation_dealloc(inv);
            return false;
        }
        obj = obj->next;
    }

    Boolean success = false;
    switch(options.emit_mode)
    {
        case kEmitModeRelocatableObject:
            success = __linker_link_relocatable(inv, output);
            if(options.verbose && success)
            {
                fprintf(stderr, "emex64ld: emitted relocatable object → %s\n", EFStringGetCStringPtr(EFURLGetPath(EFFileGetURL(output)), kEFStringEncodingUTF8));
            }
            break;
        case kEmitModeFirmware:
        default:
            success = __linker_link_firmware(inv, output);
            break;
    }

    linker_invocation_dealloc(inv);
    return success;
}
