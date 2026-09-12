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

#include <EmexToolchain/ETCompiler/ETCompilerParser.h>

typedef struct __ETCompilerParser {
    EFAllocatorRef allocator;
    EFArrayRef tokens;
    ETCompilerDiagnosticConsumerRef diagnosticConsumer;
    EFIndex index;
    EFIndex count;
} *__ETCompilerParser;

#pragma mark - the heart of the C AST generator

static ETCompilerTokenRef Peek(__ETCompilerParser parser)
{
    EFIndex count = EFArrayGetCount(parser->tokens);
    return EFArrayGetValueAtIndex(parser->tokens, (parser->index < count) ? parser->index : count - 1);
}

static Boolean Check(__ETCompilerParser parser,
                     ETCompilerTokenType type)
{
    return ETCompilerTokenGetType(Peek(parser)) == type;
}

static ETCompilerTokenRef Advance(__ETCompilerParser parser)
{
    ETCompilerTokenRef token = Peek(parser);
    if(!Check(parser, kETCompilerTokenTypeEOF))
    {
        parser->index++;
    }
    return token;
}

static Boolean Match(__ETCompilerParser parser,
                     ETCompilerTokenType type)
{
    if(!Check(parser, type))
    {
        return false;
    }
    Advance(parser);
    return true;
}

static ETCompilerTokenRef Expect(__ETCompilerParser parser,
                                 ETCompilerTokenType type,
                                 const char *what)
{
    if(Check(parser, type))
    {
        return Advance(parser);
    }

    ETCompilerDiagnosticConsumerReport(parser->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("expected %s (%@)"), what ? what : "<nil>", Peek(parser));
    return NULL;
}

#pragma mark - a huge mess

static ETCompilerTokenRef PeekAhead(__ETCompilerParser parser,
                                    EFIndex n)
{
    EFIndex i = parser->index + n;
    return EFArrayGetValueAtIndex(parser->tokens, (i < parser->count) ? i : parser->count - 1);
}

static ETCompilerTokenRef PeekPrevious(__ETCompilerParser parser)
{
    if(parser->index == 0)
    {
        return Peek(parser);
    }
    return EFArrayGetValueAtIndex(parser->tokens, parser->index - 1);
}

#pragma mark - types

static ETCompilerASTNodeRef ParseTypeRef(__ETCompilerParser parser)
{
    ETCompilerTokenRef token = Expect(parser, kETCompilerTokenTypeBaseType, "type name");
    if(token == NULL)
    {
        return NULL;
    }
    return ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindTypeRef, token);
}

static ETCompilerASTNodeRef ParseParamList(__ETCompilerParser parser)
{
    ETCompilerASTNodeRef list = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindParamList, Peek(parser));
    if(list == NULL)
    {
        return NULL;
    }

    if(Check(parser, kETCompilerTokenTypeBaseType) &&
       ETCompilerTokenIsVoid(Peek(parser)) &&
       ETCompilerTokenGetType(PeekAhead(parser, 1)) == kETCompilerTokenTypeRParen)
    {
        Advance(parser);
        return list;
    }

    if(Check(parser, kETCompilerTokenTypeRParen))
    {
        return list;
    }

    do {
        ETCompilerASTNodeRef type = ParseTypeRef(parser);
        if(type == NULL)
        {
            EFRelease(list);
            return NULL;
        }

        ETCompilerTokenRef name = NULL;
        if(Check(parser, kETCompilerTokenTypeIdentifier))
        {
            name = Advance(parser);
        }

        ETCompilerASTNodeRef param = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindParamDecl, name);
        if(param == NULL)
        {
            EFRelease(type);
            EFRelease(list);
            return NULL;
        }

        ETCompilerASTNodeAppendChild(param, type);
        EFRelease(type);

        ETCompilerASTNodeAppendChild(list, param);
        EFRelease(param);
    } while(Match(parser, kETCompilerTokenTypeComma));

    return list;
}

#pragma mark - expressions

static int BinaryPrecedence(ETCompilerTokenType type)
{
    switch(type)
    {
        case kETCompilerTokenTypeAssign:
            return 1;
        case kETCompilerTokenTypeAddition:
        case kETCompilerTokenTypeSubtraction:
            return 2;
        case kETCompilerTokenTypeMultiplication:
        case kETCompilerTokenTypeDivision:
            return 3;
        default:
            return 0;
    }
}

