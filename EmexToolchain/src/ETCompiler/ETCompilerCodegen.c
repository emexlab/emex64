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

#include <EmexToolchain/ETCompiler/ETCompilerCodegen.h>

#define kETCodegenMaxRegister   9
#define kETCodegenMaxSymbols    64
#define kETCodegenMaxScopeDepth 16
#define kETCodegenMaxFunctions  64

#pragma mark - call support

typedef struct {
    EFStringRef name;
    EFIndex paramCount;
    Boolean hasBody;
} ETCodegenFunction;

typedef struct {
    ETCodegenFunction functions[kETCodegenMaxFunctions];
    EFIndex count;
} ETCodegenFunctionTable;

static const ETCodegenFunction *FunctionTableLookup(const ETCodegenFunctionTable *table, EFStringRef name)
{
    for(EFIndex index = 0; index < table->count; index++)
    {
        if(EFStringEqual(table->functions[index].name, name))
        {
            return &table->functions[index];
        }
    }
    return NULL;
}

#pragma mark - symbol table

typedef struct {
    EFStringRef name;
    Boolean isGlobal;
    EFIndex slot;
} ETCodegenSymbol;

typedef struct {
    ETCodegenSymbol symbols[kETCodegenMaxSymbols];
    EFIndex count;
    EFIndex scopeBase[kETCodegenMaxScopeDepth];
    EFIndex scopeDepth;
    EFIndex nextSlot;
    ETCodegenFunctionTable functions;
} ETCodegenScope;

static void ScopePush(ETCodegenScope *scope)
{
    if(scope->scopeDepth >= kETCodegenMaxScopeDepth)
    {
        return;
    }
    scope->scopeBase[scope->scopeDepth++] = scope->count;
}

static void ScopePop(ETCodegenScope *scope)
{
    if(scope->scopeDepth == 0)
    {
        return;
    }
    scope->count = scope->scopeBase[--scope->scopeDepth];
}

static const ETCodegenSymbol *ScopeLookup(ETCodegenScope *scope,
                                          EFStringRef name)
{
    for(EFIndex index = scope->count; index > 0; index--)
    {
        if(EFStringEqual(scope->symbols[index - 1].name, name))
        {
            return &scope->symbols[index - 1];
        }
    }
    return NULL;
}

static Boolean ScopeDeclare(ETCodegenScope *scope,
                            EFStringRef name,
                            Boolean isGlobal)
{
    if(scope->count >= kETCodegenMaxSymbols)
    {
        return false;
    }

    EFIndex base = (scope->scopeDepth > 0) ? scope->scopeBase[scope->scopeDepth - 1] : 0;
    for(EFIndex index = base; index < scope->count; index++)
    {
        if(EFStringEqual(scope->symbols[index].name, name))
        {
            return false;
        }
    }

    scope->symbols[scope->count].name = name;
    scope->symbols[scope->count].isGlobal = isGlobal;
    scope->symbols[scope->count].slot = isGlobal ? 0 : scope->nextSlot++;
    scope->count++;
    return true;
}

#pragma mark - helpers

static EFStringRef NodeTokenString(ETCompilerASTNodeRef node)
{
    return ETCompilerTokenGetString(ETCompilerASTNodeGetToken(node));
}

static EFStringRef NodeTokenNumber(ETCompilerASTNodeRef node)
{
    return ETCompilerTokenGetNumber(ETCompilerASTNodeGetToken(node));
}

static const char *BinaryMnemonic(ETCompilerTokenType type)
{
    switch(type)
    {
        case kETCompilerTokenTypeAddition: return "add";
        case kETCompilerTokenTypeSubtraction: return "sub";
        case kETCompilerTokenTypeMultiplication: return "mul";
        case kETCompilerTokenTypeDivision: return "div";
        default: return NULL;
    }
}

static Boolean IsCommutative(ETCompilerTokenType type)
{
    return type == kETCompilerTokenTypeAddition || type == kETCompilerTokenTypeMultiplication;
}

static EFStringRef TryImmediate(ETCompilerASTNodeRef node)
{
    if(node == NULL) return NULL;
    if(ETCompilerASTNodeGetKind(node) != kETCompilerASTNodeKindIntLiteral)
    {
        return NULL;
    }
    return NodeTokenNumber(node);
}

