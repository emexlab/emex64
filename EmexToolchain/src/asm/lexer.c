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
#include <stdint.h>
#include <ctype.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/support/parser.h>
#include <EmexToolchain/support/pack.h>
#include <EmexToolchain/asm/lexer.h>
#include <EmexToolchain/asm/emitter/register.h>
#include <EmexToolchain/asm/emitter/opcode.h>

typedef enum: UInt8 {
    /* the nothing mode */
    kLextokTokenModeNone,

    /*
     * string mode, means it parses the
     * next characters as a character
     * buffer sequence
     */
    kLextokTokenModeString,

    /*
     * character mode, means it parses the next characters as
     * a character, if its not a valid character next steps in
     * compilation will fail, but thats not responsibility
     * of this parser!
     */
    kLextokTokenModeCharacter,

    /*
     * this mode is for <some/path/to/some/system.e64inc>
     * rawrrrrr >:3
     */
    kLextokTokenModeHeaderName,
} kLextokTokenMode;

_Thread_local static const char *stokptr;
_Thread_local static const char *ltokptr;
_Thread_local static char otoken[LEXTOK_LENGHT_MAX + 1];

static inline void __lextok_skip_ignore_spaces(void)
{
    while(1)
    {
        switch(ltokptr[0])
        {
            case ' ':
            case '\t':
                ltokptr++;
                break;
            case ';':
                ltokptr = NULL;
                [[fallthrough]];
            default:
                return;
        }
    }
}

static inline void __lextok_append(unsigned short *otoken_pos)
{
    otoken[(*otoken_pos)++] = *(ltokptr++);
}

static inline void __lextok_handle_punctuation(unsigned short *otoken_pos,
                                               lextok_token_t *token)
{
    if(*otoken_pos > 0)
    {
        return;
    }
    token->type = kAssemblerTokenTypeIdentifier;
    __lextok_append(otoken_pos);
}

static inline char __lextok_get_end_char(kLextokTokenMode mode)
{
    switch(mode)
    {
        case kLextokTokenModeCharacter:
            return '\'';
        case kLextokTokenModeString:
            return '"';
        case kLextokTokenModeHeaderName:
        default:
            return '>';
    }
}

lextok_token_t assembler_lexer_tok(const char *token)
{
    if(token != NULL)
    {
        /* if token is passed then this is the beginning of something we are meant to parse */
        ltokptr = token;
        stokptr = token;
    }

    /* skip the junk in front of us */
    __lextok_skip_ignore_spaces();
    if(ltokptr == NULL || ltokptr[0] == '\0')
    {
        /* if ltokptr is nullified this or nullterminated then we shall not continue, there is nothing to tokenize */
        return (lextok_token_t){ .token = NULL, .column = 0 };
    }

    lextok_token_t retval;
    retval.type = kAssemblerTokenTypeIdentifier;
    retval.column = ltokptr - stokptr;

    /* perform copy */
    UInt16 a = 0;
    kLextokTokenMode token_mode = kLextokTokenModeNone;
    while(a <= LEXTOK_LENGHT_MAX)
    {
        if(a == LEXTOK_LENGHT_MAX)
        {
            retval.type = kAssemblerTokenTypeTooLong;
            break;
        }

        /* processing string */
        switch(token_mode)
        {
            case kLextokTokenModeNone:
                switch(ltokptr[0])
                {
                    /* handling what shall be skipped and not tokenized */
                    case ';':
                    case ' ':
                    case '\t':
                        goto break_out;

                    /* punctuation */
                    case ':':
                    case '(':
                    case ')':
                    case '+':
                    case '-':
                    case '*':
                    case '/':           
                    case ',':
                        __lextok_handle_punctuation(&a, &retval);
                        goto break_out;
                    
                    /* handling string beginnings */
                    case '"':
                        token_mode = kLextokTokenModeString;
                        retval.type = kAssemblerTokenTypeInvalid;
                        break;
                    
                    /* handling character beginnings */
                    case '\'':
                        token_mode = kLextokTokenModeCharacter;
                        retval.type = kAssemblerTokenTypeInvalid;
                        break;

                    /* handling header name beginnings */
                    case '<':
                        token_mode = kLextokTokenModeHeaderName;
                        retval.type = kAssemblerTokenTypeInvalid;
                        break;
                    
                    default:
                        break;
                }
                break;
            case kLextokTokenModeString:
            case kLextokTokenModeCharacter:
            case kLextokTokenModeHeaderName:
                if(ltokptr[0] == __lextok_get_end_char(token_mode))
                {
                    if(a > 0 && otoken[a - 1] == '\\')
                    {
                        /* escaped quote, stay in string mode */
                        break;
                    }

                    __lextok_append(&a);
                    retval.type = kAssemblerTokenTypeIdentifier;
                    goto break_out;
                }
                break;
            default:
                break;
        }

        otoken[a++] = ltokptr[0];

        /* check for nulltermination in ltokptr */
        if(*(++ltokptr) == '\0')
        {
            break;
        }

        continue;
    }

break_out:

    otoken[a] = '\0';

    retval.token = (a == 0) ? NULL : otoken;
    return retval;
}

