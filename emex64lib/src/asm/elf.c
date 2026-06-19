/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/fdwalker.h>
#include <emex64lib/support/parser.h>

#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/opcode.h>
#include <emex64lib/asm/register.h>
#include <emex64lib/asm/label.h>
#include <emex64lib/asm/emit.h>
#include <emex64lib/asm/section.h>

#include <emex64lib/linker/linker.h>

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} buf_t;

static bool buf_reserve(buf_t *b, size_t extra)
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
    uint8_t *nd = realloc(b->data, ncap);
    if(!nd)
    {
        return false;
    }
    b->data = nd;
    b->cap  = ncap;
    return true;
}

static bool buf_append(buf_t *b, const void *src, size_t n)
{
    if(!buf_reserve(b, n))
    {
        return false;
    }
    memcpy(b->data + b->len, src, n);
    b->len += n;
    return true;
}

static bool buf_append_u8(buf_t *b, uint8_t v)
{
    return buf_append(b, &v, 1);
}

static bool __attribute__((unused)) buf_append_u64(buf_t *b, uint64_t v)
{
    /* little-endian */
    uint8_t tmp[8];
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

bool assembler_elf_emit(assembler_invocation_t *inv)
{
    bool ok = false;

    vfd_t *d = inv->fdwalker->d;

    struct stat st;
    if(vfd_stat(d, &st) != 0)
    {
        diag_error(NULL, "elf_emit: fstat failed\n");
        return false;
    }

    size_t flat_size = (size_t)st.st_size;
    uint8_t *flat = malloc(flat_size);
    if(!flat)
    {
        diag_error(NULL, "elf_emit: out of memory\n");
        return false;
    }

    if(vfd_seek(d, 0, SEEK_SET) < 0 || vfd_read(d, flat, flat_size) != (ssize_t)flat_size)
    {
        diag_error(NULL, "elf_emit: read flat binary failed\n");
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

    const uint8_t *text_bytes = flat + text_start;
    const uint8_t *data_bytes = flat + data_start;

    buf_t sym_buf = {0};
    buf_t strtab_buf = {0};

    buf_append_u8(&strtab_buf, 0);
    ELF64_Sym sym0 = {0};
    buf_append(&sym_buf, &sym0, sizeof(sym0));

    const char *src_fname = (inv->file_cnt > 0) ? inv->file[0] : "<unknown>";
    const char *base = strrchr(src_fname, '/');
    base = base ? base + 1 : src_fname;

    ELF64_Sym sym_file = {
        .st_name = (uint32_t)strtab_intern(&strtab_buf, base),
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

    uint32_t first_global = (uint32_t)(sym_buf.len / sizeof(ELF64_Sym));

    for(uint64_t i = 0; i < inv->label_cnt; i++)
    {
        assembler_label_t *lbl = &inv->label[i];
        if(!lbl->name)
        {
            continue;
        }

        uint64_t addr = lbl->addr;
        uint16_t shndx;
        uint64_t st_value;

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
            .st_name = (uint32_t)strtab_intern(&strtab_buf, lbl->name),
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
        uint32_t sym_idx = 0;
        {
            size_t n = sym_buf.len / sizeof(ELF64_Sym);
            ELF64_Sym *syms = (ELF64_Sym *)sym_buf.data;
            for(size_t s = first_global; s < n; s++)
            {
                const char *sname = (char*)(strtab_buf.data + syms[s].st_name);
                if(strcmp(sname, rtbe->name) == 0)
                {
                    sym_idx = (uint32_t)s;
                    break;
                }
            }
            if(sym_idx == 0)
            {
                ELF64_Sym usym = {
                    .st_name = (uint32_t)strtab_intern(&strtab_buf, rtbe->name),
                    .st_info = ELF_SYM_INFO(kELFSymbolTableBindingGlobal, kELFSymbolTableTypeNoType),
                    .st_other = kELFSymbolVisibilityDefault,
                    .st_shndx = kELFSectionHeaderNumberUndefined,
                    .st_value = 0,
                    .st_size = 0,
                };
                sym_idx = (uint32_t)(sym_buf.len / sizeof(ELF64_Sym));
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

    uint32_t shname_null = 0;
    uint32_t shname_text = (uint32_t)strtab_intern(&shstrtab_buf, ".text");
    uint32_t shname_data = (uint32_t)strtab_intern(&shstrtab_buf, ".data");
    uint32_t shname_bss = (uint32_t)strtab_intern(&shstrtab_buf, ".bss");
    uint32_t shname_rela_text = (uint32_t)strtab_intern(&shstrtab_buf, ".rela.text");
    uint32_t shname_rela_data = (uint32_t)strtab_intern(&shstrtab_buf, ".rela.data");
    uint32_t shname_symtab = (uint32_t)strtab_intern(&shstrtab_buf, ".symtab");
    uint32_t shname_strtab = (uint32_t)strtab_intern(&shstrtab_buf, ".strtab");
    uint32_t shname_shstrtab = (uint32_t)strtab_intern(&shstrtab_buf, ".shstrtab");

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
        diag_error(NULL, "elf_emit: ftruncate failed\n");
        goto done;
    }
    if(vfd_seek(d, 0, SEEK_SET) < 0)
    {
        diag_error(NULL, "elf_emit: lseek failed\n");
        goto done;
    }

#define WRITE_BUF(buf, len) do { \
    if(vfd_write(d, (buf), (len)) != (ssize_t)(len)) \
    { \
        diag_error(NULL, "elf_emit: write failed\n"); \
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
    ehdr.e_shoff = (uint64_t)shdr_off;
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
    shdrs[kELFSectionHeaderIndexText].sh_offset = (uint64_t)text_off;
    shdrs[kELFSectionHeaderIndexText].sh_size = (uint64_t)text_size;
    shdrs[kELFSectionHeaderIndexText].sh_addralign = 1;

    /* [2] .data */
    shdrs[kELFSectionHeaderIndexData].sh_name = shname_data;
    shdrs[kELFSectionHeaderIndexData].sh_type = kELFSectionHeaderTypeProgbits;
    shdrs[kELFSectionHeaderIndexData].sh_flags = kELFSectionFlagAlloc | kELFSectionFlagWrite;
    shdrs[kELFSectionHeaderIndexData].sh_addr = 0;
    shdrs[kELFSectionHeaderIndexData].sh_offset = (uint64_t)data_off;
    shdrs[kELFSectionHeaderIndexData].sh_size = (uint64_t)data_size;
    shdrs[kELFSectionHeaderIndexData].sh_addralign = 1;

    /* [3] .bss */
    shdrs[kELFSectionHeaderIndexBSS].sh_name = shname_bss;
    shdrs[kELFSectionHeaderIndexBSS].sh_type = kELFSectionHeaderTypeNobits;
    shdrs[kELFSectionHeaderIndexBSS].sh_flags = kELFSectionFlagAlloc | kELFSectionFlagWrite;
    shdrs[kELFSectionHeaderIndexBSS].sh_addr = 0;
    shdrs[kELFSectionHeaderIndexBSS].sh_offset = (uint64_t)(data_off + data_size);
    shdrs[kELFSectionHeaderIndexBSS].sh_size = (uint64_t)bss_size;
    shdrs[kELFSectionHeaderIndexBSS].sh_addralign = 1;

    /* [4] .rela.text */
    shdrs[kELFSectionHeaderIndexRelaText].sh_name = shname_rela_text;
    shdrs[kELFSectionHeaderIndexRelaText].sh_type = kELFSectionHeaderTypeRelative;
    shdrs[kELFSectionHeaderIndexRelaText].sh_flags = kELFSectionFlagAlloc;
    shdrs[kELFSectionHeaderIndexRelaText].sh_offset = (uint64_t)rela_text_off;
    shdrs[kELFSectionHeaderIndexRelaText].sh_size = (uint64_t)rela_text_buf.len;
    shdrs[kELFSectionHeaderIndexRelaText].sh_link = kELFSectionHeaderIndexSymtab;
    shdrs[kELFSectionHeaderIndexRelaText].sh_info = kELFSectionHeaderIndexText;
    shdrs[kELFSectionHeaderIndexRelaText].sh_addralign = 8;
    shdrs[kELFSectionHeaderIndexRelaText].sh_entsize = sizeof(ELF64_Rela);

    /* [5] .rela.data */
    shdrs[kELFSectionHeaderIndexRelaData].sh_name = shname_rela_data;
    shdrs[kELFSectionHeaderIndexRelaData].sh_type = kELFSectionHeaderTypeRelative;
    shdrs[kELFSectionHeaderIndexRelaData].sh_flags = kELFSectionFlagAlloc;
    shdrs[kELFSectionHeaderIndexRelaData].sh_offset = (uint64_t)rela_data_off;
    shdrs[kELFSectionHeaderIndexRelaData].sh_size = (uint64_t)rela_data_buf.len;
    shdrs[kELFSectionHeaderIndexRelaData].sh_link = kELFSectionHeaderIndexSymtab;
    shdrs[kELFSectionHeaderIndexRelaData].sh_info = kELFSectionHeaderIndexData;
    shdrs[kELFSectionHeaderIndexRelaData].sh_addralign = 8;
    shdrs[kELFSectionHeaderIndexRelaData].sh_entsize = sizeof(ELF64_Rela);

    /* [6] .symtab */
    shdrs[kELFSectionHeaderIndexSymtab].sh_name = shname_symtab;
    shdrs[kELFSectionHeaderIndexSymtab].sh_type = kELFSectionHeaderTypeSymtab;
    shdrs[kELFSectionHeaderIndexSymtab].sh_offset = (uint64_t)sym_off;
    shdrs[kELFSectionHeaderIndexSymtab].sh_size = (uint64_t)sym_buf.len;
    shdrs[kELFSectionHeaderIndexSymtab].sh_link = kELFSectionHeaderIndexStrtab;
    shdrs[kELFSectionHeaderIndexSymtab].sh_info = first_global;
    shdrs[kELFSectionHeaderIndexSymtab].sh_addralign = 8;
    shdrs[kELFSectionHeaderIndexSymtab].sh_entsize = sizeof(ELF64_Sym);

    /* [7] .strtab */
    shdrs[kELFSectionHeaderIndexStrtab].sh_name = shname_strtab;
    shdrs[kELFSectionHeaderIndexStrtab].sh_type = kELFSectionHeaderTypeStrtab;
    shdrs[kELFSectionHeaderIndexStrtab].sh_offset = (uint64_t)str_off;
    shdrs[kELFSectionHeaderIndexStrtab].sh_size = (uint64_t)strtab_buf.len;
    shdrs[kELFSectionHeaderIndexStrtab].sh_addralign = 1;

    /* [8] .shstrtab */
    shdrs[kELFSectionHeaderIndexShstrtab].sh_name = shname_shstrtab;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_type = kELFSectionHeaderTypeStrtab;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_offset = (uint64_t)shstr_off;
    shdrs[kELFSectionHeaderIndexShstrtab].sh_size = (uint64_t)shstrtab_buf.len;
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
