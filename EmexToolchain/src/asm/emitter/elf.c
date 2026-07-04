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
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/support/virtual/vbitwalker.h>
#include <EmexToolchain/support/parser.h>
#include <EmexToolchain/asm/label/label.h>
#include <EmexToolchain/asm/invocation.h>
#include <EmexToolchain/asm/emitter/opcode.h>
#include <EmexToolchain/asm/emitter/register.h>
#include <EmexToolchain/asm/emitter/emitter.h>
#include <EmexToolchain/asm/section.h>
#include <EmexToolchain/linker/linker.h>

typedef struct {
    UInt8 *data;
    size_t len;
    size_t cap;
} buf_t;

static Boolean buf_reserve(buf_t *b, size_t extra)
{
    if(b->len + extra <= b->cap)
    {
        return true;
    }
    size_t ncap = b->cap ? b->cap * 2 : 64;
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

static Boolean buf_append(buf_t *b, const void *src, size_t n)
{
    if(!buf_reserve(b, n))
    {
        return false;
    }
    memcpy(b->data + b->len, src, n);
    b->len += n;
    return true;
}

static Boolean buf_append_u8(buf_t *b, UInt8 v)
{
    return buf_append(b, &v, 1);
}

static Boolean __attribute__((unused)) buf_append_u64(buf_t *b, UInt64 v)
{
    /* little-endian */
    UInt8 tmp[8];
    for(int i = 0; i < 8; i++)
    {
        tmp[i] = v & 0xff; v >>= 8;
    }
    return buf_append(b, tmp, 8);
}

static size_t strtab_intern(buf_t *strtab, const char *s)
{
    size_t i = 0;
    while(i < strtab->len)
    {
        if (strcmp((char*)(strtab->data + i), s) == 0) return i;
        i += strlen((char*)(strtab->data + i)) + 1;
    }
    size_t off = strtab->len;
    buf_append(strtab, s, strlen(s) + 1);
    return off;
}

Boolean assembler_elf_emit(assembler_invocation_t *inv)
{
    Boolean ok = false;

    vfd_t *d = inv->out_vbitwalker->d;

    struct stat st;
    if(vfd_stat(d, &st) != 0)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "elf_emit: fstat failed");
        return false;
    }

    size_t flat_size = (size_t)st.st_size;
    UInt8 *flat = malloc(flat_size);
    if(!flat)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "elf_emit: out of memory");
        return false;
    }

    if(vfd_seek(d, 0, SEEK_SET) < 0 || vfd_read(d, flat, flat_size) != (ssize_t)flat_size)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "elf_emit: read flat binary failed");
        free(flat);
        return false;
    }

    size_t data_start = (inv->data_section_start != UINT64_MAX) ? (size_t)inv->data_section_start : flat_size;
    size_t data_end_raw = (inv->data_section_end != UINT64_MAX) ? (size_t)inv->data_section_end : data_start;
    size_t bss_size  = (size_t)inv->bss_section_size;
    size_t bss_start = (inv->bss_section_start != UINT64_MAX) ? (size_t)inv->bss_section_start : flat_size;
    size_t bss_end = bss_start != flat_size ? bss_start + bss_size : flat_size;

    size_t code_start = 10;
    if(inv->data_section_start != UINT64_MAX && data_end_raw > code_start)
    {
        code_start = data_end_raw;
    }
    if(inv->bss_section_start != UINT64_MAX && bss_end > code_start)
    {
        code_start = bss_end;
    }

    size_t text_start = code_start;
    size_t text_size = flat_size > text_start ? flat_size - text_start : 0;
    size_t data_size = data_end_raw > data_start && data_start < flat_size ? data_end_raw - data_start : 0;

    const UInt8 *text_bytes = flat + text_start;
    const UInt8 *data_bytes = flat + data_start;

    buf_t sym_buf = {0};
    buf_t strtab_buf = {0};

    buf_append_u8(&strtab_buf, 0);
    ELF64_Sym sym0 = {0};
    buf_append(&sym_buf, &sym0, sizeof(sym0));

    const char *src_fname = (inv->file_cnt > 0) ? inv->file[0]->path : "<unknown>";
    const char *base = strrchr(src_fname, '/');
    base = base ? base + 1 : src_fname;

    ELF64_Sym sym_file = {
        .st_name = (UInt32)strtab_intern(&strtab_buf, base),
        .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeFile),
        .st_other = kELFSymbolVisibilityDefault,
        .st_shndx = kELFSectionHeaderNumberAbsolute,
        .st_value = 0,
        .st_size = 0,
    };
    buf_append(&sym_buf, &sym_file, sizeof(sym_file));

    ELF64_Sym sym_text_sec = {
        .st_name = 0,
        .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeSection),
        .st_other = kELFSymbolVisibilityDefault,
        .st_shndx = kELFSectionHeaderIndexText,
        .st_value = 0,
        .st_size = 0,
    };
    ELF64_Sym sym_data_sec = {
        .st_name = 0,
        .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeSection),
        .st_other = kELFSymbolVisibilityDefault,
        .st_shndx = kELFSectionHeaderIndexData,
        .st_value = 0,
        .st_size = 0,
    };
    ELF64_Sym sym_bss_sec = {
        .st_name = 0,
        .st_info = ELF_SYM_INFO(kELFSymbolTableBindingLocal, kELFSymbolTableTypeSection),
        .st_other = kELFSymbolVisibilityDefault,
        .st_shndx = kELFSectionHeaderIndexBSS,
        .st_value = 0,
        .st_size = 0,
    };

    size_t local_section_text_idx __attribute__((unused)) = sym_buf.len / sizeof(ELF64_Sym);
    buf_append(&sym_buf, &sym_text_sec, sizeof(sym_text_sec));
    size_t local_section_data_idx __attribute__((unused)) = sym_buf.len / sizeof(ELF64_Sym);
    buf_append(&sym_buf, &sym_data_sec, sizeof(sym_data_sec));
    size_t local_section_bss_idx __attribute__((unused))  = sym_buf.len / sizeof(ELF64_Sym);
    buf_append(&sym_buf, &sym_bss_sec,  sizeof(sym_bss_sec));

    UInt32 first_global = (UInt32)(sym_buf.len / sizeof(ELF64_Sym));

    const void *key; size_t klen; assembler_label_t *lbl;
    for(hashmap_iter_t it = hashmap_iter_create(inv->label_hashmap); hashmap_next(&it, &key, &klen, (void**)&lbl);)
    {
        if(!lbl->name)
        {
            continue;
        }

        if(!lbl->defined)
        {
            continue;
        }

        UInt64 addr = lbl->addr;
        UInt16 shndx;
        UInt64 st_value;

        if(inv->data_section_start != UINT64_MAX && addr >= inv->data_section_start && addr < data_end_raw)
        {
            shndx = kELFSectionHeaderIndexData;
            st_value = addr - inv->data_section_start;
        }
        else if(inv->bss_section_start != UINT64_MAX && addr >= bss_start && addr < bss_end)
        {
            shndx = kELFSectionHeaderIndexBSS;
            st_value = addr - bss_start;
        }
        else if(addr >= text_start)
        {
            shndx = kELFSectionHeaderIndexText;
            st_value = addr - text_start;
        }
        else
        {
            shndx = kELFSectionHeaderNumberAbsolute;
            st_value = addr;
        }

        ELF64_Sym sym = {
            .st_name = (UInt32)strtab_intern(&strtab_buf, lbl->name),
            .st_info = ELF_SYM_INFO(kELFSymbolTableBindingGlobal, kELFSymbolTableTypeNoType),
            .st_other = kELFSymbolVisibilityDefault,
            .st_shndx = shndx,
            .st_value = st_value,
            .st_size = 0,
        };
        buf_append(&sym_buf, &sym, sizeof(sym));
    }

    buf_t rela_text_buf = {0};
    buf_t rela_data_buf = {0};

    reloc_table_entry_t *rtbe = inv->rtbe;
    while(rtbe)
    {
        UInt32 sym_idx = 0;
        {
            size_t n = sym_buf.len / sizeof(ELF64_Sym);
            ELF64_Sym *syms = (ELF64_Sym *)sym_buf.data;
            for(size_t s = first_global; s < n; s++)
            {
                const char *sname = (char*)(strtab_buf.data + syms[s].st_name);
                if(strcmp(sname, rtbe->name) == 0)
                {
                    sym_idx = (UInt32)s;
                    break;
                }
            }
            if(sym_idx == 0)
            {
                ELF64_Sym usym = {
                    .st_name = (UInt32)strtab_intern(&strtab_buf, rtbe->name),
                    .st_info = ELF_SYM_INFO(kELFSymbolTableBindingGlobal, kELFSymbolTableTypeNoType),
                    .st_other = kELFSymbolVisibilityDefault,
                    .st_shndx = kELFSectionHeaderNumberUndefined,
                    .st_value = 0,
                    .st_size = 0,
                };
                sym_idx = (UInt32)(sym_buf.len / sizeof(ELF64_Sym));
                buf_append(&sym_buf, &usym, sizeof(usym));
            }
        }

        size_t byte_pos = rtbe->byte_pos;

        ELF64_Rela rela = {
            .r_info = ELF64_R_INFO(sym_idx, R_EMEX64_ABS64),
            .r_addend = 0,
        };

        if(inv->data_section_start != UINT64_MAX && byte_pos >= (size_t)inv->data_section_start && byte_pos <  (size_t)data_end_raw)
        {
            rela.r_offset = byte_pos - inv->data_section_start;
            buf_append(&rela_data_buf, &rela, sizeof(rela));
        }
        else if(byte_pos >= text_start)
        {
            rela.r_offset = byte_pos - text_start;
            buf_append(&rela_text_buf, &rela, sizeof(rela));
        }

        rtbe = rtbe->next;
    }

    buf_t shstrtab_buf = {0};
    buf_append_u8(&shstrtab_buf, 0);

    UInt32 shname_null = 0;
    UInt32 shname_text = (UInt32)strtab_intern(&shstrtab_buf, ".text");
    UInt32 shname_data = (UInt32)strtab_intern(&shstrtab_buf, ".data");
    UInt32 shname_bss = (UInt32)strtab_intern(&shstrtab_buf, ".bss");
    UInt32 shname_rela_text = (UInt32)strtab_intern(&shstrtab_buf, ".rela.text");
    UInt32 shname_rela_data = (UInt32)strtab_intern(&shstrtab_buf, ".rela.data");
    UInt32 shname_symtab = (UInt32)strtab_intern(&shstrtab_buf, ".symtab");
    UInt32 shname_strtab = (UInt32)strtab_intern(&shstrtab_buf, ".strtab");
    UInt32 shname_shstrtab = (UInt32)strtab_intern(&shstrtab_buf, ".shstrtab");

    size_t ehdr_size = sizeof(ELF64_Ehdr);
    size_t text_off = ehdr_size;
    size_t data_off = text_off + text_size;
    size_t rela_text_off = data_off + data_size;
    size_t rela_data_off = rela_text_off + rela_text_buf.len;
    size_t sym_off = rela_data_off + rela_data_buf.len;
    size_t str_off = sym_off + sym_buf.len;
    size_t shstr_off = str_off + strtab_buf.len;
    size_t shdr_off = shstr_off + shstrtab_buf.len;

    if(vfd_truncate(d, 0) != 0)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "elf_emit: ftruncate failed");
        goto done;
    }
    if(vfd_seek(d, 0, SEEK_SET) < 0)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "elf_emit: lseek failed");
        goto done;
    }

