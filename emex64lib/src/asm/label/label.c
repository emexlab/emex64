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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/asm/label/label.h>
#include <emex64lib/asm/invocation.h>

assembler_label_t *assembler_label_lookup(assembler_invocation_t *inv,
                                          const char *name)
{
    return (assembler_label_t*)hashmap_gets(inv->label_hashmap, name);
}

bool assembler_label_append(assembler_token_t *at)
{
    /* validate label definition */
    bool failed_validity = false;
    if(at->al->type != kAssemblerLineTypeSectionData)
    {
        for(uint64_t i = at->al->type == kAssemblerLineTypeExternLabel ? 2 : 1; i < at->al->token_cnt; i++)
        {
            diag_error(at->al->token[i], "unknown token after label definition '%s'\n", at->al->token[i]->str);
            failed_validity = true;
        }
    }

    /* accessing compiler invocation */
    assembler_invocation_t *inv = at->al->inv;

    /* copying label name */
    char *name = NULL;
    switch(at->al->type)
    {
        case kAssemblerLineTypeLocalLabel:
        {
            if(inv->label_scope == NULL)
            {
                diag_error(at, "local label '%s' was defined out of the scope of a global label, which is illegal\n", at->str);
                return false;
            }

            /* constructing scoped label */
            size_t label_scope_len = strlen(inv->label_scope);
            size_t ct_len = strlen(at->str);
            size_t size = label_scope_len + ct_len;
            name = malloc(size);
            if(name == NULL)
            {
                diag_error(at, "failed to allocate memory for this label\n");
                return false;
            }

            memcpy(name, inv->label_scope, label_scope_len);
            memcpy(name + label_scope_len, at->str, ct_len - 1); /* minus 1 to ommit the ':' character */
            name[size - 1] = '\0';
            break;
        }
        case kAssemblerLineTypeGlobalLabel:
        {
            /* constructing global label */
            size_t size = strlen(at->str);
            name = malloc(size);
            if(name == NULL)
            {
                diag_error(at, "failed to allocate memory for this label\n");
                return false;
            }
            memcpy(name, at->str, size - 1);
            name[size - 1] = '\0';
            break;
        }
        case kAssemblerLineTypeSectionData:
        {
            /* constructing global label */
            size_t size = strlen(at->str) + 1;
            name = malloc(size);
            if(name == NULL)
            {
                diag_error(at, "failed to allocate memory for this label\n");
                return false;
            }
            memcpy(name, at->str, size - 1);
            name[size - 1] = '\0';
            break;
        }
        case kAssemblerLineTypeExternLabel:
        {
            /* constructing extern label */
            /* first we need the 2nd token, not the 1st */
            at = at->al->token[1];

            size_t size = strlen(at->str) + 1;
            name = malloc(size);
            if(name == NULL)
            {
                diag_error(at, "failed to allocate memory for this label\n");
                return false;
            }
            memcpy(name, at->str, size - 1);
            name[size - 1] = '\0';
            break;
        }
        default:
            diag_fatal(at, "this is not a label, report this at 'https://github.com/emexlab/emex64'\n");
            exit(1);
    }

    /* checking for duplicated labels */
    assembler_label_t *label = assembler_label_lookup(inv, name);
    if(label != NULL)
    {
        /* can be redeclared using 'extern' safely */
        if(at->al->type == kAssemblerLineTypeExternLabel)
        {
            free(name);
            return !failed_validity;
        }

        /* label can be defined after using 'extern' too */
        if(!label->defined)
        {
            if(at->al->type == kAssemblerLineTypeGlobalLabel)
            {
                /* set it as scope */
                inv->label_scope = label->name;
            }

            label->defined = true;
            label->at_link = at;
            label->addr = vbitwalker_bytes_used(inv->out_vbitwalker);
            free(name);
            return !failed_validity;
        }

        diag_note(label->at_link, "label '%s' already defined here\n", name);
        diag_error(at, "duplicated label '%s'\n", name);
        free(name);
        return false;
    }

    if(at->al->type == kAssemblerLineTypeGlobalLabel)
    {
        /* set it as scope */
        inv->label_scope = name;
    }

    label = calloc(1, sizeof(assembler_label_t));
    if(label == NULL)
    {
        diag_fatal(NULL, "out of memory, failed to allocate assembler label\n");
        free(name);
        return false;
    }

    label->addr = vbitwalker_bytes_used(inv->out_vbitwalker);
    label->at_link = at;
    label->defined = at->al->type != kAssemblerLineTypeExternLabel;
    label->name = name;
    if(!hashmap_puts(inv->label_hashmap, name, label))
    {
        diag_fatal(NULL, "out of memory, failed to insert label into hashmap\n");
        free(label);
        free(name);
        return false;
    }

    return !failed_validity;
}