static Boolean IsRightAssociative(ETCompilerTokenType type)
{
    return type == kETCompilerTokenTypeAssign;
}

static ETCompilerASTNodeRef ParseExpression(__ETCompilerParser parser, int minPrecedence);

static ETCompilerASTNodeRef ParsePrimary(__ETCompilerParser parser)
{
    if(Check(parser, kETCompilerTokenTypeNumber))
    {
        return ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindIntLiteral, Advance(parser));
    }

    if(Check(parser, kETCompilerTokenTypeIdentifier))
    {
        return ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindVarRef, Advance(parser));
    }

    if(Match(parser, kETCompilerTokenTypeLParen))
    {
        ETCompilerASTNodeRef inner = ParseExpression(parser, 0);
        if(inner == NULL)
        {
            return NULL;
        }
        if(Expect(parser, kETCompilerTokenTypeRParen, "')'") == NULL)
        {
            EFRelease(inner);
            return NULL;
        }
        return inner;
    }

    ETCompilerDiagnosticConsumerReport(parser->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("expected expression (%@)"), Peek(parser));
    return NULL;
}

static ETCompilerASTNodeRef ParsePostfix(__ETCompilerParser parser);

static ETCompilerASTNodeRef ParseExpression(__ETCompilerParser parser,
                                            int minPrecedence)
{
    ETCompilerASTNodeRef lhs = ParsePostfix(parser);
    if(lhs == NULL)
    {
        return NULL;
    }

    for(;;)
    {
        ETCompilerTokenType type = ETCompilerTokenGetType(Peek(parser));
        int precedence = BinaryPrecedence(type);
        if(precedence == 0 || precedence < minPrecedence)
        {
            break;
        }

        ETCompilerTokenRef op = Advance(parser);
        int nextMin = IsRightAssociative(type) ? precedence : precedence + 1;

        ETCompilerASTNodeRef rhs = ParseExpression(parser, nextMin);
        if(rhs == NULL)
        {
            EFRelease(lhs);
            return NULL;
        }

        ETCompilerASTNodeRef binary = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindBinaryOp, op);
        if(binary == NULL)
        {
            EFRelease(lhs);
            EFRelease(rhs);
            return NULL;
        }

        ETCompilerASTNodeAppendChild(binary, lhs);
        ETCompilerASTNodeAppendChild(binary, rhs);
        EFRelease(lhs);
        EFRelease(rhs);

        lhs = binary;
    }
    return lhs;
}

#pragma mark - statements

static ETCompilerASTNodeRef ParseExternalDeclaration(__ETCompilerParser parser);
static ETCompilerASTNodeRef ParseBlock(__ETCompilerParser parser);

static ETCompilerASTNodeRef ParseStatement(__ETCompilerParser parser)
{
    if(Check(parser, kETCompilerTokenTypeBaseType))
    {
        return ParseExternalDeclaration(parser);
    }

    if(Check(parser, kETCompilerTokenTypeKeyword) && ETCompilerTokenIsReturn(Peek(parser)))
    {
        ETCompilerTokenRef keyword = Advance(parser);
        ETCompilerASTNodeRef node = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindReturn, keyword);
        if(node == NULL)
        {
            return NULL;
        }

        if(!Check(parser, kETCompilerTokenTypeSemicolon))
        {
            ETCompilerASTNodeRef expression = ParseExpression(parser, 0);
            if(expression == NULL)
            {
                EFRelease(node);
                return NULL;
            }
            ETCompilerASTNodeAppendChild(node, expression);
            EFRelease(expression);
        }
        Expect(parser, kETCompilerTokenTypeSemicolon, "';'");
        return node;
    }

    if(Check(parser, kETCompilerTokenTypeLBrace))
    {
        return ParseBlock(parser);
    }

    ETCompilerTokenRef first = Peek(parser);
    ETCompilerASTNodeRef expression = ParseExpression(parser, 0);
    if(expression == NULL)
    {
        return NULL;
    }
    Expect(parser, kETCompilerTokenTypeSemicolon, "';'");

    ETCompilerASTNodeRef statement = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindExprStmt, first);
    if(statement == NULL)
    {
        EFRelease(expression);
        return NULL;
    }

    ETCompilerASTNodeAppendChild(statement, expression);
    EFRelease(expression);
    return statement;
}

