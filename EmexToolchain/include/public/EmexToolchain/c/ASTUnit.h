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

#ifndef EMEX64C_ASTUNIT_H
#define EMEX64C_ASTUNIT_H

#include <stddef.h>
#include <EmexFoundation/EmexFoundation.h>

enum ASTNodeKind: UInt8 {
    /* structures */
    ASTNodeKindTranslationUnit,
    ASTNodeKindFunctionDeclaration,
    ASTNodeKindFunctionDefinition,
    ASTNodeKindStructDeclaration,

    /* statements */
    ASTNodeKindCompoundStatement,
    ASTNodeKindExpressionStatement,
    ASTNodeKindReturnStatement,
    ASTNodeKindIfStatement,
    ASTNodeKindWhileStatement,
    ASTNodeKindForStatement,
    ASTNodeKindBreakStatement,
    ASTNodeKindContinueStatement,

    /* declarations */
    ASTNodeKindVariableDeclaration,
    ASTNodeKindParameterDeclaration,

    /* leaf expressions */
    ASTNodeKindBinaryExpression,
    ASTNodeKindUnaryExpression,
    ASTNodeKindAssignExpression,
    ASTNodeKindFunctionCall,
    ASTNodeKindMemberAccess,
    ASTNodeKindArrayIndex,
    ASTNodeKindCastExpression,
};

/* characters that are in between expressions */
enum OpKind: UInt8 {
    OpKindAdd,
    OpKindSub,
    OpKindMul,
    OpKindDiv,
    OpKindEqual,
    OpKindNotEqual,
    OpKindLessThan,
    OpKindGreaterThan
};

/* characters that are right before literals */
enum UnaryOpKind: UInt8 {
    UnaryOpKindMinus,
    UnaryOpKindNot,
    UnaryOpKindBitwiseNot,
    UnaryOpKindDereference,
    UnaryOpKindReference
};

/* structure access */
enum AccessKind: UInt8 {
    AccessKindDot,
    AccessKindArrow
} AccessKind;

enum DataType: UInt8 {
    DataTypeUnsignedChar,       /* 8 bit unsigned */
    DataTypeUnsignedShort,      /* 16 bit unsigned */
    DataTypeUnsignedInteger,    /* 32 bit unsigned */
    DataTypeUnsignedLong,       /* 64 bit unsigned */
    DataTypeSignedChar,         /* 8 bit signed */
    DataTypeSignedShort,        /* 16 bit signed */
    DataTypeSignedInteger,      /* 32 bit signed (that is the standard type of int) */
    DataTypeSignedLong          /* 64 bit signed */
};

struct ASTNode {
    enum ASTNodeKind kind;

    union {
        /* structures */

        /*
         * the entire c file that needs to be translated
         * down to ASM.
         */
        struct {
            struct ASTNode* declarations;
        } translationUnit;

        /*
         * function declarations are...
         * like..
         * 
         * int foo();
         * 
         */
        struct {
            char* name;
            enum DataType type;
            struct ASTNode* parameters; 
        } functionDeclaration;

        /*
         * function definitions are the function it self,
         * like..
         * 
         * int foo()
         * {
         *     return 0;
         * }
         * 
         */
        struct {
            char* name;
            enum DataType type;
            struct ASTNode* parameters;
            struct ASTNode* body;
        } functionDefinition;

        /* bruh, its a declaration of a structure */
        struct {
            char* name;
            struct ASTNode* members;       /* hopefully all variable declaration nodes lol */
        } structDeclaration;

        /* statements */

        struct {
            struct ASTNode* body;
        } compoundStatement;

        struct ASTNodeExpressionStatement {
            struct ASTNode* expression;
        } expressionStatement;

        struct ASTNodeExpressionStatement returnStatement;

        struct {
            struct ASTNode* condition;      /* hopefully an expression */
            struct ASTNode* then_branch;    /* what to execute if condition(yk.. the expression) is met (usually an compound statement) */
            struct ASTNode* else_branch;    /* what to execute if condition is not met (null if not existing) (usually an compound statement) */
        } ifStatement;

        struct {
            struct ASTNode* condition;      /* hopefully an expression */
            struct ASTNode* body;           /* hopefully an compound statement */
        } whileStatement;

        struct {
            struct ASTNode* init;
            struct ASTNode* condition;
            struct ASTNode* increment;
            struct ASTNode* body;
        } forStatement;

        /* declarations */

        struct {
            char* name;
            enum DataType type;
            struct ASTNode* init;
        } variableDeclaration;

        struct {
            char* name;
            enum DataType type;
        } parameterDeclaration;

        /* leaf expressions */

        struct {
            enum OpKind kind;
            struct ASTNode* left;
            struct ASTNode* right;
        } binaryExpression;

        struct {
            enum UnaryOpKind op;
            struct ASTNode* operand;
        } unaryExpression;

        struct {
            struct ASTNode* left;   /* destination */
            struct ASTNode* right;  /* expression */
        } assignExpression;

        struct {
            char* callee;
            struct ASTNode* arguments;
            size_t argument_count;
        } functionCall;

        struct {
            struct ASTNode* object;
            char* member_name;
            enum AccessKind kind;
        } memberAccess;

        struct {
            struct ASTNode* array;
            struct ASTNode* index;
        } arrayIndex;

        struct {
            enum DataType type;
            struct ASTNode* operand;
        } castExpression;
    };

    struct ASTNode *prev;
    struct ASTNode *next;
};

void astnode_unlink(struct ASTNode *node);
void astnode_link(struct ASTNode *node, struct ASTNode *new);

struct ASTNode *astnode_create_translation_unit(void);

#endif /* EMEX64C_ASTUNIT_H */
