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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#include <emex64lib/vm/core.h>

#include <emex64lib/support/diag.h>
#include <emex64lib/support/file.h>

#include <emex64lib/linker/linker.h>

static inline uint64_t obj_text_size(const linker_object_t *o)
{
    return o->idx_text >= 0 ? o->shdrs[o->idx_text].sh_size : 0;
}

static inline uint64_t obj_data_size(const linker_object_t *o)
{
    return o->idx_data >= 0 ? o->shdrs[o->idx_data].sh_size : 0;
}

static inline uint64_t obj_bss_size(const linker_object_t *o)
{
    return o->idx_bss >= 0 ? o->shdrs[o->idx_bss].sh_size : 0;
}

static bool obj_register_symbols(linker_invocation_t *inv,
                                 linker_object_t *o)
{
    if(o->idx_symtab < 0)
    {
        return true;
    }

    ELF64_Shdr *symsh = &o->shdrs[o->idx_symtab];
    ELF64_Sym *syms = (ELF64_Sym *)(o->file->content + symsh->sh_offset);
    size_t nsyms  = symsh->sh_size / sizeof(ELF64_Sym);
    const char *strtab = (o->idx_strtab >= 0) ? (char *)(o->file->content + o->shdrs[o->idx_strtab].sh_offset) : NULL;

    for(size_t i = 0; i < nsyms; i++)
    {
        ELF64_Sym *sym = &syms[i];
        uint8_t bind = sym->st_info >> 4;

        if(bind != kELFSymbolTableBindingGlobal)
        {
            continue;
        }
        if(sym->st_shndx == kELFSectionHeaderNumberUndefined)
        {
            continue;
        }
        if(!strtab)
        {
            continue;
        }

        const char *name = strtab + sym->st_name;
        if(!name || !*name)
        {
            continue;
        }

        uint64_t addr = 0;

        if(sym->st_shndx == kELFSectionHeaderNumberAbsolute)
        {
            addr = sym->st_value;
        }
        else if((int32_t)sym->st_shndx == o->idx_text)
        {
            addr = o->base_text + sym->st_value;
        }
        else if((int32_t)sym->st_shndx == o->idx_data)
        {
            addr = o->base_data + sym->st_value;
        }
        else if((int32_t)sym->st_shndx == o->idx_bss)
        {
            addr = o->base_bss  + sym->st_value;
        }
        else
        {
            addr = sym->st_value;
        }

        if(!linker_append_global_symbol_definition(inv, name, o->file->path, addr))
        {
            return false;
        }
    }
    return true;
}

static uint64_t sym_resolve(linker_invocation_t *inv,
                            const linker_object_t *o,
                            uint32_t sym_idx)
{
    if(o->idx_symtab < 0)
    {
        return 0;
    }

    ELF64_Shdr *symsh = &o->shdrs[o->idx_symtab];
    ELF64_Sym *syms = (ELF64_Sym *)(o->file->content + symsh->sh_offset);
    size_t nsyms = symsh->sh_size / sizeof(ELF64_Sym);
    const char *strtab = (o->idx_strtab >= 0) ? (char *)(o->file->content + o->shdrs[o->idx_strtab].sh_offset) : NULL;

    if(sym_idx >= nsyms)
    {
        return 0;
    }

    ELF64_Sym *sym = &syms[sym_idx];
    (void)(sym->st_info >> 4);

    if((sym->st_info & 0xf) == kELFSymbolTableTypeSection)
    {
        if((int32_t)sym->st_shndx == o->idx_text)
        {
            return o->base_text;
        }
        if((int32_t)sym->st_shndx == o->idx_data)
        {
            return o->base_data;
        }
        if((int32_t)sym->st_shndx == o->idx_bss)
        {
            return o->base_bss;
        }
        return 0;
    }

    if(strtab)
    {
        const char *name = strtab + sym->st_name;
        linker_global_symbol_t *gsym = linker_lookup_global_symbol(inv, name);
        if(gsym && gsym->defined)
        {
            return gsym->addr;
        }

        if(sym->st_shndx != kELFSectionHeaderNumberUndefined)
        {
            if((int32_t)sym->st_shndx == o->idx_text)
            {
                return o->base_text + sym->st_value;
            }
            if((int32_t)sym->st_shndx == o->idx_data)
            {
                return o->base_data + sym->st_value;
            }
            if((int32_t)sym->st_shndx == o->idx_bss)
            {
                return o->base_bss  + sym->st_value;
            }
        }

        diag_error(NULL, "undefined symbol '%s', needed by \"%s\"\n", name, o->file->path);
        exit(1); /* TODO: somehow make it not as strict, so it becomes embeddable */
    }
    return 0;
}