static void EmitLoadSymbol(EFMutableStringRef asmSource,
                           const ETCodegenSymbol *symbol,
                           EFIndex reg)
{
    if(symbol->isGlobal)
    {
        EFStringAppendFormat(asmSource, EFSTR("  ldq r%ld, %@\n"), (long)reg, symbol->name);
    }
    else
    {
        EFStringAppendFormat(asmSource, EFSTR("  ldq r%ld, [fp - %ld]\n"), (long)reg, (long)(symbol->slot * 8));
    }
}

static void EmitStoreSymbol(EFMutableStringRef asmSource,
                            const ETCodegenSymbol *symbol,
                            EFIndex reg)
{
    if(symbol->isGlobal)
    {
        EFStringAppendFormat(asmSource, EFSTR("  stq %@, r%ld\n"), symbol->name, (long)reg);
    }
    else
    {
        EFStringAppendFormat(asmSource, EFSTR("  stq [fp - %ld], r%ld\n"), (long)(symbol->slot * 8), (long)reg);
    }
}

#pragma mark - expressions

static Boolean EmitExpression(EFMutableStringRef asmSource,
                              ETCodegenScope *scope,
                              ETCompilerASTNodeRef node,
                              EFIndex reg)
{
    if(node == NULL || reg > kETCodegenMaxRegister)
    {
        return false;
    }

    switch(ETCompilerASTNodeGetKind(node))
    {
        case kETCompilerASTNodeKindIntLiteral:
            EFStringAppendFormat(asmSource, EFSTR("  mov r%ld, %@\n"), (long)reg, NodeTokenNumber(node));
            return true;

        case kETCompilerASTNodeKindVarRef:
        {
            const ETCodegenSymbol *symbol = ScopeLookup(scope, NodeTokenString(node));
            if(symbol == NULL)
            {
                return false;
            }
            EmitLoadSymbol(asmSource, symbol, reg);
            return true;
        }

        case kETCompilerASTNodeKindBinaryOp:
        {
            EFAUTOREL EFArrayRef operands = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, node);
            if(EFArrayGetCount(operands) != 2)
            {
                return false;
            }

            ETCompilerASTNodeRef lhs = EFArrayGetValueAtIndex(operands, 0);
            ETCompilerASTNodeRef rhs = EFArrayGetValueAtIndex(operands, 1);
            ETCompilerTokenType op = ETCompilerTokenGetType(ETCompilerASTNodeGetToken(node));

            if(op == kETCompilerTokenTypeAssign)
            {
                if(ETCompilerASTNodeGetKind(lhs) != kETCompilerASTNodeKindVarRef)
                {
                    return false;
                }

                const ETCodegenSymbol *symbol = ScopeLookup(scope, NodeTokenString(lhs));
                if(symbol == NULL)
                {
                    return false;
                }

                if(!EmitExpression(asmSource, scope, rhs, reg))
                {
                    return false;
                }
                EmitStoreSymbol(asmSource, symbol, reg);
                return true;
            }

            const char *mnemonic = BinaryMnemonic(op);
            if(mnemonic == NULL)
            {
                return false;
            }

            EFStringRef lhsImm = TryImmediate(lhs);
            EFStringRef rhsImm = TryImmediate(rhs);

            if(lhsImm != NULL && rhsImm != NULL)
            {
                EFStringAppendFormat(asmSource, EFSTR("  %s r%ld, %@, %@\n"), mnemonic, (long)reg, lhsImm, rhsImm);
                return true;
            }

            if(rhsImm != NULL)
            {
                if(!EmitExpression(asmSource, scope, lhs, reg))
                {
                    return false;
                }
                EFStringAppendFormat(asmSource, EFSTR("  %s r%ld, %@\n"), mnemonic, (long)reg, rhsImm);
                return true;
            }

            if(lhsImm != NULL && IsCommutative(op))
            {
                if(!EmitExpression(asmSource, scope, rhs, reg))
                {
                    return false;
                }
                EFStringAppendFormat(asmSource, EFSTR("  %s r%ld, %@\n"), mnemonic, (long)reg, lhsImm);
                return true;
            }

            if(lhsImm != NULL)
            {
                if(!EmitExpression(asmSource, scope, rhs, reg))
                {
                    return false;
                }
                EFStringAppendFormat(asmSource, EFSTR("  %s r%ld, %@, r%ld\n"), mnemonic, (long)reg, lhsImm, (long)reg);
                return true;
            }

            /* both need registers */
            if(!EmitExpression(asmSource, scope, lhs, reg))
            {
                return false;
            }
            if(!EmitExpression(asmSource, scope, rhs, reg + 1))
            {
                return false;
            }
            EFStringAppendFormat(asmSource, EFSTR("  %s r%ld, r%ld\n"), mnemonic, (long)reg, (long)(reg + 1));
            return true;
        }

        case kETCompilerASTNodeKindCall:
        {
            EFStringRef callee = NodeTokenString(node);

            const ETCodegenFunction *function = FunctionTableLookup(&scope->functions, callee);
            if(function == NULL)
            {
                return false;
            }

            EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, node);
            if(EFArrayGetCount(children) < 1)
            {
                return false;
            }

            EFAUTOREL EFArrayRef arguments = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, EFArrayGetValueAtIndex(children, 0));
            EFIndex argumentCount = EFArrayGetCount(arguments);

            if(argumentCount != function->paramCount)
            {
                return false;
            }
            if(argumentCount > 9)
            {
                return false;
            }

            for(EFIndex i = argumentCount; i > 0; i--)
            {
                ETCompilerASTNodeRef argument = EFArrayGetValueAtIndex(arguments, i - 1);
                if(TryImmediate(argument) != NULL) continue;
                if(!EmitExpression(asmSource, scope, argument, reg + (i - 1)))
                {
                    return false;
                }
            }

            EFAUTOREL EFMutableStringRef call = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR("  blw "));
            if(call == NULL)
            {
                return false;
            }

            EFStringAppendFormat(call, EFSTR("%@"), callee);

            for(EFIndex i = 0; i < argumentCount; i++)
            {
                ETCompilerASTNodeRef argument = EFArrayGetValueAtIndex(arguments, i);
                EFStringRef immediate = TryImmediate(argument);

                if(immediate != NULL)
                {
                    EFStringAppendFormat(call, EFSTR(", %@"), immediate);
                }
                else
                {
                    EFStringAppendFormat(call, EFSTR(", r%ld"), (long)(reg + i));
                }
            }

            EFStringAppendString(call, EFSTR("\n"));
            EFStringAppendString(asmSource, call);

            EFStringAppendFormat(asmSource, EFSTR("  mov r%ld, rr\n"), (long)reg);
            return true;
        }

        default:
            return false;
    }
}