static void SynchronizeInBlock(__ETCompilerParser parser)
{
    if(!Check(parser, kETCompilerTokenTypeEOF))
    {
        Advance(parser);
    }

    while(!Check(parser, kETCompilerTokenTypeEOF))
    {
        if(ETCompilerTokenGetType(PeekPrevious(parser)) == kETCompilerTokenTypeSemicolon)
        {
            return;
        }

        if(Check(parser, kETCompilerTokenTypeRBrace))
        {
            return;
        }

        if(Check(parser, kETCompilerTokenTypeBaseType) ||
           Check(parser, kETCompilerTokenTypeLBrace)   ||
           (Check(parser, kETCompilerTokenTypeKeyword) && ETCompilerTokenIsReturn(Peek(parser))))
        {
            return;
        }

        Advance(parser);
    }
}

static ETCompilerASTNodeRef ParseBlock(__ETCompilerParser parser)
{
    ETCompilerTokenRef brace = Expect(parser, kETCompilerTokenTypeLBrace, "'{'");
    if(brace == NULL)
    {
        return NULL;
    }

    ETCompilerASTNodeRef block = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindBlock, brace);
    if(block == NULL)
    {
        return NULL;
    }

    while(!Check(parser, kETCompilerTokenTypeRBrace) && !Check(parser, kETCompilerTokenTypeEOF))
    {
        ETCompilerASTNodeRef statement = ParseStatement(parser);
        if(statement == NULL)
        {
            SynchronizeInBlock(parser);
            continue;
        }
        ETCompilerASTNodeAppendChild(block, statement);
        EFRelease(statement);
    }

    Expect(parser, kETCompilerTokenTypeRBrace, "'}'");
    return block;
}

#pragma mark - declarations

static ETCompilerASTNodeRef ParseExternalDeclaration(__ETCompilerParser parser)
{
    ETCompilerASTNodeRef type = ParseTypeRef(parser);
    if(type == NULL)
    {
        return NULL;
    }

    ETCompilerTokenRef name = Expect(parser, kETCompilerTokenTypeIdentifier, "declarator name");
    if(name == NULL)
    {
        EFRelease(type);
        return NULL;
    }

    ETCompilerASTNodeRef node = NULL;

    if(Match(parser, kETCompilerTokenTypeLParen))
    {
        ETCompilerASTNodeRef params = ParseParamList(parser);
        if(params == NULL)
        {
            EFRelease(type);
            return NULL;
        }

        if(Expect(parser, kETCompilerTokenTypeRParen, "')'") == NULL)
        {
            EFRelease(type);
            EFRelease(params);
            return NULL;
        }

        if(Check(parser, kETCompilerTokenTypeLBrace))
        {
            ETCompilerASTNodeRef body = ParseBlock(parser);
            if(body == NULL)
            {
                EFRelease(type);
                EFRelease(params);
                return NULL;
            }

            node = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindFunctionDef, name);
            if(node == NULL)
            {
                EFRelease(type);
                EFRelease(params);
                EFRelease(body);
                return NULL;
            }

            ETCompilerASTNodeAppendChild(node, type);
            ETCompilerASTNodeAppendChild(node, params);
            ETCompilerASTNodeAppendChild(node, body);
            EFRelease(body);
        }
        else
        {
            if(Expect(parser, kETCompilerTokenTypeSemicolon, "';'") == NULL)
            {
                EFRelease(type);
                EFRelease(params);
                return NULL;
            }

            node = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindFunctionDecl, name);
            if(node == NULL)
            {
                EFRelease(type);
                EFRelease(params);
                return NULL;
            }

            ETCompilerASTNodeAppendChild(node, type);
            ETCompilerASTNodeAppendChild(node, params);
        }
        EFRelease(params);
    }
    else
    {
        node = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindVarDecl, name);
        if(node == NULL)
        {
            EFRelease(type);
            return NULL;
        }

        ETCompilerASTNodeAppendChild(node, type);

        if(Match(parser, kETCompilerTokenTypeAssign))
        {
            ETCompilerASTNodeRef initializer = ParseExpression(parser, 0);
            if(initializer == NULL)
            {
                EFRelease(type);
                EFRelease(node);
                return NULL;
            }
            ETCompilerASTNodeAppendChild(node, initializer);
            EFRelease(initializer);
        }
        Expect(parser, kETCompilerTokenTypeSemicolon, "';'");
    }

    EFRelease(type);
    return node;
}

