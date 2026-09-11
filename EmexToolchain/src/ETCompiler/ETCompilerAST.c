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

#include <EmexToolchain/ETCompiler/ETCompilerAST.h>
#include <pthread.h>

typedef struct __ETCompilerASTNode {
    EFObject super;
    ETCompilerASTNodeKind kind;
    ETCompilerTokenRef token;       /* the token the node came from */
    EFMutableArrayRef children;
} *__ETCompilerASTNode;

void __ETCompilerASTNodeDeinit(EFObjectRef ref)
{
    ETCompilerASTNodeRef node = (ETCompilerASTNodeRef)ref;
    EFReleaseTry(node->token);
    EFReleaseTry(node->children);
}

EFClassDefinitionV4 ETCompilerASTNodeClass = {
    .header = {
        .version = 4,
        .typeID = kEFTypeIDNone,
        .name = EFSTR_FILESCOPE("ETCompilerASTNode"),
    },
    .init = NULL,
    .deinit = __ETCompilerASTNodeDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

static void ETCompilerASTNodeRegisterClass(void)
{
    EFClassRegister(&ETCompilerASTNodeClass);
}

EFTypeID ETCompilerASTNodeGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETCompilerASTNodeRegisterClass);
    return ETCompilerASTNodeClass.header.typeID;
}

ETCompilerASTNodeRef ETCompilerASTNodeCreate(EFAllocatorRef allocator,
                                             ETCompilerASTNodeKind kind,
                                             ETCompilerTokenRef token)
{
    ETCompilerTokenRef ownedToken = NULL;
    if(token != NULL)
    {
        ownedToken = EFRetain(token);
        if(ownedToken == NULL)
        {
            return NULL;
        }
    }

    ETCompilerASTNodeRef node = (ETCompilerASTNodeRef)EFObjectCreate(allocator, ETCompilerASTNodeGetTypeID(), (EFIndex)sizeof(struct __ETCompilerASTNode));
    if(node == NULL)
    {
        EFReleaseTry(ownedToken);
        return NULL;
    }

    node->kind = kind;
    node->token = ownedToken;
    node->children = EFArrayCreateMutable(allocator, kEFArrayCallbacksObjectCallbacks, 0);
    if(node->children == NULL)
    {
        EFRelease(node);
        return NULL;
    }
    return node;
}

ETCompilerASTNodeKind ETCompilerASTNodeGetKind(ETCompilerASTNodeRef node)
{
    if(node == NULL)
    {
        return kETCompilerASTNodeKindTranslationUnit;
    }
    
    return node->kind;
}

ETCompilerTokenRef ETCompilerASTNodeGetToken(ETCompilerASTNodeRef node)
{
    if(node == NULL)
    {
        return NULL;
    }

    return node->token;
}

EFArrayRef ETCompilerASTNodeCopyChildren(EFAllocatorRef allocator,
                                         ETCompilerASTNodeRef node)
{
    if(node == NULL)
    {
        return NULL;
    }

    return EFArrayCreateCopy(allocator, node->children);
}

Boolean ETCompilerASTNodeAppendChild(ETCompilerASTNodeRef node,
                                     ETCompilerASTNodeRef child)
{
    if(node == NULL)
    {
        return false;
    }

    return EFArrayAppendValue(node->children, child);
}
