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

#include <EmexToolchain/ETCompiler/ETCompilerLexer.h>
#include <EmexToolchain/Support/pack.h>

static Boolean __ETCompilerLexerAppendToken(EFMutableArrayRef tokens,
                                            EFStringRef token,
                                            EFRange range,
                                            ETCompilerTokenType type)
{
    if(type != kETCompilerTokenTypeIdentifier)
    {
        goto skip_to_creation;
    }

    switch(pack_name(EFStringGetCStringPtr(token, kEFStringEncodingUTF8)))
    {
        case PACK('v','o','i','d'):
        case PACK('u','6','4'):
            type = kETCompilerTokenTypeBaseType;
            goto skip_to_creation;
        case PACK('i','f'):
        case PACK('e','l','s','e'):
        case PACK('s','w','i','t','c','h'):
        case PACK('r','e','t','u','r','n'):
        /*case PACK('t','y','p','e','d','e','f'):
        case PACK('s','t','r','u','c','t'):*/
            type = kETCompilerTokenTypeKeyword;
            goto skip_to_creation;
        default:
            break;
    }

    if(EFStringIsNumber(token))
    {
        type = kETCompilerTokenTypeNumber;
    }

skip_to_creation:
    {
        EFAUTOREL ETCompilerTokenRef compilerToken = ETCompilerTokenCreate(kEFAllocatorDefault, token, range, type);
        if(!EFArrayAppendValue(tokens, compilerToken))
        {
            EFLog(EFSTR("[!] failed to append token\n"));
            return false;
        }
    }
    return true;
}

EFMutableArrayRef ETCompilerLexerCreateTokenArrayWithFile(EFFileRef inputFile,
                                                          ETCompilerDiagnosticConsumerRef diagnosticConsumer)
{
    if(inputFile == NULL)
    {
        ETCompilerDiagnosticConsumerReport(diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("no input file provided to lexer"));
        return NULL;
    }
    EFAllocatorRef allocator = EFGetAllocator(inputFile);

    /* need that string */
    EFAUTOREL EFDataRef inputFileData = EFFileCopyData(allocator, inputFile);
    EFAUTOREL EFStringRef inputFileString = EFStringCreateFromExternalRepresentation(allocator, inputFileData, kEFStringEncodingUTF8);
    const char *cString = EFStringGetCStringPtr(inputFileString, kEFStringEncodingUTF8);
    if(cString == NULL)
    {
        ETCompilerDiagnosticConsumerReport(diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("file data lexer received was malformed"));
        return NULL;
    }

    /* okay now we can parse the shit out of that file */
    EFAUTOREL EFMutableArrayRef tokens = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, 0);
    if(tokens == NULL)
    {
        ETCompilerDiagnosticConsumerReport(diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, couldn't allocate array for tokens"));
        return NULL;
    }

    EFIndex cStringLength = EFStringGetLength(inputFileString);

    /* lexer state */
    EFIndex loopLocation = 0;
    EFRange range = EFRangeZero;
    ETCompilerTokenType mtype = kETCompilerTokenTypeIdentifier; /* for multi switches */

    /* lexer loop */
    for(; loopLocation < cStringLength; loopLocation++)
    {
        const char *cStringPtr = (const char*)((EFAddr)cString + (EFAddr)loopLocation);
        switch(cStringPtr[0])
        {
            /* comment bay >~< */
            case '/':
            {
                if((loopLocation + 1) <= cStringLength)
                {
                    switch(cStringPtr[1])
                    {
                        case '*':   /* block comment */
                        {
                            for(; (loopLocation + 1) < cStringLength; loopLocation++)
                            {
                                cStringPtr = (const char*)((EFAddr)cString + (EFAddr)loopLocation);
                                if(cStringPtr[0] == '*' && cStringPtr[1] == '/')
                                {
                                    loopLocation++;
                                    goto block_comment_end;
                                }
                            }
                        block_comment_end:
                            range.length = 0;
                            range.location = loopLocation + 2;
                            goto continue_lexer_loop;
                        }
                        case '/':   /* normal comment */
                        {
                            for(; loopLocation < cStringLength; loopLocation++)
                            {
                                cStringPtr = (const char*)((EFAddr)cString + (EFAddr)loopLocation);
                                if(cStringPtr[0] == '\n')
                                {
                                    goto comment_end;
                                }
                            }
                        comment_end:
                            range.length = 0;
                            range.location = loopLocation + 1;
                            goto continue_lexer_loop;
                        }
                    }
                }

                /* math division operation */
                mtype = kETCompilerTokenTypeDivision;
                goto handle_punctuation;
            }

            /* binary operation */
            case '+':
                mtype = kETCompilerTokenTypeAddition;
                goto handle_punctuation;
            case '-':
                mtype = kETCompilerTokenTypeSubtraction;
                goto handle_punctuation;
            case '*':
                mtype = kETCompilerTokenTypeMultiplication;
                goto handle_punctuation;
            case '=':
                mtype = kETCompilerTokenTypeAssign;
                goto handle_punctuation;

            /* punctuation bay */
            case ',':
                mtype = kETCompilerTokenTypeComma;
                goto handle_punctuation;
            case ';':
                mtype = kETCompilerTokenTypeSemicolon;
                goto handle_punctuation;
            case '{':
                mtype = kETCompilerTokenTypeLBrace;
                goto handle_punctuation;
            case '}':
                mtype = kETCompilerTokenTypeRBrace;
                goto handle_punctuation;
            case '(':
                mtype = kETCompilerTokenTypeLParen;
                goto handle_punctuation;
            case ')':
                mtype = kETCompilerTokenTypeRParen;
                goto handle_punctuation;
            case '[':
                mtype = kETCompilerTokenTypeLPack;
                goto handle_punctuation;
            case ']':
                mtype = kETCompilerTokenTypeRPack;
        handle_punctuation:
            {
                if(range.length > 0)
                {
                    EFAUTOREL EFStringRef token = EFStringCreateCopyWithRange(kEFAllocatorDefault, inputFileString, range);
                    if(!__ETCompilerLexerAppendToken(tokens, token, range, kETCompilerTokenTypeIdentifier))
                    {
                        ETCompilerDiagnosticConsumerReport(diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("lexer failed to append token"));
                        return NULL;
                    }
                }

                range.location = loopLocation;
                range.length = 1;

                EFAUTOREL EFStringRef token = EFStringCreateCopyWithRange(kEFAllocatorDefault, inputFileString, range);
                if(!__ETCompilerLexerAppendToken(tokens, token, range, mtype))
                {
                    ETCompilerDiagnosticConsumerReport(diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("lexer failed to append token"));
                    return NULL;
                }

                range.length = 0;
                range.location = loopLocation + 1;
                goto continue_lexer_loop;
            }

            /* generic tokenization bay */
            case ' ':
            case '\t':
            case '\n':
            {
                if(range.length > 0)
                {
                    EFAUTOREL EFStringRef token = EFStringCreateCopyWithRange(kEFAllocatorDefault, inputFileString, range);
                    if(!__ETCompilerLexerAppendToken(tokens, token, range, kETCompilerTokenTypeIdentifier))
                    {
                        ETCompilerDiagnosticConsumerReport(diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("lexer failed to append token"));
                        return NULL;
                    }
                }
                range.length = 0;
                range.location = loopLocation + 1;
                goto continue_lexer_loop;
            }
            default:
                range.length++;
                break;
        }
    continue_lexer_loop:
        continue;
    }

    return EFAUTOTRANSFER(tokens);
}
