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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <emex64lib/c/ASTUnit.h>

static inline void astnode_unlink_list(struct ASTNode *node)
{
    struct ASTNode *current = node;

    /* walking back to the most previous node */
    while(current->prev != NULL)
    {
        current = current->prev;
    }

    /* unlink each node recursively */
    while(current != NULL)
    {
        struct ASTNode *deathNode = current;
        current = deathNode->next;
        astnode_unlink(deathNode);
    }
}

void astnode_unlink(struct ASTNode *node)
{
    switch(node->kind)
    {
        case ASTNodeKindTranslationUnit:
            astnode_unlink_list(node->translationUnit.declarations);
            break;
        case ASTNodeKindFunctionDeclaration:
            astnode_unlink_list(node->functionDeclaration.parameters);
            break;
        case ASTNodeKindFunctionDefinition:
            astnode_unlink_list(node->functionDefinition.body);
            astnode_unlink_list(node->functionDefinition.parameters);
            free(node->functionDefinition.name);
            break;
        case ASTNodeKindStructDeclaration:
            astnode_unlink_list(node->structDeclaration.members);
            free(node->structDeclaration.name);
            break;
        case ASTNodeKindCompoundStatement:
            astnode_unlink_list(node->compoundStatement.body);
            break;
        case ASTNodeKindReturnStatement:
        case ASTNodeKindExpressionStatement:
            astnode_unlink_list(node->expressionStatement.expression);
            break;
        case ASTNodeKindIfStatement:
            astnode_unlink_list(node->ifStatement.condition);
            astnode_unlink_list(node->ifStatement.then_branch);
            astnode_unlink_list(node->ifStatement.else_branch);
            break;
        case ASTNodeKindWhileStatement:
            astnode_unlink_list(node->whileStatement.condition);
            astnode_unlink_list(node->whileStatement.body);
            break;
        case ASTNodeKindForStatement:
            astnode_unlink_list(node->forStatement.condition);
            astnode_unlink_list(node->forStatement.init);
            astnode_unlink_list(node->forStatement.increment);
            astnode_unlink_list(node->forStatement.body);
            break;
        case ASTNodeKindVariableDeclaration:
            astnode_unlink_list(node->variableDeclaration.init);
            free(node->variableDeclaration.name);
            break;
        case ASTNodeKindParameterDeclaration:
            free(node->parameterDeclaration.name);
            break;
        case ASTNodeKindBinaryExpression:
            astnode_unlink_list(node->binaryExpression.left);
            astnode_unlink_list(node->binaryExpression.right);
            break;
        case ASTNodeKindUnaryExpression:
            astnode_unlink_list(node->unaryExpression.operand);
            break;
        case ASTNodeKindAssignExpression:
            astnode_unlink_list(node->assignExpression.left);
            astnode_unlink_list(node->assignExpression.right);
            break;
        case ASTNodeKindFunctionCall:
            astnode_unlink_list(node->functionCall.arguments);
            free(node->functionCall.callee);
            break;
        case ASTNodeKindMemberAccess:
            astnode_unlink_list(node->memberAccess.object);
            free(node->memberAccess.member_name);
            break;
        case ASTNodeKindArrayIndex:
            astnode_unlink_list(node->arrayIndex.array);
            astnode_unlink_list(node->arrayIndex.index);
            break;
        case ASTNodeKindCastExpression:
            astnode_unlink_list(node->castExpression.operand);
            break;
        default:
            break;
    }

    if(node->next != NULL)
    {
        node->next->prev = node->prev;
    }

    if(node->prev != NULL)
    {
        node->prev->next = node->next;
    }

    free(node);
}

void astnode_link(struct ASTNode *node,
                  struct ASTNode *new)
{
    if(node->next != NULL)
    {
        node->next->prev = new;
    }

    new->prev = node;
    node->next = new;
}

struct ASTNode *astnode_create_translation_unit(void)
{
    struct ASTNode *node = malloc(sizeof(struct ASTNode));
    if(node == NULL)
    {
        return NULL;
    }

    node->kind = ASTNodeKindTranslationUnit;
    node->translationUnit.declarations = NULL;

    return node;
}
