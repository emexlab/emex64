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

#include <stdlib.h>
#include <string.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETLinker/linker.h>
#include <EmexToolchain/ETLinker/obj.h>
#include <EmexToolchain/VM/E64Memory.h>

static unsigned long obj_sec_align(linker_object_t *o, SInt32 idx)
{
    if(idx < 0)
    {
        return 1;
    }
    unsigned long a = o->shdrs[idx].sh_addralign;
    return a < 2 ? 1 : a;
}

static inline unsigned long align_up(unsigned long v, unsigned long a)
{
    return (v + a - 1) & ~(a - 1);
}

linker_object_t *linker_object_alloc(emex_file_t *object_file)
{
    linker_object_t *obj = calloc(1, sizeof(linker_object_t));

    /* setting to -1 as a sentinel */
    obj->idx_text = -1;
    obj->idx_data = -1;
    obj->idx_bss = -1;
    obj->idx_rela_text = -1;
    obj->idx_rela_data = -1;
    obj->idx_symtab = -1;
    obj->idx_strtab = -1;

    obj->file = object_file;

    if(!emex_file_map(obj->file))
    {
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    if(obj->file->len < sizeof(ELF64_Shdr))
    {
        //diag_error(NULL, "%s: too small to be ELF\n", obj->file->path);
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    obj->ehdr = (ELF64_Ehdr *)obj->file->content;

    if(obj->ehdr->e_ident[0] != ELF_MAGIC_0 ||
       obj->ehdr->e_ident[1] != ELF_MAGIC_1 ||
       obj->ehdr->e_ident[2] != ELF_MAGIC_2 ||
       obj->ehdr->e_ident[3] != ELF_MAGIC_3)
    {
        //diag_error(NULL, "%s: not an ELF file\n", obj->file->path);
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    if(obj->ehdr->e_machine != ELF_MAGIC_EMEX64)
    {
        //diag_error(NULL, "%s: not an emex64 object (e_machine=0x%x)\n", obj->file->path, obj->ehdr->e_machine);
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    if(obj->ehdr->e_type != kELFTypeRel)
    {
        //diag_error(NULL, "%s: not a relocatable object\n", obj->file->path);
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    obj->shdrs = (ELF64_Shdr *)(obj->file->content + obj->ehdr->e_shoff);

    if(obj->ehdr->e_shstrndx != 0xFFFF &&
       obj->ehdr->e_shstrndx < obj->ehdr->e_shnum)
    {
        ELF64_Shdr *ss = &obj->shdrs[obj->ehdr->e_shstrndx];
        obj->shstrtab = (char *)(obj->file->content + ss->sh_offset);
    }

    /* find known sections */
    for(UInt16 i = 0; i < obj->ehdr->e_shnum; i++)
    {
        if(!obj->shstrtab)
        {
            continue;
        }
        const char *name = obj->shstrtab + obj->shdrs[i].sh_name;

        if(strcmp(name, ".text") == 0)
        {
            obj->idx_text = i;
        }
        else if(strcmp(name, ".data") == 0)
        {
            obj->idx_data = i;
        }
        else if(strcmp(name, ".bss") == 0)
        {
            obj->idx_bss = i;
        }
        else if(strcmp(name, ".rela.text") == 0)
        {
            obj->idx_rela_text = i;
        }
        else if(strcmp(name, ".rela.data") == 0)
        {
            obj->idx_rela_data = i;
        }
        else if(obj->shdrs[i].sh_type == kELFSectionHeaderTypeSymtab)
        {
            obj->idx_symtab = i;
        }
    }

    if(obj->idx_symtab >= 0)
    {
        obj->idx_strtab = (SInt32)obj->shdrs[obj->idx_symtab].sh_link;
    }

    return obj;
}

void linker_object_dealloc(linker_object_t *obj)
{
    free(obj);
}

Boolean linker_load_object(linker_invocation_t *inv,
                           emex_file_t *object_file)
{
    /* load object */
    linker_object_t *obj = linker_object_alloc(object_file);
    if(obj == NULL)
    {
        return false;
    }

    /* stiching object into the linked list ^^ */
    if(inv->obj == NULL)
    {
        inv->obj = obj;
    }
    else
    {
        linker_object_t *tail = inv->obj;
        while(tail->next)
        {
            tail = tail->next;
        }
        tail->next = obj;
    }

    return true;
}

void linker_layout(linker_invocation_t *inv)
{
    unsigned long cur = BOOT_HEADER_SIZE;

    for(linker_object_t *o = inv->obj; o; o = o->next)
    {
        cur = align_up(cur, obj_sec_align(o, o->idx_text));
        o->base_text = cur;
        cur += linker_object_text_size(o);
    }

    if(!inv->options.use_old_magic)
    {
        cur = align_up(cur, EMEX64_PAGE_SIZE);
    }
    inv->out_text_off = cur;

    for(linker_object_t *o = inv->obj; o; o = o->next)
    {
        cur = align_up(cur, obj_sec_align(o, o->idx_data));
        o->base_data = cur;
        cur += linker_object_data_size(o);
    }
    cur = align_up(cur, EMEX64_PAGE_SIZE);
    inv->out_data_off = cur;

    for(linker_object_t *o = inv->obj; o; o = o->next)
    {
        cur = align_up(cur, obj_sec_align(o, o->idx_bss));
        o->base_bss = cur;
        cur += linker_object_bss_size(o);
    }
    cur = align_up(cur, EMEX64_PAGE_SIZE);
    inv->out_bss_off = cur;
}
