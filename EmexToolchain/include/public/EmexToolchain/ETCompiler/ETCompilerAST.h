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

#ifndef ETCOMPILERAST_H
#define ETCOMPILERAST_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETCompiler/ETCompilerToken.h>

typedef enum: UInt8 {
    kETCompilerASTNodeKindTranslationUnit,
    kETCompilerASTNodeKindFunctionDef,
    kETCompilerASTNodeKindFunctionDecl,
    kETCompilerASTNodeKindParamDecl,
    kETCompilerASTNodeKindVarDecl,
    kETCompilerASTNodeKindBlock,
    kETCompilerASTNodeKindReturn,
    kETCompilerASTNodeKindExprStmt,
    kETCompilerASTNodeKindBinaryOp,
    kETCompilerASTNodeKindIntLiteral,
    kETCompilerASTNodeKindVarRef,
} ETCompilerASTNodeKind;

typedef struct __ETCompilerASTNode *ETCompilerASTNodeRef;

EFTypeID ETCompilerASTNodeGetTypeID(void);

ETCompilerASTNodeRef ETCompilerASTNodeCreate(EFAllocatorRef allocator, ETCompilerASTNodeKind kind, ETCompilerTokenRef token);

ETCompilerASTNodeKind ETCompilerASTNodeGetKind(ETCompilerASTNodeRef node);
ETCompilerTokenRef ETCompilerASTNodeGetToken(ETCompilerASTNodeRef node);
EFArrayRef ETCompilerASTNodeCopyChildren(EFAllocatorRef allocator, ETCompilerASTNodeRef node);

Boolean ETCompilerASTNodeAppendChild(ETCompilerASTNodeRef node, ETCompilerASTNodeRef child);

#endif /* ETCOMPILERAST_H */