static void SynchronizeToNextDeclaration(__ETCompilerParser parser)
{
    if(!Check(parser, kETCompilerTokenTypeEOF))
    {
        Advance(parser);
    }

    while(!Check(parser, kETCompilerTokenTypeEOF))
    {
        ETCompilerTokenType previous = ETCompilerTokenGetType(PeekPrevious(parser));
        if(previous == kETCompilerTokenTypeSemicolon)
        {
            return;
        }
        if(previous == kETCompilerTokenTypeRBrace)
        {
            return;
        }
        if(Check(parser, kETCompilerTokenTypeBaseType))
        {
            return;
        }
        Advance(parser);
    }
}

#pragma mark - function calling

static ETCompilerASTNodeRef ParseArgList(__ETCompilerParser parser)
{
    ETCompilerASTNodeRef list = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindArgList, Peek(parser));
    if(list == NULL)
    {
        return NULL;
    }

    if(Check(parser, kETCompilerTokenTypeRParen))
    {
        return list;
    }

    do
    {
        ETCompilerASTNodeRef argument = ParseExpression(parser, 0);
        if(argument == NULL)
        {
            EFRelease(list);
            return NULL;
        }

        ETCompilerASTNodeAppendChild(list, argument);
        EFRelease(argument);
    } while(Match(parser, kETCompilerTokenTypeComma));

    return list;
}

static ETCompilerASTNodeRef ParsePostfix(__ETCompilerParser parser)
{
    ETCompilerASTNodeRef expression = ParsePrimary(parser);
    if(expression == NULL)
    {
        return NULL;
    }

    while(Match(parser, kETCompilerTokenTypeLParen))
    {
        if(ETCompilerASTNodeGetKind(expression) != kETCompilerASTNodeKindVarRef)
        {
            ETCompilerDiagnosticConsumerReport(parser->diagnosticConsumer, kDiagnosticSeverityError, NULL, EFSTR("called object is not a function"));
            EFRelease(expression);
            return NULL;
        }

        ETCompilerASTNodeRef arguments = ParseArgList(parser);
        if(arguments == NULL)
        {
            EFRelease(expression);
            return NULL;
        }

        if(Expect(parser, kETCompilerTokenTypeRParen, "')'") == NULL)
        {
            EFRelease(expression);
            EFRelease(arguments);
            return NULL;
        }

        ETCompilerASTNodeRef call = ETCompilerASTNodeCreate(parser->allocator, kETCompilerASTNodeKindCall, ETCompilerASTNodeGetToken(expression));
        if(call == NULL)
        {
            EFRelease(expression);
            EFRelease(arguments);
            return NULL;
        }

        ETCompilerASTNodeAppendChild(call, arguments);
        EFRelease(arguments);
        EFRelease(expression);

        expression = call;
    }

    return expression;
}

#pragma mark - entry point

ETCompilerASTNodeRef ETCompilerParserCopyTranslationUnitForTokenArray(EFAllocatorRef allocator,
                                                                      EFArrayRef tokens,
                                                                      ETCompilerDiagnosticConsumerRef diagnosticConsumer)
{
    if(tokens == NULL || diagnosticConsumer == NULL)
    {
        return NULL;
    }

    EFIndex count = EFArrayGetCount(tokens);
    if(count == 0)
    {
        return NULL;
    }

    struct __ETCompilerParser state = {
        .allocator = allocator,
        .tokens = tokens,
        .diagnosticConsumer = diagnosticConsumer,
        .index = 0,
        .count = count,
    };
    __ETCompilerParser parser = &state;

    ETCompilerASTNodeRef unit = ETCompilerASTNodeCreate(allocator, kETCompilerASTNodeKindTranslationUnit, NULL);
    if(unit == NULL)
    {
        return NULL;
    }

    while(!Check(parser, kETCompilerTokenTypeEOF))
    {
        ETCompilerASTNodeRef declaration = ParseExternalDeclaration(parser);
        if(declaration == NULL)
        {
            SynchronizeToNextDeclaration(parser);
            continue;
        }
        ETCompilerASTNodeAppendChild(unit, declaration);
        EFRelease(declaration);
    }

    return unit;
}
