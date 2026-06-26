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

#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/asm/lexer.h>
#include <emex64lib/asm/expr.h>

assembler_token_t *expr_peek(assembler_expr_t *e)
{
    return (e->pos < e->count) ? e->tok[e->pos] : NULL;
}

int64_t expr_primary(assembler_expr_t *e)
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
        case kAssemblerTokenTypePlus:
            e->pos++;
            return +expr_primary(e);
        case kAssemblerTokenTypeMinus:
            e->pos++;
            return -expr_primary(e);
        case kAssemblerTokenTypeInteger:
            e->pos++;
            return (int64_t)t->integer_literal.v;
        case kAssemblerTokenTypeLParen:
        {
            e->pos++;
            int64_t v = expr_addsub(e);
            assembler_token_t *close = expr_peek(e);
            if(close == NULL || close->type != kAssemblerTokenTypeRParen)
            {
                e->error = true;
                e->blame = (close != NULL) ? close : t;
                e->why = "expected ')'";
                return 0;
            }
            e->pos++;
            return v;
        }
        case kAssemblerTokenTypeIdentifier:
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

int64_t expr_term(assembler_expr_t *e)
{
    int64_t v = expr_primary(e);
    while(!e->error)
    {
        assembler_token_t *t = expr_peek(e);
        if(t == NULL)
        {
            break;
        }
        if(t->type == kAssemblerTokenTypeMultiply)
        {
            e->pos++;
            v *= expr_primary(e);
        }
        else if(t->type == kAssemblerTokenTypeDivide)
        {
            e->pos++;
            int64_t d = expr_primary(e);
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

int64_t expr_addsub(assembler_expr_t *e)
{
    int64_t v = expr_term(e);
    while(!e->error)
    {
        assembler_token_t *t = expr_peek(e);
        if(t == NULL)
        {
            break;
        }
        if(t->type == kAssemblerTokenTypePlus)
        {
            e->pos++;
            v += expr_term(e);
        }
        else if(t->type == kAssemblerTokenTypeMinus)
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

bool assembler_eval_const(assembler_token_t **tok, uint64_t count, int64_t *out)
{
    assembler_expr_t e = {
        .tok = tok, .count = count, .pos = 0,
        .error = false, .blame = NULL, .why = NULL
    };
    
    int64_t v = expr_addsub(&e);

    if(e.error)
    {
        assembler_token_t *token = e.blame != NULL ? e.blame : tok[0];
        diagnostic_report(token->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(token), "%s", e.why);
        return false;
    }
    if(e.pos != count)
    {
        diagnostic_report(tok[e.pos]->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(tok[e.pos]), "unexpected %s '%s' after constant expression", assembler_lexer_str_for_token_type(tok[e.pos]->type), tok[e.pos]->str);
        return false;
    }

    *out = v;
    return true;
}
