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

#ifndef ETCOMPILERTOKEN_H
#define ETCOMPILERTOKEN_H

#include <EmexFoundation/EmexFoundation.h>

typedef enum: UInt8 {
    /* statements */
    kETCompilerTokenTypeIdentifier,
    kETCompilerTokenTypeKeyword,
    kETCompilerTokenTypeBaseType,
    kETCompilerTokenTypeNumber,

    /* binary operations */
    kETCompilerTokenTypeAddition,
    kETCompilerTokenTypeSubtraction,
    kETCompilerTokenTypeMultiplication,
    kETCompilerTokenTypeDivision,
    kETCompilerTokenTypeAssign,

    /* punctuation */
    kETCompilerTokenTypeComma,
    kETCompilerTokenTypeSemicolon,
    kETCompilerTokenTypeRBrace,
    kETCompilerTokenTypeLBrace,
    kETCompilerTokenTypeRParen,
    kETCompilerTokenTypeLParen,
    kETCompilerTokenTypeRPack,
    kETCompilerTokenTypeLPack,
} ETCompilerTokenType;

typedef struct __ETCompilerToken *ETCompilerTokenRef;

ETCompilerTokenRef ETCompilerTokenCreate(EFAllocatorRef allocator, EFStringRef tokenString, EFRange range, ETCompilerTokenType type);

EFStringRef ETCompilerTokenGetString(ETCompilerTokenRef token);
EFNumberRef ETCompilerTokenGetNumber(ETCompilerTokenRef token);
EFRange ETCompilerTokenGetRange(ETCompilerTokenRef token);
ETCompilerTokenType ETCompilerTokenGetType(ETCompilerTokenRef token);

#endif /* ETCOMPILERTOKEN_H */