static bool obj_apply_relocs(linker_invocation_t *inv,
                             const linker_object_t *o,
                             uint8_t *out_text,
                             uint8_t *out_data)
{
    if(o->idx_rela_text >= 0)
    {
        ELF64_Shdr *rs = &o->shdrs[o->idx_rela_text];
        ELF64_Rela *rela = (ELF64_Rela *)(o->file->content + rs->sh_offset);
        size_t cnt = rs->sh_size / sizeof(ELF64_Rela);

        for(size_t i = 0; i < cnt; i++)
        {
            uint32_t type = (uint32_t)ELF32_R_TYPE(rela[i].r_info);
            uint32_t sym_idx = (uint32_t)ELF32_R_SYM(rela[i].r_info);
            uint64_t offset = rela[i].r_offset;
            int64_t  addend = rela[i].r_addend;

            if(type != R_EMEX64_ABS64)
            {
                diag_error(NULL, "unsupported relocation type %u in .rela.text\n", type);
                return false;
            }

            uint64_t sym_addr = sym_resolve(inv, o, sym_idx);
            uint64_t value = sym_addr + (uint64_t)addend;

            uint8_t *patch = out_text + offset;
            memcpy(patch, &value, 8);
        }
    }

    if(o->idx_rela_data >= 0)
    {
        ELF64_Shdr *rs = &o->shdrs[o->idx_rela_data];
        ELF64_Rela *rela = (ELF64_Rela *)(o->file->content + rs->sh_offset);
        size_t cnt = rs->sh_size / sizeof(ELF64_Rela);

        for(size_t i = 0; i < cnt; i++)
        {
            uint32_t type = (uint32_t)ELF32_R_TYPE(rela[i].r_info);
            uint32_t sym_idx = (uint32_t)ELF32_R_SYM(rela[i].r_info);
            uint64_t offset = rela[i].r_offset;
            int64_t addend = rela[i].r_addend;

            if(type != R_EMEX64_ABS64)
            {
                diag_error(NULL, "unsupported relocation type %u in .rela.data\n", type);
                return false;
            }

            uint64_t sym_addr = sym_resolve(inv, o, sym_idx);
            uint64_t value = sym_addr + (uint64_t)addend;

            uint8_t *patch = out_data + offset;
            memcpy(patch, &value, 8);
        }
    }

    return true;
}

typedef struct {
    const char *script_path;
    char *name;
    char *expr;
} script_sym_t;

static script_sym_t *script_syms = NULL;
static size_t script_sym_cnt = 0;