#pragma mark - statements

static void CountLocals(ETCompilerASTNodeRef block, EFIndex *count)
{
    EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, block);
    for(EFIndex index = 0; index < EFArrayGetCount(children); index++)
    {
        ETCompilerASTNodeRef child = EFArrayGetValueAtIndex(children, index);
        switch(ETCompilerASTNodeGetKind(child))
        {
            case kETCompilerASTNodeKindVarDecl:
                (*count)++;
                break;
            case kETCompilerASTNodeKindBlock:
                CountLocals(child, count);
                break;
            default:
                break;
        }
    }
}

static void EmitBlock(EFMutableStringRef asmSource, ETCodegenScope *scope, ETCompilerASTNodeRef block);

static void EmitStatement(EFMutableStringRef asmSource,
                          ETCodegenScope *scope,
                          ETCompilerASTNodeRef statement)
{
    switch(ETCompilerASTNodeGetKind(statement))
    {
        case kETCompilerASTNodeKindVarDecl:
        {
            EFStringRef name = NodeTokenString(statement);

            if(!ScopeDeclare(scope, name, false))
            {
                break;
            }

            const ETCodegenSymbol *symbol = ScopeLookup(scope, name);
            if(symbol == NULL)
            {
                break;
            }

            EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, statement);
            if(EFArrayGetCount(children) > 1)
            {
                ETCompilerASTNodeRef initializer = EFArrayGetValueAtIndex(children, 1);
                if(EmitExpression(asmSource, scope, initializer, 0))
                {
                    EmitStoreSymbol(asmSource, symbol, 0);
                }
            }
            break;
        }
        case kETCompilerASTNodeKindExprStmt:
        {
            EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, statement);
            if(EFArrayGetCount(children) > 0)
            {
                EmitExpression(asmSource, scope, EFArrayGetValueAtIndex(children, 0), 0);
            }
            break;
        }
        case kETCompilerASTNodeKindReturn:
        {
            EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, statement);
            if(EFArrayGetCount(children) > 0)
            {
                if(EmitExpression(asmSource, scope, EFArrayGetValueAtIndex(children, 0), 0))
                {
                    EFStringAppendString(asmSource, EFSTR("  mov rr, r0\n"));
                }
            }
            else
            {
                EFStringAppendString(asmSource, EFSTR("  clr rr\n"));
            }
            EFStringAppendString(asmSource, EFSTR("  wret\n"));
            break;
        }
        case kETCompilerASTNodeKindBlock:
            EmitBlock(asmSource, scope, statement);
            break;
        default:
            /* not supported yet */
            break;
    }
}

