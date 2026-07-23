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

#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/lexer.h>
#include <EmexToolchain/ETAssembler/expr.h>

assembler_token_t *expr_peek(assembler_expr_t *e)
{
    return (e->pos < e->count) ? e->tok[e->pos] : NULL;
}

SInt64 expr_primary(assembler_expr_t *e)
{
    assembler_token_t *t = expr_peek(e);
    if(t == NULL)
    {
        e->error = true;
        e->blame = (e->count > 0) ? e->tok[e->count - 1] : NULL;
        e->why = "expected a value";
        return 0;
    }

    switch(t->type)
    {
        case kETAssemblerTokenTypePlus:
            e->pos++;
            return +expr_primary(e);
        case kETAssemblerTokenTypeMinus:
            e->pos++;
            return -expr_primary(e);
        case kETAssemblerTokenTypeInteger:
            e->pos++;
            return (SInt64)t->integer_literal.v;
        case kETAssemblerTokenTypeLParen:
        {
            e->pos++;
            SInt64 v = expr_addsub(e);
            assembler_token_t *close = expr_peek(e);
            if(close == NULL || close->type != kETAssemblerTokenTypeRParen)
            {
                e->error = true;
                e->blame = (close != NULL) ? close : t;
                e->why = "expected ')'";
                return 0;
            }
            e->pos++;
            return v;
        }
        case kETAssemblerTokenTypeIdentifier:
            e->error = true;
            e->blame = t;
            e->why = "labels can't be used inside constant expressions yet";
            return 0;
        default:
            e->error = true;
            e->blame = t;
            e->why = "unexpected token in constant expression";
            return 0;
    }
}

SInt64 expr_term(assembler_expr_t *e)
{
    SInt64 v = expr_primary(e);
    while(!e->error)
    {
        assembler_token_t *t = expr_peek(e);
        if(t == NULL)
        {
            break;
        }
        if(t->type == kETAssemblerTokenTypeMultiply)
        {
            e->pos++;
            v *= expr_primary(e);
        }
        else if(t->type == kETAssemblerTokenTypeDivide)
        {
            e->pos++;
            SInt64 d = expr_primary(e);
            if(d == 0)
            {
                e->error = true;
                e->blame = t;
                e->why = "division by zero in constant expression";
                return 0;
            }
            v /= d;
        }
        else
        {
            break;
        }
    }
    return v;
}

SInt64 expr_addsub(assembler_expr_t *e)
{
    SInt64 v = expr_term(e);
    while(!e->error)
    {
        assembler_token_t *t = expr_peek(e);
        if(t == NULL)
        {
            break;
        }
        if(t->type == kETAssemblerTokenTypePlus)
        {
            e->pos++;
            v += expr_term(e);
        }
        else if(t->type == kETAssemblerTokenTypeMinus)
        {
            e->pos++;
            v -= expr_term(e);
        }
        else
        {
            break;
        }
    }
    return v;
}

Boolean assembler_eval_const(assembler_token_t **tok,
                             UInt64 count,
                             SInt64 *out)
{
    assembler_expr_t e = {
        .tok = tok, .count = count, .pos = 0,
        .error = false, .blame = NULL, .why = NULL
    };

    SInt64 v = expr_addsub(&e);

    if(e.error)
    {
        assembler_token_t *token = e.blame != NULL ? e.blame : tok[0];
        ETAssemblerDiagnosticConsumerReport(token->al->inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(token), EFSTR("%s"), e.why);
        return false;
    }
    if(e.pos != count)
    {
        ETAssemblerDiagnosticConsumerReport(tok[e.pos]->al->inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(tok[e.pos]), EFSTR("unexpected %s '%s' after constant expression"), assembler_lexer_str_for_token_type(tok[e.pos]->type), tok[e.pos]->str);
        return false;
    }

    *out = v;
    return true;
}
