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
#include <ctype.h>
#include <unistd.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>

assembler_label_t *assembler_label_lookup(ETAssemblerInvocationRef inv,
                                          const char *name)
{
    return (assembler_label_t*)hashmap_gets(inv->label_hashmap, name);
}

Boolean assembler_label_is_symbol(assembler_token_t *at)
{
    switch(at->al->type)
    {
        case kETAssemblerLineTypeSymbol:
        case kETAssemblerLineTypeExternSymbol:
        case kETAssemblerLineTypeSectionData:
            return true;
        default:
            return false;
    }
}

Boolean assembler_label_append(assembler_token_t *at)
{
    /* accessing compiler invocation */
    ETAssemblerInvocationRef inv = at->al->inv;

    /* copying label name */
    char *name = NULL;
    switch(at->al->type)
    {
        case kETAssemblerLineTypeLabel:
        {
            if(inv->label_scope == NULL)
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(at), "label '%s' was defined out of the scope of a symbol, which is illegal", at->str);
                return false;
            }

            /* constructing scoped label */
            size_t label_scope_len = strlen(inv->label_scope);
            size_t ct_len = strlen(at->str);
            size_t size = label_scope_len + ct_len + 1;
            name = malloc(size);
            if(name == NULL)
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(at), "out of memory, failed to allocate memory for label definition '%s'", at->str);
                return false;
            }

            memcpy(name, inv->label_scope, label_scope_len);
            memcpy(name + label_scope_len, at->str, ct_len);    /* minus 1 to ommit the ':' character */
            name[size - 1] = '\0';
            break;
        }
        case kETAssemblerLineTypeSymbol:
        {
            /* constructing symbol */
            size_t size = strlen(at->str) + 1;
            name = malloc(size);
            if(name == NULL)
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(at), "out of memory, failed to allocate memory for symbol definition '%s'", at->str);
                return false;
            }
            memcpy(name, at->str, size);
            name[size - 1] = '\0';
            break;
        }
        case kETAssemblerLineTypeExternSymbol:
        {
            /* constructing extern symbol */
            /* first we need the 2nd token, not the 1st */
            at = at->al->token[1];

            size_t size = strlen(at->str) + 1;
            name = malloc(size);
            if(name == NULL)
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(at), "out of memory, failed to allocate memory for external symbol declaration '%s'", at->str);
                return false;
            }
            memcpy(name, at->str, size - 1);
            name[size - 1] = '\0';
            break;
        }
        case kETAssemblerLineTypeSectionData:
        {
            /* constructing symbol */
            size_t size = strlen(at->str) + 1;
            name = malloc(size);
            if(name == NULL)
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(at), "out of memory, failed to allocate memory for symbol definition '%s'", at->str);
                return false;
            }
            memcpy(name, at->str, size - 1);
            name[size - 1] = '\0';
            break;
        }
        default:
            diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(at), "this is not a label, report this at 'https://github.com/emexlab/emex64'");
            exit(1);
    }

    /* checking for duplicated labels */
    assembler_label_t *label = assembler_label_lookup(inv, name);
    if(label != NULL)
    {
        /* can be redeclared using 'extern' safely */
        if(at->al->type == kETAssemblerLineTypeExternSymbol)
        {
            label->global = true;
            free(name);
            return true;
        }

        /* label can be defined after using 'extern' too */
        if(!label->defined)
        {
            if(at->al->type == kETAssemblerLineTypeSymbol)
            {
                /* set it as scope */
                inv->label_scope = label->name;
            }

            /* 'extern' already made it global */
            label->defined = true;
            label->at_link = at;
            label->addr = (UInt64)EFBitWalkerBytesUsed(inv->out_vbitwalker);
            free(name);
            return true;
        }

        /* have to find out flavour */
        const char *label_string = assembler_label_is_symbol(label->at_link) ? "symbol" : "label";
        diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityNote, AT_TO_DLOC(label->at_link), "%s '%s' already defined here", label_string, name);
        diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(at), "duplicated %s '%s'", label_string, name); /* need to mirror cause of name missmatch possibilities */
        free(name);
        return false;
    }

    if(at->al->type == kETAssemblerLineTypeSymbol)
    {
        /* set it as scope */
        inv->label_scope = name;
    }

    label = calloc(1, sizeof(assembler_label_t));
    if(label == NULL)
    {
        diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory, failed to allocate assembler label");
        free(name);
        return false;
    }

    label->addr = (UInt64)EFBitWalkerBytesUsed(inv->out_vbitwalker);
    label->at_link = at;
    label->defined = at->al->type != kETAssemblerLineTypeExternSymbol;
    label->global = at->al->type == kETAssemblerLineTypeExternSymbol || at->al->type == kETAssemblerLineTypeSymbol;
    label->name = name;
    if(!hashmap_puts(inv->label_hashmap, name, label))
    {
        diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory, failed to insert label into hashmap");
        free(label);
        free(name);
        return false;
    }

    return true;
}