static bool parse_linker_script(const char *path)
{
    FILE *f = fopen(path, "r");
    if(!f)
    {
        diag_error(NULL, "cannot open linker script '%s': %s\n", path, strerror(errno));
        return false;
    }

    char line[1024];
    int lineno = 0;
    while(fgets(line, sizeof(line), f))
    {
        lineno++;
        char *comment = strchr(line, '#');
        if(comment)
        {
            *comment = '\0';
        }
        char *end = line + strlen(line);
        while(end > line && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' '  || end[-1] == '\t'))
        {
            *--end = '\0';
        }

        char *p = line;
        while(*p == ' ' || *p == '\t')
        {
            p++;
        }
        if(!*p)
        {
            continue;
        }

        if(strncmp(p, "PROVIDE", 7) == 0 && (p[7] == ' ' || p[7] == '\t'))
        {
            p += 7;
            while(*p == ' ' || *p == '\t')
            {
                p++;
            }

            /* symbol name */
            char *name_start = p;
            while(*p && *p != '=' && *p != ' ' && *p != '\t')
            {
                p++;
            }
            size_t name_len = (size_t)(p - name_start);
            if(name_len == 0)
            {
                diag_error(NULL, "%s:%d: expected symbol name after PROVIDE\n", path, lineno);
                fclose(f);
                return false;
            }
            char *sym_name = malloc(name_len + 1);
            memcpy(sym_name, name_start, name_len);
            sym_name[name_len] = '\0';

            while(*p == ' ' || *p == '\t')
            {
                p++;
            }
            if(*p != '=')
            {
                diag_error(NULL, "%s:%d: expected '=' after symbol name\n", path, lineno);
                free(sym_name);
                fclose(f);
                return false;
            }
            p++;
            while(*p == ' ' || *p == '\t')
            {
                p++;
            }

            char *expr_start = p;
            char *semi = strchr(p, ';');
            if(semi)
            {
                *semi = '\0';
            }
            end = p + strlen(p);
            while(end > p && (end[-1] == ' ' || end[-1] == '\t'))
            {
                *--end = '\0';
            }

            if(!*expr_start)
            {
                diag_error(NULL, "%s:%d: empty expression\n", path, lineno);
                free(sym_name);
                fclose(f);
                return false;
            }

            script_syms = realloc(script_syms, (script_sym_cnt + 1) * sizeof(script_sym_t));
            script_syms[script_sym_cnt].name = sym_name;
            script_syms[script_sym_cnt].expr = strdup(expr_start);
            script_syms[script_sym_cnt].script_path = path;
            script_sym_cnt++;
            continue;
        }

        diag_error(NULL, "%s:%d: unrecognised linker script directive: '%s'\n", path, lineno, p);
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

static bool apply_script_symbols(linker_invocation_t *inv,
                                 uint64_t image_end,
                                 uint64_t text_start,
                                 uint64_t data_start,
                                 uint64_t bss_start)
{
    for(size_t i = 0; i < script_sym_cnt; i++)
    {
        const char *expr = script_syms[i].expr;
        uint64_t value = 0;

        if(strcmp(expr, "IMAGE_END") == 0)
        {
            value = image_end;
        }
        else if(strcmp(expr, "IMAGE_START") == 0)
        {
            value = 0;
        }
        else if(strcmp(expr, "TEXT_START") == 0)
        {
            value = text_start;
        }
        else if(strcmp(expr, "DATA_START") == 0)
        {
            value = data_start;
        }
        else if(strcmp(expr, "BSS_START") == 0)
        {
            value = bss_start;
        }
        else
        {
            /* parse hex / decimal number */
            char *endptr = NULL;
            value = (uint64_t)strtoull(expr, &endptr, 0);
            if(!endptr || *endptr != '\0')
            {
                diag_error(NULL, "unknown expression '%s' in linker script\n", expr);
                return false;
            }
        }

        if(!linker_append_global_symbol_definition(inv, script_syms[i].name, script_syms[i].script_path, value))
        {
            return false;
        }
    }
    return true;
}

static void write_le_u64(uint8_t *buf, uint64_t v)
{
    for(int i = 0; i < 8; i++)
    {
        buf[i] = v & 0xff;
        v >>= 8;
    }
}

static void emit_boot_header(uint8_t hdr[10], uint64_t entry)
{
    memset(hdr, 0, 10);

    /* b <start sym> */
    hdr[0] = kEmex64OpcodeB;
    uint8_t coding = (uint8_t)(kEmex64ParameterCodingImm64 & 0x7); /* 0b110 = 6 */
    uint64_t payload_lo = (uint64_t)coding | (entry << 3);
    uint64_t payload_hi = entry >> 61;
    write_le_u64(hdr + 1, payload_lo);
    hdr[9] = (uint8_t)(payload_hi & 0x7);
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-o output] [-e entry] [-T script.e64ld] file1.e64o [...]\n", prog);
    fprintf(stderr, "  -o output        Output file (default: a.out)\n");
    fprintf(stderr, "  -e entry         Entry symbol (default: _start)\n");
    fprintf(stderr, "  -T script.e64ld  Linker script (or pass .e64ld files directly)\n");
    fprintf(stderr, "  .e64ld files are auto-detected by extension\n");
    fprintf(stderr, "  -v verbose       verbose mode\n");
}

