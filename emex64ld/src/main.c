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
#include <emex64lib/support/fdwalker.h>

#include <emex64lib/linker/linker.h>

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
                             fdwalker_t *fw)
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

            fdwalker_seek(fw, o->base_text + offset, 0);
            fdwalker_write(fw, value, 64);
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

            fdwalker_seek(fw, o->base_data + offset, 0);
            fdwalker_write(fw, value, 64);
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

static void emit_boot_header(fdwalker_t *fw,
                             uint64_t entry)
{
    fdwalker_seek(fw, 0, 0);
    fdwalker_write(fw, kEmex64OpcodeB, 8);
    fdwalker_write(fw, kEmex64ParameterCodingImm64, 3);
    fdwalker_write(fw, entry, 64);
}

int main(int argc, char *argv[])
{
    linker_options_t *options = linker_options_alloc();
    if(options == NULL)
    {
        return 1;
    }

    kEmitMode emit_mode = kEmitModeFirmware;
    char **input_file = calloc(argc, sizeof(char*));
    uint64_t input_file_cnt = 0;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            fprintf(stderr, "Usage: %s [-o output] [-e entry] [-T script.e64ld] file1.e64o [...]\n", argv[0]);
            fprintf(stderr, "  -o output        Output file (default: a.out)\n");
            fprintf(stderr, "  -e entry         Entry symbol (default: _start)\n");
            fprintf(stderr, "  -T script.e64ld  Linker script (or pass .e64ld files directly)\n");
            fprintf(stderr, "  .e64ld files are auto-detected by extension\n");
            fprintf(stderr, "  -v verbose       verbose mode\n");
            fprintf(stderr, "  -r               emits relocatable object\n");
            return 0;
        }
        else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            options->output_path = strdup(argv[++i]);
        }
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
        {
            options->entry_name = strdup(argv[++i]);
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
        else if(strcmp(argv[i], "-v") == 0)
        {
            options->verbose = true;
        }
        else if(strcmp(argv[i], "-r") == 0)
        {
            diag_error(NULL, "relocatable object emission is not supported yet\n");
            emit_mode = kEmitModeObject;
            return 1;
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
                input_file[input_file_cnt++] = argv[i];
            }
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
            return 1;
        }
    }

    if(emit_mode == kEmitModeNone)
    {
        diag_error(NULL, "no emit mode was set\n");
        return 1;
    }

    if(input_file_cnt <= 0)
    {
        diag_error(NULL, "no input files\n");
        return 1;
    }

    linker_invocation_t *inv = linker_invocation_alloc(options);
    if(inv == NULL)
    {
        return 1;
    }

    for(uint64_t i = 0; i < input_file_cnt; i++)
    {
        if(!linker_load_object(inv, input_file[i]))
        {
            diag_error(NULL, "object file \'%s\' couldn't be loaded\n", argv[i]);
            return 1;
        }
    }

    uint64_t total_text = inv->out_text_off - BOOT_HEADER_SIZE;
    uint64_t total_data = inv->out_data_off - inv->out_text_off;

    if(!apply_script_symbols(inv, inv->out_bss_off, BOOT_HEADER_SIZE, inv->out_text_off, inv->out_bss_off > inv->out_data_off ? inv->out_data_off : inv->out_bss_off))
    {
        return 1;
    }

    linker_object_t *obj = inv->obj;
    while(obj != NULL)
    {
        if(!obj_register_symbols(inv, obj))
        {
            return 1;
        }
        obj = obj->next;
    }

    emex_file_t *file = emex_file_alloc(linker_options_get_output_path(options), object_file_out_policy);
    if(file == NULL)
    {
        return 1;
    }

    fdwalker_t *fw = emex_file_dup_fdwalker(file, BW_LITTLE_ENDIAN);
    if(fw == NULL)
    {
        emex_file_dealloc(file);
        fdwalker_dealloc(fw);
        return 1;
    }

    /* copy .text sections */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(obj->idx_text < 0)
        {
            obj = obj->next;
            continue;
        }
        ELF64_Shdr *sh = &obj->shdrs[obj->idx_text];
        fdwalker_seek(fw, obj->base_text, 0);
        fdwalker_write_buf(fw, obj->file->content + sh->sh_offset, sh->sh_size);
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
        fdwalker_seek(fw, obj->base_data, 0);
        fdwalker_write_buf(fw, obj->file->content + sh->sh_offset, sh->sh_size);
        obj = obj->next;
    }

    /* .bss: already zeroed by calloc */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(!obj_apply_relocs(inv, obj, fw))
        {
            return 1;
        }
        obj = obj->next;
    }

    linker_global_symbol_t *gsym = linker_lookup_global_symbol(inv, linker_options_get_entry_name(options));
    if(!gsym || !gsym->defined)
    {
        diag_error(NULL, "entry symbol '%s' not found\n", linker_options_get_entry_name(options));
        return 1;
    }

    uint64_t entry_addr = gsym->addr;
    emit_boot_header(fw, entry_addr);

    fsync(fw->fd);
    fdwalker_dealloc(fw);

    if(options->verbose)
    {
        fprintf(stderr,
                "emex64ld: linked object(s) → %s\n"
                "  .text  %8lu bytes @ 0x%08lx\n"
                "  .data  %8lu bytes @ 0x%08lx\n"
                "  .bss   %8lu bytes @ 0x%08lx (virtual)\n"
                "  entry  %s @ 0x%08lx\n", linker_options_get_output_path(options),
                (unsigned long)total_text, (unsigned long)BOOT_HEADER_SIZE,
                (unsigned long)total_data, (unsigned long)inv->out_text_off,
                (unsigned long)(inv->out_bss_off - inv->out_data_off), (unsigned long)inv->out_data_off,
                linker_options_get_entry_name(options), (unsigned long)entry_addr);
    }

    linker_invocation_dealloc(inv);
    return 0;
}