static void EmitBlock(EFMutableStringRef asmSource,
                      ETCodegenScope *scope,
                      ETCompilerASTNodeRef block)
{
    ScopePush(scope);

    EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, block);
    for(EFIndex index = 0; index < EFArrayGetCount(children); index++)
    {
        EmitStatement(asmSource, scope, EFArrayGetValueAtIndex(children, index));
    }

    ScopePop(scope);
}

#pragma mark - call support 2 =3

static void CollectFunctions(ETCompilerASTNodeRef unit, ETCodegenScope *scope)
{
    EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, unit);
    for(EFIndex index = 0; index < EFArrayGetCount(children); index++)
    {
        ETCompilerASTNodeRef child = EFArrayGetValueAtIndex(children, index);
        ETCompilerASTNodeKind kind = ETCompilerASTNodeGetKind(child);

        if(kind != kETCompilerASTNodeKindFunctionDef && kind != kETCompilerASTNodeKindFunctionDecl) continue;

        EFStringRef name = NodeTokenString(child);
        Boolean hasBody = (kind == kETCompilerASTNodeKindFunctionDef);

        EFIndex paramCount = 0;
        {
            EFAUTOREL EFArrayRef parts = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, child);
            if(EFArrayGetCount(parts) > 1)
            {
                EFAUTOREL EFArrayRef params = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, EFArrayGetValueAtIndex(parts, 1));
                paramCount = EFArrayGetCount(params);
            }
        }

        ETCodegenFunction *existing = (ETCodegenFunction *)FunctionTableLookup(&scope->functions, name);
        if(existing != NULL)
        {
            if(hasBody) existing->hasBody = true;
            continue;
        }

        if(scope->functions.count >= kETCodegenMaxFunctions)
        {
            return;
        }

        scope->functions.functions[scope->functions.count].name = name;
        scope->functions.functions[scope->functions.count].paramCount = paramCount;
        scope->functions.functions[scope->functions.count].hasBody = hasBody;
        scope->functions.count++;
    }
}

static void EmitParameters(EFMutableStringRef asmSource,
                           ETCodegenScope *scope,
                           ETCompilerASTNodeRef parameterList)
{
    EFAUTOREL EFArrayRef params = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, parameterList);

    for(EFIndex index = 0; index < EFArrayGetCount(params); index++)
    {
        ETCompilerASTNodeRef param = EFArrayGetValueAtIndex(params, index);
        ETCompilerTokenRef nameToken = ETCompilerASTNodeGetToken(param);
        if(nameToken == NULL)
        {
            continue;
        }

        EFStringRef name = ETCompilerTokenGetString(nameToken);
        if(!ScopeDeclare(scope, name, false))
        {
            continue;
        }

        const ETCodegenSymbol *symbol = ScopeLookup(scope, name);
        if(symbol == NULL)
        {
            continue;
        }

        EmitStoreSymbol(asmSource, symbol, index);
    }
}