static Boolean __assembly_lexer_validate_identifier(const char *s)
{
    if(s == NULL || s[0] == '\0')
    {
        return false;
    }

    size_t len = strlen(s);
    size_t end = len;
    if(s[end - 1] == ':')
    {
        end--;
    }

    if(end == 0)
    {
        return false;
    }

    if(!isalpha((unsigned char)s[0]) && s[0] != '_' && s[0] != '.')
    {
        return false;
    }

    if(s[0] == '.' && end == 1)
    {
        return false;
    }

    for(size_t i = 1; i < end; i++)
    {
        if(!isalnum((unsigned char)s[i]) && s[i] != '_' && s[i] != '.')
        {
            return false;
        }
    }

    return true;
}

static kAssemblerKeyword __assembler_lexer_keyword(const char *s)
{
    switch(pack_name(s))
    {
        case PACK('s','e','c','t','i','o','n'): return kAssemblerKeywordSection;
        case PACK('e','x','t','e','r','n'): return kAssemblerKeywordExtern;
        default: return kAssemblerKeywordInvalid;
    }
}

Boolean assembler_lexer_classify(assembler_token_t *at)
{
    /* first we need to find out what exactly they are */
    parser_return_t pret = parse_value_from_string(at->str);
    switch(pret.type)
    {
        case emexParserValueTypeNumber:
            at->integer_literal.v = pret.value;
            at->type = kAssemblerTokenTypeInteger;
            return true;
        case emexParserValueTypeBuffer:
            at->string_literal.buf = calloc(pret.len + 1, sizeof(char));
            if(at->string_literal.buf == NULL)
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(at), "out of memory, couldn't allocate buffer for string literal '%s'", at->str);
                return false;
            }
            memcpy(at->string_literal.buf, (const char*)pret.value, pret.len);
            at->string_literal.buf[pret.len] = '\0';
            at->string_literal.len = pret.len;
            at->type = kAssemblerTokenTypeString;
            return true;
        case emexParserValueTypeOverflow:
            diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(at), "integer literal '%s' overflows 64bit length", at->str);
            return false;
        case emexParserValueTypeString:
            /* remains a identifier under certain conditions */
            size_t len = strlen(at->str);
            if(len == 1)
            {
                /* checking if it is a math op or structural punctation */
                switch(at->str[0])
                {
                    case ',':
                        at->type = kAssemblerTokenTypeComma;
                        return true;
                    case ':':
                        at->type = kAssemblerTokenTypeColon;
                        return true;
                    case '(':
                        at->type = kAssemblerTokenTypeLParen;
                        return true;
                    case ')':
                        at->type = kAssemblerTokenTypeRParen;
                        return true;
                    case '+':
                        at->type = kAssemblerTokenTypePlus;
                        return true;
                    case '-':
                        at->type = kAssemblerTokenTypeMinus;
                        return true;
                    case '*':
                        at->type = kAssemblerTokenTypeMultiply;
                        return true;
                    case '/':
                        at->type = kAssemblerTokenTypeDivide;
                        return true;
                    default:
                        break;
                }
            }

            /* checking if it is a register */
            E64Register reg = register_from_string(at->str);
            if(reg != kE64RegisterInvalid)
            {
                at->register_identifier.v = reg;
                at->type = kAssemblerTokenTypeRegister;
                return true;
            }

            E64RegisterExtended reg_ext = register_extended_from_string(at->str);
            if(reg_ext != kE64RegisterExtendedInvalid)
            {
                at->register_identifier.v_extended = reg_ext;
                at->type = kAssemblerTokenTypeRegisterExtended;
                return true;
            }

            /* checking if it is a opcode */
            E64Opcode op = opcode_from_string(at->str);
            if(op != kE64OpcodeInvalid)
            {
                at->instruction_identifier.v = op;
                at->type = kAssemblerTokenTypeInstruction;
                return true;
            }

            /* checking if it is a keyword */
            kAssemblerKeyword keyword = __assembler_lexer_keyword(at->str);
            if(keyword != kAssemblerKeywordInvalid)
            {
                at->keyword.v = keyword;
                at->type = kAssemblerTokenTypeKeyword;
                return true;
            }

            /* checking if identifier is in a valid format */
            if(!__assembly_lexer_validate_identifier(at->str))
            {
                diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(at), "token '%s' is not a valid identifier", at->str);
                return false;
            }

            at->type = kAssemblerTokenTypeIdentifier;
            return true;
        default:
            diagnostic_report(at->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(at), "unknown token '%s'", at->str);
            return false;
    }
}

const char *assembler_lexer_str_for_token_type(kAssemblerTokenType type)
{
    switch(type)
    {
        case kAssemblerTokenTypeIdentifier:
            return "identifier";
        case kAssemblerTokenTypeInteger:
            return "integer literal";
        case kAssemblerTokenTypeString:
            return "string literal";
        case kAssemblerTokenTypeRegister:
            return "register identifier";
        case kAssemblerTokenTypeInstruction:
            return "instruction identifier";
        case kAssemblerTokenTypeKeyword:
            return "keyword";
        case kAssemblerTokenTypeComma:
        case kAssemblerTokenTypeColon:
        case kAssemblerTokenTypeLParen:
        case kAssemblerTokenTypeRParen:
            return "punctuation";
        case kAssemblerTokenTypePlus:
        case kAssemblerTokenTypeMinus:
        case kAssemblerTokenTypeMultiply:
        case kAssemblerTokenTypeDivide:
            return "binary operation";
        default:
            return "unknown token";
    }
}
