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

#include <pthread.h>
#include <EmexToolchain/ETCompiler/ETCompilerToken.h>

typedef struct __ETCompilerToken {
    EFObject super;
    EFStringRef tokenString;    /* token it self */
    EFNumberRef tokenNumber;    /* token it self aswell */
    EFRange range;              /* range inside of the source */
    ETCompilerTokenType type;
} *__ETCompilerToken;

void __ETCompilerTokenDeinit(EFObjectRef compilerTokenRef)
{
    ETCompilerTokenRef compilerToken = (ETCompilerTokenRef)compilerTokenRef;
    EFReleaseTry(compilerToken->tokenString);
    EFReleaseTry(compilerToken->tokenNumber);
}

EFStringRef __ETCompilerTokenTypeStringForType(ETCompilerTokenType type)
{
    switch(type)
    {
        case kETCompilerTokenTypeIdentifier:
            return EFSTR("IDENTIFIER");
        case kETCompilerTokenTypeKeyword:
            return EFSTR("KEYWORD");
        case kETCompilerTokenTypeBaseType:
            return EFSTR("TYPE");
        case kETCompilerTokenTypeNumber:
            return EFSTR("NUMBER");
        case kETCompilerTokenTypeSemicolon:
            return EFSTR("SEMICOLON");
        case kETCompilerTokenTypeRBrace:
            return EFSTR("RBRACE");
        case kETCompilerTokenTypeLBrace:
            return EFSTR("LBRACE");
        case kETCompilerTokenTypeRParen:
            return EFSTR("RPAREN");
        case kETCompilerTokenTypeLParen:
            return EFSTR("LPAREN");
        case kETCompilerTokenTypeRPack:
            return EFSTR("RPACK");
        case kETCompilerTokenTypeLPack:
            return EFSTR("LPACK");
        case kETCompilerTokenTypeAddition:
            return EFSTR("ADDITION");
        case kETCompilerTokenTypeSubtraction:
            return EFSTR("SUBTRACTION");
        case kETCompilerTokenTypeMultiplication:
            return EFSTR("MULTIPLY");
        case kETCompilerTokenTypeDivision:
            return EFSTR("DIVISION");
        case kETCompilerTokenTypeAssign:
            return EFSTR("ASSIGN");
        default:
            return EFSTR("UNKNOWN");
    }
}

EFStringRef __ETCompilerTokenCopyDescription(EFObjectRef compilerTokenRef)
{
    ETCompilerTokenRef compilerToken = (ETCompilerTokenRef)compilerTokenRef;
    EFStringRef typeString = __ETCompilerTokenTypeStringForType(compilerToken->type);
    if(compilerToken->type != kETCompilerTokenTypeNumber)
    {
        return EFStringCreateWithFormat(EFGetAllocator(compilerTokenRef), EFSTR("%@(\"%@\")"), typeString, compilerToken->tokenString);
    }
    else
    {
        return EFStringCreateWithFormat(EFGetAllocator(compilerTokenRef), EFSTR("%@(%@)"), typeString, compilerToken->tokenNumber);
    }
}

EFStringRef __ETCompilerTokenCopyDebugDescription(EFObjectRef compilerTokenRef)
{
    ETCompilerTokenRef compilerToken = (ETCompilerTokenRef)compilerTokenRef;
    EFStringRef typeString = __ETCompilerTokenTypeStringForType(compilerToken->type);
    return EFStringCreateWithFormat(EFGetAllocator(compilerTokenRef), EFSTR("<ETCompilerToken %p>{tokenString = \"%@\", tokenNumber = %@, range = {location = %d, length = %d}, type = %@}"), compilerToken, compilerToken->tokenString, compilerToken->tokenNumber, compilerToken->range.location, compilerToken->range.length, typeString);
}

EFClassDefinitionV4 ETCompilerTokenClass = {
    .header = {
        .version = 4,
        .typeID = kEFTypeIDNone,
        .name = EFSTR_FILESCOPE("ETCompilerToken"),
    },
    .init = NULL,
    .deinit = __ETCompilerTokenDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = __ETCompilerTokenCopyDescription,
    .copyDebugDescription = __ETCompilerTokenCopyDebugDescription,
};

static void ETCompilerTokenRegisterClass(void)
{
    EFClassRegister(&ETCompilerTokenClass);
}

EFTypeID ETCompilerTokenGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETCompilerTokenRegisterClass);
    return ETCompilerTokenClass.header.typeID;
}

ETCompilerTokenRef ETCompilerTokenCreate(EFAllocatorRef allocator,
                                         EFStringRef tokenString,
                                         EFRange range,
                                         ETCompilerTokenType type)
{
    if(tokenString == NULL)
    {
        return NULL;
    }

    ETCompilerTokenRef compilerToken = (ETCompilerTokenRef)EFObjectCreate(allocator, ETCompilerTokenGetTypeID(), (EFIndex)sizeof(struct __ETCompilerToken));
    if(compilerToken == NULL)
    {
        return NULL;
    }

    compilerToken->tokenString = EFRetain(tokenString);
    if(compilerToken->tokenString == NULL)
    {
        EFRelease(compilerToken);
        return NULL;
    }

    if(type == kETCompilerTokenTypeNumber)
    {
        compilerToken->tokenNumber = EFStringCopyNumber(allocator, tokenString);
        if(compilerToken->tokenNumber == NULL)
        {
            EFRelease(compilerToken);
            return NULL;
        }
    }

    compilerToken->range = range;
    compilerToken->type = type;

    return compilerToken;
}

EFStringRef ETCompilerTokenGetString(ETCompilerTokenRef token)
{
    if(token == NULL)
    {
        return NULL;
    }

    return token->tokenString;
}

EFNumberRef ETCompilerTokenGetNumber(ETCompilerTokenRef token)
{
    if(token == NULL)
    {
        return NULL;
    }

    return token->tokenNumber;
}

EFRange ETCompilerTokenGetRange(ETCompilerTokenRef token)
{
    if(token == NULL)
    {
        return EFRangeZero;
    }

    return token->range;
}

ETCompilerTokenType ETCompilerTokenGetType(ETCompilerTokenRef token)
{
    if(token == NULL)
    {
        return kETCompilerTokenTypeIdentifier;
    }

    return token->type;
}

