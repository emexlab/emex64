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

#include <stdlib.h>
#include <string.h>
#include <string.h>

#include <emex64lib/support/diag.h>

#include <emex64lib/linker/obj.h>

linker_object_t *linker_object_alloc(const char *object_path)
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

    obj->file = emex_file_alloc(object_path, object_file_load_policy);
    if(obj->file == NULL)
    {
        free(obj);
        return NULL;
    }

    if(!emex_file_map(obj->file))
    {
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    if(obj->file->len < sizeof(ELF64_Shdr))
    {
        diag_error(NULL, "%s: too small to be ELF\n", object_path);
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
        diag_error(NULL, "%s: not an ELF file\n", object_path);
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    if(obj->ehdr->e_machine != ELF_MAGIC_EMEX64)
    {
        diag_error(NULL, "%s: not an emex64 object (e_machine=0x%x)\n", object_path, obj->ehdr->e_machine);
        emex_file_dealloc(obj->file);
        free(obj);
        return NULL;
    }

    if(obj->ehdr->e_type != kELFTypeRel)
    {
        diag_error(NULL, "%s: not a relocatable object\n", object_path);
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
    for(uint16_t i = 0; i < obj->ehdr->e_shnum; i++)
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
        obj->idx_strtab = (int32_t)obj->shdrs[obj->idx_symtab].sh_link;
    }

    return obj;
}

void linker_object_dealloc(linker_object_t *obj)
{
    emex_file_dealloc(obj->file);
    free(obj);
}