#pragma mark - entry point

EFStringRef ETCompilerCodegenCreateASMSourceWithAST(ETCompilerASTNodeRef node)
{
    if(node == NULL)
    {
        return NULL;
    }

    EFAUTOREL EFMutableStringRef asmSource = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR("; code generated by the emex64 C compiler\n\n"));
    if(asmSource == NULL)
    {
        return NULL;
    }

    ETCodegenScope scope = {0};
    ScopePush(&scope);

    EFAUTOREL EFArrayRef children = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, node);

    /* file scope variables (emission as sections) */
    {
        EFAUTOREL EFMutableStringRef dataSectionSource = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR("section .data\n"));
        EFAUTOREL EFMutableStringRef bssSectionSource = EFStringCreateMutableCopy(kEFAllocatorDefault, EFSTR("section .bss\n"));
        if(dataSectionSource == NULL || bssSectionSource == NULL)
        {
            return NULL;
        }

        Boolean emitDataSection = false;
        Boolean emitBSSSection = false;

        for(EFIndex index = 0; index < EFArrayGetCount(children); index++)
        {
            ETCompilerASTNodeRef child = EFArrayGetValueAtIndex(children, index);
            if(ETCompilerASTNodeGetKind(child) != kETCompilerASTNodeKindVarDecl)
            {
                continue;
            }

            EFStringRef name = NodeTokenString(child);
            ScopeDeclare(&scope, name, true);

            EFAUTOREL EFArrayRef declaration = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, child);

            if(EFArrayGetCount(declaration) > 1)
            {
                ETCompilerASTNodeRef initializer = EFArrayGetValueAtIndex(declaration, 1);

                if(ETCompilerASTNodeGetKind(initializer) != kETCompilerASTNodeKindIntLiteral)
                {
                    /* huh =< */
                    continue;
                }

                EFStringAppendFormat(dataSectionSource, EFSTR("  %@ dq %@\n"), name, NodeTokenNumber(initializer));
                emitDataSection = true;
            }
            else
            {
                EFStringAppendFormat(bssSectionSource, EFSTR("  %@ dq 1\n"), name);
                emitBSSSection = true;
            }
        }

        if(emitBSSSection)
        {
            EFStringAppendFormat(asmSource, EFSTR("%@\n"), bssSectionSource);
        }
        if(emitDataSection)
        {
            EFStringAppendFormat(asmSource, EFSTR("%@\n"), dataSectionSource);
        }
    }

    CollectFunctions(node, &scope);

    /* functions */
    for(EFIndex index = 0; index < EFArrayGetCount(children); index++)
    {
        ETCompilerASTNodeRef child = EFArrayGetValueAtIndex(children, index);
        if(ETCompilerASTNodeGetKind(child) != kETCompilerASTNodeKindFunctionDef)
        {
            continue;
        }

        EFAUTOREL EFArrayRef functionBody = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, child);
        if(EFArrayGetCount(functionBody) < 3)
        {
            continue;
        }

        ETCompilerASTNodeRef parameterListRef = EFArrayGetValueAtIndex(functionBody, 1);
        ETCompilerASTNodeRef blockRef = EFArrayGetValueAtIndex(functionBody, 2);

        EFIndex localCount = 0;
        CountLocals(blockRef, &localCount);
        {
            EFAUTOREL EFArrayRef params = ETCompilerASTNodeCopyChildren(kEFAllocatorDefault, parameterListRef);
            localCount += EFArrayGetCount(params);
        }

        EFStringAppendFormat(asmSource, EFSTR("%@:\n"), NodeTokenString(child));

        if(localCount > 0)
        {
            EFStringAppendFormat(asmSource, EFSTR("  sub sp, %ld\n"), (long)(localCount * 8));
        }

        scope.nextSlot = 0;
        ScopePush(&scope);
        EmitParameters(asmSource, &scope, parameterListRef);
        EmitBlock(asmSource, &scope, blockRef);
        ScopePop(&scope);
        EFStringAppendString(asmSource, EFSTR("\n"));
    }

    ScopePop(&scope);
    return EFAUTOTRANSFER(asmSource);
}
