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
#include <stdarg.h>

typedef struct __ETCompilerASTNode {
    EFObject super;
    ETCompilerASTNodeKind kind;
    ETCompilerTokenRef token;       /* the token the node came from */
    EFMutableArrayRef children;
} *__ETCompilerASTNode;

static void __ETCompilerASTNodeDeinit(EFObjectRef ref)
{
    ETCompilerASTNodeRef node = (ETCompilerASTNodeRef)ref;
    EFReleaseTry(node->token);
    EFReleaseTry(node->children);
}

static EFStringRef __ETCompilerASTNodeKindToString(ETCompilerASTNodeKind kind)
{
    switch(kind)
    {
        case kETCompilerASTNodeKindTranslationUnit:
            return EFSTR("TranslationUnit");
        case kETCompilerASTNodeKindFunctionDef:
            return EFSTR("FunctionDefinition");
        case kETCompilerASTNodeKindFunctionDecl:
            return EFSTR("FunctionDeclaration");
        case kETCompilerASTNodeKindParamDecl:
            return EFSTR("ParameterDeclaration");
        case kETCompilerASTNodeKindVarDecl:
            return EFSTR("VariableDeclaration");
        case kETCompilerASTNodeKindBlock:
            return EFSTR("Block");
        case kETCompilerASTNodeKindReturn:
            return EFSTR("Return");
        case kETCompilerASTNodeKindExprStmt:
            return EFSTR("ExpressionStatement");
        case kETCompilerASTNodeKindBinaryOp:
            return EFSTR("BinaryOperation");
        case kETCompilerASTNodeKindIntLiteral:
            return EFSTR("IntegerLiteral");
        case kETCompilerASTNodeKindVarRef:
            return EFSTR("VariableReference");
        case kETCompilerASTNodeKindTypeRef:
            return EFSTR("TypeReference");
        case kETCompilerASTNodeKindParamList:
            return EFSTR("ParameterList");
        case kETCompilerASTNodeKindCall:
            return EFSTR("Call");
        case kETCompilerASTNodeKindArgList:
            return EFSTR("ArgumentList");
        default:
            return EFSTR("Unknown");
    }
}

static void __ETCompilerASTNodeLevelFormatAppend(EFMutableStringRef description,
                                                 EFIndex level,
                                                 EFStringRef format,
                                                 ...)
{
    for(EFIndex index = 0; index < level; index++)
    {
        EFStringAppendString(description, EFSTR("  "));
    }
    
    va_list arguments;
    va_start(arguments, format);
    EFAUTOREL EFStringRef resultRef = EFStringCreateWithFormatAndArguments(NULL, format, arguments);
    va_end(arguments);

    EFStringAppendString(description, resultRef);
}

static void __ETCompilerASTNodeDescriptionAppend(EFMutableStringRef description,
                                                 ETCompilerASTNodeRef node,
                                                 EFIndex level)
{
    EFStringRef kindString = __ETCompilerASTNodeKindToString(node->kind);
    __ETCompilerASTNodeLevelFormatAppend(description, level, EFSTR("%@\n"), kindString);
    level++;

    if(node->token != NULL)
    {
        __ETCompilerASTNodeLevelFormatAppend(description, level, EFSTR("token: %@\n"), node->token);
    }

    if(EFArrayGetCount(node->children) > 0)
    {
        __ETCompilerASTNodeLevelFormatAppend(description, level, EFSTR("children:\n"));

        level++;
        for(EFIndex index = 0; index < EFArrayGetCount(node->children); index++)
        {
            ETCompilerASTNodeRef child = EFArrayGetValueAtIndex(node->children, index);
            __ETCompilerASTNodeDescriptionAppend(description, child, level);
        }
        level--;
    }

    level--;
}

static EFStringRef __ETCompilerASTNodeCopyDescription(EFObjectRef ref)
{
    ETCompilerASTNodeRef node = (ETCompilerASTNodeRef)ref;

    EFAUTOREL EFMutableStringRef description = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR(""));
    if(description == NULL)
    {
        return NULL;
    }

    __ETCompilerASTNodeDescriptionAppend(description, node, 0);

    if(EFStringHasSuffix(description, EFSTR("\n")))
    {
        EFStringDelete(description, EFRangeMake(EFStringGetLength(description) - 1, 1));
    }

    return EFAUTOTRANSFER(description);
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
    .copyDescription = __ETCompilerASTNodeCopyDescription,
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
