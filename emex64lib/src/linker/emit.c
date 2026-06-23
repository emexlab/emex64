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
#include <fcntl.h>
#include <unistd.h>
#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/linker/emit.h>
#include <emex64lib/linker/linker.h>

/*
 * barely ported start
 *
 * note: todo more we have to port those parts
 *       in the future too, for example for
 *       linker relaxation.
 */

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

        if(!linker_symbol_append_definition(inv, name, o->file->path, addr))
        {
            return false;
        }
    }
    return true;
}

static uint64_t sym_resolve(linker_invocation_t *inv,
                            const linker_object_t *o,
                            uint32_t sym_idx,
                            bool *success)
{
    *success = true;

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
        linker_symbol_t *gsym = linker_symbol_lookup(inv, name);
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
                return o->base_bss + sym->st_value;
            }
        }

        diag_error(NULL, "undefined symbol '%s', needed by '%s'\n", name, o->file->path);
        *success = false;
        return 0;
    }
    return 0;
}

static bool obj_apply_relocs(linker_invocation_t *inv,
                             const linker_object_t *o,
                             vbitwalker_t *vb)
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

            bool success = false;
            uint64_t sym_addr = sym_resolve(inv, o, sym_idx, &success);
            if(!success)
            {
                /* error printed by callee */
                return false;
            }
            uint64_t value = sym_addr + (uint64_t)addend;

            vbitwalker_seek(vb, o->base_text + offset, 0);
            vbitwalker_write(vb, value, 64);
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

            bool success = false;
            uint64_t sym_addr = sym_resolve(inv, o, sym_idx, &success);
            if(!success)
            {
                /* error printed by callee */
                return false;
            }
            uint64_t value = sym_addr + (uint64_t)addend;

            vbitwalker_seek(vb, o->base_data + offset, 0);
            vbitwalker_write(vb, value, 64);
        }
    }

    return true;
}

/* barely ported end */

static void emit_boot_header(vbitwalker_t *vb,
                             uint64_t entry)
{
    vbitwalker_seek(vb, 0, 0);
    vbitwalker_write(vb, kEmex64OpcodeB, 8);
    vbitwalker_write(vb, kEmex64ParameterCodingImm64, 3);
    vbitwalker_write(vb, entry, 64);
}

bool linker_link(linker_options_t options,
                 emex_file_t **input_file,
                 uint64_t input_file_cnt,
                 emex_file_t **linker_script_file,
                 uint64_t linker_script_file_cnt,
                 emex_file_t *output)
{
    linker_invocation_t *inv = linker_invocation_alloc(options);
    if(inv == NULL)
    {
        return false;
    }

    for(uint64_t i = 0; i < input_file_cnt; i++)
    {
        if(!linker_load_object(inv, input_file[i]))
        {
            diag_error(NULL, "object file \'%s\' couldn't be loaded\n", input_file[i]);
            linker_invocation_dealloc(inv);
            return false;
        }
    }

    for(uint64_t i = 0; i < linker_script_file_cnt; i++)
    {
        if(!linker_script_parse(inv, linker_script_file[i]))
        {
            diag_error(NULL, "linker script file \'%s\' is problematic\n", linker_script_file[i]);
            linker_invocation_dealloc(inv);
            return false;
        }
    }

    uint64_t total_text = inv->out_text_off - BOOT_HEADER_SIZE;
    uint64_t total_data = inv->out_data_off - inv->out_text_off;

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

    vbitwalker_t *vb = emex_file_dup_vbitwalker(output, BW_LITTLE_ENDIAN);
    if(vb == NULL)
    {
        linker_invocation_dealloc(inv);
        return false;
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
        vbitwalker_seek(vb, obj->base_text, 0);
        vbitwalker_write_buf(vb, obj->file->content + sh->sh_offset, sh->sh_size);
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
        vbitwalker_seek(vb, obj->base_data, 0);
        vbitwalker_write_buf(vb, obj->file->content + sh->sh_offset, sh->sh_size);
        obj = obj->next;
    }

    /* .bss: already zeroed by calloc */
    obj = inv->obj;
    while(obj != NULL)
    {
        if(!obj_apply_relocs(inv, obj, vb))
        {
            vbitwalker_dealloc(vb);
            linker_invocation_dealloc(inv);
            return false;
        }
        obj = obj->next;
    }

    linker_symbol_t *gsym = linker_symbol_lookup(inv, options.entry_name);
    if(!gsym || !gsym->defined)
    {
        diag_error(NULL, "entry symbol '%s' not found\n", options.entry_name);
        vbitwalker_dealloc(vb);
        linker_invocation_dealloc(inv);
        return false;
    }

    uint64_t entry_addr = gsym->addr;
    emit_boot_header(vb, entry_addr);

    vbitwalker_sync(vb);
    vbitwalker_dealloc(vb);

    if(options.verbose)
    {
        fprintf(stderr,
                "emex64ld: linked object(s) → %s\n"
                "  .text  %8lu bytes @ 0x%08lx\n"
                "  .data  %8lu bytes @ 0x%08lx\n"
                "  .bss   %8lu bytes @ 0x%08lx (virtual)\n"
                "  entry  %s @ 0x%08lx\n", output->path,
                (unsigned long)total_text, (unsigned long)BOOT_HEADER_SIZE,
                (unsigned long)total_data, (unsigned long)inv->out_text_off,
                (unsigned long)(inv->out_bss_off - inv->out_data_off), (unsigned long)inv->out_data_off,
                options.entry_name, (unsigned long)entry_addr);
    }

    linker_invocation_dealloc(inv);
    return true;
}