int main(int argc, char *argv[])
{
    bool verbose = false;
    const char *output_path = "a.out";
    const char *entry_name = "_start";
    int file_count   = 0;
    const char **input_files = calloc((size_t)argc, sizeof(char *));

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            output_path = argv[++i];
        }
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
        {
            entry_name = argv[++i];
        }
        else if((strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--script") == 0) && i + 1 < argc)
        {
            if(!parse_linker_script(argv[++i]))
            {
                return 1;
            }
        }
        else if (strncmp(argv[i], "-T", 2) == 0 && argv[i][2])
        {
            if(!parse_linker_script(argv[i] + 2))
            {
                return 1;
            }
        }
        else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if(strcmp(argv[i], "-v") == 0)
        {
            verbose = true;
        }
        else if (argv[i][0] != '-')
        {
            size_t n = strlen(argv[i]);
            if(n > 5 && strcmp(argv[i] + n - 5, ".e64ld") == 0)
            {
                if(!parse_linker_script(argv[i]))
                {
                    return 1;
                }
            }
            else
            {
                input_files[file_count++] = argv[i];
            }
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if(file_count == 0)
    {
        diag_error(NULL, "no input files\n");
        usage(argv[0]);
        return 1;
    }

    linker_invocation_t *inv = linker_invocation_alloc();
    if(inv == NULL)
    {
        return 1;
    }

    uint64_t cur_text = BOOT_HEADER_SIZE;
    uint64_t cur_data = 0;
    uint64_t cur_bss = 0;

    for(int i = 0; i < file_count; i++)
    {
        if(!linker_load_object(inv, input_files[i]))
        {
            return 1;
        }

        inv->obj->base_text = cur_text;
        cur_text += obj_text_size(inv->obj);
    }

    cur_data = cur_text;
    linker_object_t *obj = inv->obj;
    while(obj != NULL)
    {
        obj->base_data = cur_data;
        cur_data += obj_data_size(obj);
        obj = obj->next;
    }

    cur_bss = cur_data;
    obj = inv->obj;
    while(obj != NULL)
    {
        obj->base_bss = cur_bss;
        cur_bss += obj_bss_size(obj);
        obj = obj->next;
    }

    uint64_t total_text = cur_text - BOOT_HEADER_SIZE;
    uint64_t total_data = cur_data - cur_text;
    uint64_t image_size = BOOT_HEADER_SIZE + total_text + total_data;

    if(!apply_script_symbols(inv, cur_bss, BOOT_HEADER_SIZE, cur_text, cur_bss > cur_data ? cur_data : cur_bss))
    {
        return 1;
    }

    obj = inv->obj;
    while(obj != NULL)
    {
        if(!obj_register_symbols(inv, obj))
        {
            return 1;
        }
        obj = obj->next;
    }

    uint8_t *image = calloc(image_size, 1);
    if(!image)
    {
        perror("malloc");
        return 1;
    }

    /* copy .text sections */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(obj->idx_text < 0)
        {
            continue;
        }
        ELF64_Shdr *sh = &obj->shdrs[obj->idx_text];
        uint64_t dst_off = obj->base_text;
        memcpy(image + dst_off, obj->file->content + sh->sh_offset, sh->sh_size);
        obj = obj->next;
    }

    /* copy .data sections */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(obj->idx_data < 0)
        {
            continue;
        }
        ELF64_Shdr *sh = &obj->shdrs[obj->idx_data];
        uint64_t dst_off = obj->base_data;
        memcpy(image + dst_off, obj->file->content + sh->sh_offset, sh->sh_size);
        obj = obj->next;
    }

    /* .bss: already zeroed by calloc */
    obj = inv->obj;
    while(obj != NULL)
    {
        uint8_t *obj_text_ptr = image + obj->base_text;
        uint8_t *obj_data_ptr = image + obj->base_data;
        if(!obj_apply_relocs(inv, obj, obj_text_ptr, obj_data_ptr))
        {
            return 1;
        }
        obj = obj->next;
    }

    linker_global_symbol_t *gsym = linker_lookup_global_symbol(inv, entry_name);
    if(!gsym || !gsym->defined)
    {
        diag_error(NULL, "entry symbol '%s' not found\n", entry_name);
        return 1;
    }

    uint64_t entry_addr = gsym->addr;
    uint8_t boot_hdr[10];
    emit_boot_header(boot_hdr, entry_addr);
    memcpy(image, boot_hdr, 10);

    int fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if(fd < 0)
    {
        perror(output_path);
        return 1;
    }

    ssize_t written = write(fd, image, image_size);
    if(written != (ssize_t)image_size)
    {
        diag_error(NULL, "write error: %s\n", output_path);
        close(fd);
        return 1;
    }

    fsync(fd);
    close(fd);

    if(verbose)
    {
        fprintf(stderr,
                "emex64ld: linked %d object(s) → %s\n"
                "  .text  %8lu bytes @ 0x%08lx\n"
                "  .data  %8lu bytes @ 0x%08lx\n"
                "  .bss   %8lu bytes @ 0x%08lx (virtual)\n"
                "  entry  %s @ 0x%08lx\n",
                file_count, output_path,
                (unsigned long)total_text, (unsigned long)BOOT_HEADER_SIZE,
                (unsigned long)total_data, (unsigned long)cur_text,
                (unsigned long)(cur_bss - cur_data), (unsigned long)cur_data,
                entry_name, (unsigned long)entry_addr);
    }

    free(image);
    free(input_files);
    linker_invocation_dealloc(inv);
    return 0;
}
