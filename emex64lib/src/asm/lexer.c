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
#include <ctype.h>
#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/support/parser.h>
#include <emex64lib/asm/lexer.h>
#include <emex64lib/asm/emitter/register.h>

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
            case ',':
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
    unsigned short a = 0;
    unsigned char token_mode = kCmptokTokenModeNone;
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
            case kCmptokTokenModeNone:
                switch(ltokptr[0])
                {
                    /* handling what shall be skipped and not tokenized */
                    case ';':
                    case ' ':
                    case ',':
                    case '\t':
                        goto break_out;
                    
                    /* handling string beginnings */
                    case '"':
                        token_mode = kCmptokTokenModeString;
                        retval.type = kAssemblerTokenTypeInvalid;
                        break;
                    
                    /* handling character beginnings */
                    case '\'':
                        token_mode = kCmptokTokenModeCharacter;
                        retval.type = kAssemblerTokenTypeInvalid;
                        break;
                    
                    default:
                        break;
                }
                break;
            case kCmptokTokenModeString:
                switch(ltokptr[0])
                {
                    /* handling string ends */
                    case '"':
                        if(a > 0 && otoken[a-1] == '\\')
                        {
                            /* escaped quote, stay in string mode */
                            break;
                        }

                        __lextok_append(&a);
                        retval.type = kAssemblerTokenTypeIdentifier;
                        goto break_out;
                    default:
                        break;
                }
                break;
            case kCmptokTokenModeCharacter:
                switch(ltokptr[0])
                {
                    /* handling character ends */
                    case '\'':
                        if(a > 0 && otoken[a-1] == '\\')
                        {
                            /* escaped meter, stay in string mode */
                            break;
                        }
                        
                        __lextok_append(&a);
                        retval.type = kAssemblerTokenTypeIdentifier;
                        goto break_out;
                    default:
                        break;
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

    break_out:
        break;
    }

    otoken[a] = '\0';

    retval.token = (a == 0) ? NULL : otoken;
    return retval;
}

static bool __assembly_lexer_validate_identifier(const char *s)
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

bool assembler_lexer_classify(assembler_token_t *at)
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
                diag_fatal(at, "out of memory, couldn't allocate buffer for string literal '%s'\n", at->str);
                return false;
            }
            memcpy(at->string_literal.buf, (const char*)pret.value, pret.len);
            at->string_literal.buf[pret.len] = '\0';
            at->string_literal.len = pret.len;
            at->type = kAssemblerTokenTypeString;
            return true;
        case emexParserValueTypeOverflow:
            diag_error(at, "integer literal '%s' overflows 64bit length\n", at->str);
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
            kEmex64Register reg = register_from_string(at->str);
            if(reg != kEmex64RegisterInvalid)
            {
                at->register_literal.v = reg;
                at->type = kAssemblerTokenTypeRegister;
                return true;
            }

            /* checking if identifier is in a valid format */
            if(!__assembly_lexer_validate_identifier(at->str))
            {
                diag_error(at, "token '%s' is not a valid identifier\n", at->str);
                return false;
            }

            at->type = kAssemblerTokenTypeIdentifier;
            return true;
        default:
            diag_error(at, "unknown token '%s'\n", at->str);
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
            return "register literal";
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
