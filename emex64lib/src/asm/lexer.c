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

#include <emex64lib/asm/lexer.h>

_Thread_local static const char *stokptr;
_Thread_local static const char *ltokptr;
_Thread_local static char otoken[CMPTOK_LENGHT_MAX];

static inline void cmptok_skip_triggers(void)
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

static inline void cmptok_append(unsigned short *otoken_pos)
{
    otoken[(*otoken_pos)++] = *(ltokptr++);
}

cmptok_token_t cmptok(const char *token)
{
    if(token != NULL)
    {
        /* if token is passed then this is the beginning of something we are meant to parse */
        ltokptr = token;
        stokptr = token;
    }

    /* skip the junk in front of us */
    cmptok_skip_triggers();
    if(ltokptr == NULL || ltokptr[0] == '\0')
    {
        /* if ltokptr is nullified this or nullterminated then we shall not continue, there is nothing to tokenize */
        return (cmptok_token_t){ .token = NULL, .column = 0 };
    }

    cmptok_token_t retval;
    retval.type = kAssemblerTokenTypeStandard;
    retval.column = ltokptr - stokptr;

    /* perform copy */
    unsigned short a = 0;
    unsigned char token_mode = kCmptokTokenModeNone;
    while(a < CMPTOK_LENGHT_MAX - 1)
    {
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
                    
                    /* handling static expression beginnings */
                    case '(':
                        token_mode = kCmptokTokenModeExpression;
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

                        cmptok_append(&a);
                        retval.type = kAssemblerTokenTypeStandard;
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
                        
                        cmptok_append(&a);
                        retval.type = kAssemblerTokenTypeStandard;
                        goto break_out;
                    default:
                        break;
                }
                break;
            case kCmptokTokenModeExpression:
                switch(ltokptr[0])
                {
                    /* handling character ends */
                    case ')':                        
                        cmptok_append(&a);
                        retval.type = kAssemblerTokenTypeStaticExpression;
                        goto break_out;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        goto skip_break_out;

break_out:
        break;
skip_break_out:

        otoken[a++] = ltokptr[0];

        /* check for nulltermination in ltokptr */
        if(*(++ltokptr) == '\0')
        {
            break;
        }
    }

    otoken[a] = '\0';

    retval.token = (a == 0) ? NULL : otoken;
    return retval;
}