#define WRITE_BUF(buf, len) do { \
    if(vfd_write(d, (buf), (len)) != (ssize_t)(len)) \
    { \
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "elf_emit: write failed"); \
        goto done; \
    } \
} while(0)

    /* da header ^~^ */
    /* yeah sowwy for MachO, I use ELF cause it has better documentation :3 */
    ELF64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    memcpy(ehdr.e_ident, ident, EI_NIDENT);
    ehdr.e_type = kELFTypeRel;
    ehdr.e_machine = ELF_MAGIC_EMEX64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = 0;
    ehdr.e_phoff = 0;
    ehdr.e_shoff = (UInt64)shdr_off;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(ELF64_Ehdr);
    ehdr.e_phentsize = 0;
    ehdr.e_phnum = 0;
    ehdr.e_shentsize = sizeof(ELF64_Shdr);
    ehdr.e_shnum = kELFSectionHeaderIndexCount;
    ehdr.e_shstrndx = kELFSectionHeaderIndexShstrtab;
    WRITE_BUF(&ehdr, sizeof(ehdr));

    /* section data */
    if(text_size > 0)
    {
        WRITE_BUF(text_bytes, text_size);
    }
    if(data_size > 0)
    {
        WRITE_BUF(data_bytes, data_size);
    }

    /* bss: no file bytes */
    if(rela_text_buf.len > 0)
    {
        WRITE_BUF(rela_text_buf.data, rela_text_buf.len);
    }
    if(rela_data_buf.len > 0)
    {
        WRITE_BUF(rela_data_buf.data, rela_data_buf.len);
    }
    WRITE_BUF(sym_buf.data, sym_buf.len);
    WRITE_BUF(strtab_buf.data, strtab_buf.len);
    WRITE_BUF(shstrtab_buf.data, shstrtab_buf.len);

    /* section headers */
    ELF64_Shdr shdrs[kELFSectionHeaderIndexCount];
    memset(shdrs, 0, sizeof(shdrs));

    shdrs[kELFSectionHeaderIndexNull].sh_name = shname_null;
    shdrs[kELFSectionHeaderIndexNull].sh_type = kELFSectionHeaderTypeProgbits;

    /* [1] .text */
    shdrs[kELFSectionHeaderIndexText].sh_name = shname_text;
    shdrs[kELFSectionHeaderIndexText].sh_type = kELFSectionHeaderTypeProgbits;
    shdrs[kELFSectionHeaderIndexText].sh_flags = kELFSectionFlagAlloc | kELFSectionFlagExec;
    shdrs[kELFSectionHeaderIndexText].sh_addr = 0;
    shdrs[kELFSectionHeaderIndexText].sh_offset = (UInt64)text_off;
    shdrs[kELFSectionHeaderIndexText].sh_size = (UInt64)text_size;
    shdrs[kELFSectionHeaderIndexText].sh_addralign = 1;

    /* [2] .data */
    shdrs[kELFSectionHeaderIndexData].sh_name = shname_data;
    shdrs[kELFSectionHeaderIndexData].sh_type = kELFSectionHeaderTypeProgbits;
    shdrs[kELFSectionHeaderIndexData].sh_flags = kELFSectionFlagAlloc | kELFSectionFlagWrite;
    shdrs[kELFSectionHeaderIndexData].sh_addr = 0;
    shdrs[kELFSectionHeaderIndexData].sh_offset = (UInt64)data_off;
    shdrs[kELFSectionHeaderIndexData].sh_size = (UInt64)data_size;
    shdrs[kELFSectionHeaderIndexData].sh_addralign = 1;

    /* [3] .bss */
    shdrs[kELFSectionHeaderIndexBSS].sh_name = shname_bss;
    shdrs[kELFSectionHeaderIndexBSS].sh_type = kELFSectionHeaderTypeNobits;
    shdrs[kELFSectionHeaderIndexBSS].sh_flags = kELFSectionFlagAlloc | kELFSectionFlagWrite;
    shdrs[kELFSectionHeaderIndexBSS].sh_addr = 0;
    shdrs[kELFSectionHeaderIndexBSS].sh_offset = (UInt64)(data_off + data_size);
    shdrs[kELFSectionHeaderIndexBSS].sh_size = (UInt64)bss_size;
    shdrs[kELFSectionHeaderIndexBSS].sh_addralign = 1;

    /* [4] .rela.text */
    shdrs[kELFSectionHeaderIndexRelaText].sh_name = shname_rela_text;
    shdrs[kELFSectionHeaderIndexRelaText].sh_type = kELFSectionHeaderTypeRelative;
    shdrs[kELFSectionHeaderIndexRelaText].sh_flags = kELFSectionFlagAlloc;
    shdrs[kELFSectionHeaderIndexRelaText].sh_offset = (UInt64)rela_text_off;
    shdrs[kELFSectionHeaderIndexRelaText].sh_size = (UInt64)rela_text_buf.len;
    shdrs[kELFSectionHeaderIndexRelaText].sh_link = kELFSectionHeaderIndexSymtab;
    shdrs[kELFSectionHeaderIndexRelaText].sh_info = kELFSectionHeaderIndexText;
    shdrs[kELFSectionHeaderIndexRelaText].sh_addralign = 8;
    shdrs[kELFSectionHeaderIndexRelaText].sh_entsize = sizeof(ELF64_Rela);

    /* [5] .rela.data */
    shdrs[kELFSectionHeaderIndexRelaData].sh_name = shname_rela_data;
    shdrs[kELFSectionHeaderIndexRelaData].sh_type = kELFSectionHeaderTypeRelative;
    shdrs[kELFSectionHeaderIndexRelaData].sh_flags = kELFSectionFlagAlloc;
    shdrs[kELFSectionHeaderIndexRelaData].sh_offset = (UInt64)rela_data_off;
    shdrs[kELFSectionHeaderIndexRelaData].sh_size = (UInt64)rela_data_buf.len;
    shdrs[kELFSectionHeaderIndexRelaData].sh_link = kELFSectionHeaderIndexSymtab;
    shdrs[kELFSectionHeaderIndexRelaData].sh_info = kELFSectionHeaderIndexData;
    shdrs[kELFSectionHeaderIndexRelaData].sh_addralign = 8;
    shdrs[kELFSectionHeaderIndexRelaData].sh_entsize = sizeof(ELF64_Rela);

    /* [6] .symtab */
    shdrs[kELFSectionHeaderIndexSymtab].sh_name = shname_symtab;
    shdrs[kELFSectionHeaderIndexSymtab].sh_type = kELFSectionHeaderTypeSymtab;
    shdrs[kELFSectionHeaderIndexSymtab].sh_offset = (UInt64)sym_off;
    shdrs[kELFSectionHeaderIndexSymtab].sh_size = (UInt64)sym_buf.len;
    shdrs[kELFSectionHeaderIndexSymtab].sh_link = kELFSectionHeaderIndexStrtab;
    shdrs[kELFSectionHeaderIndexSymtab].sh_info = first_global;
    shdrs[kELFSectionHeaderIndexSymtab].sh_addralign = 8;
    shdrs[kELFSectionHeaderIndexSymtab].sh_entsize = sizeof(ELF64_Sym);

    /* [7] .strtab */
    shdrs[kELFSectionHeaderIndexStrtab].sh_name = shname_strtab;
    shdrs[kELFSectionHeaderIndexStrtab].sh_type = kELFSectionHeaderTypeStrtab;
    shdrs[kELFSectionHeaderIndexStrtab].sh_offset = (UInt64)str_off;
    shdrs[kELFSectionHeaderIndexStrtab].sh_size = (UInt64)strtab_buf.len;
    shdrs[kELFSectionHeaderIndexStrtab].sh_addralign = 1;

    /* [8] .shstrtab */
    shdrs[kELFSectionHeaderIndexShstrtab].sh_name = shname_shstrtab;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_type = kELFSectionHeaderTypeStrtab;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_offset = (UInt64)shstr_off;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_size = (UInt64)shstrtab_buf.len;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_addralign = 1;

    WRITE_BUF(shdrs, sizeof(shdrs));

    vfd_sync(d);
    ok = true;

done:
    free(flat);
    free(sym_buf.data);
    free(strtab_buf.data);
    free(shstrtab_buf.data);
    free(rela_text_buf.data);
    free(rela_data_buf.data);
    return ok;
}
