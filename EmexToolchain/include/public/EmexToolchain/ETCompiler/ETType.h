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

#ifndef ETCOMPILER_ETTYPE_H
#define ETCOMPILER_ETTYPE_H

typedef enum {
    TY_VOID, TY_BOOL, TY_CHAR, TY_SHORT, TY_INT, TY_LONG,
    TY_FLOAT, TY_DOUBLE,
    TY_PTR, TY_ARRAY, TY_FUNC, TY_STRUCT, TY_UNION, TY_ENUM,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    int size;
    int align;
    bool is_unsigned;

    Type *base;
    int array_len;

    Member *members;
    bool is_flexible;

    Type *return_ty;
    Type *params;
    bool is_variadic;
    bool has_prototype;

    Type *next;
} Type;

typedef enum {
    ND_NUM, ND_VAR, ND_MEMBER,
    ND_ADD, ND_SUB, ND_MUL, ND_DIV, ND_MOD,
    ND_EQ, ND_NE, ND_LT, ND_LE,
    ND_AND, ND_OR, ND_NOT, ND_BITAND, ND_BITOR, ND_BITXOR,
    ND_SHL, ND_SHR, ND_NEG,
    ND_ASSIGN, ND_COMMA, ND_COND, ND_CAST,
    ND_ADDR, ND_DEREF,
    ND_FUNCALL,
    ND_RETURN, ND_IF, ND_FOR, ND_WHILE, ND_BLOCK, ND_EXPR_STMT,
    ND_SWITCH, ND_CASE, ND_GOTO, ND_LABEL, ND_BREAK, ND_CONTINUE,
} NodeKind;

typedef struct Node {
    NodeKind kind;
    Type *ty;
    Token *tok;

    Node *lhs, *rhs;
    Node *cond, *then, *els, *init, *inc;
    Node *body;
    Node *next;

    Obj *var;
    int64_t val;
    Member *member;
    Node *args;
    char *label;
} Node;

typedef struct Obj {
    char *name;
    Type *ty;
    bool is_local;
    int offset;
    bool is_function, is_definition, is_static;
    Obj *params, *locals;
    Node *body;
    Obj *next;
} Obj;

#endif /* ETCOMPILER_ETTYPE_H */
