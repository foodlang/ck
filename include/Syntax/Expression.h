/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines functions used when creating or duplicating expressions.
 * NOTE: The expression type is defined in include/Types.h
 *
 ***************************************************************************/

#ifndef CK_EXPRESSION_H_
#define CK_EXPRESSION_H_

#include <Types.h>
#include "Lex.h"
#include <Food.h>

// Creates an expression that only holds a type.
CkExpression *CkExpressionCreateType(
	CkArenaFrame *arena,
	CkFoodType *type);

// Creates a literal expression (an expression with no child nodes.)
CkExpression *CkExpressionCreateLiteral(
	CkArenaFrame *arena,
	const CkToken *token,
	const CkExpressionKind kind,
	CkFoodType *type);

// Creates a unary expression (an expression with one child node.)
CkExpression *CkExpressionCreateUnary(
	CkArenaFrame *arena,
	const CkToken *op,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *operand);

// Creates a binary expression (an expression with two child nodes.)
CkExpression *CkExpressionCreateBinary(
	CkArenaFrame *arena,
	const CkToken *op,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right);

// Creates a ternary expression (an expression with three child nodes.)
CkExpression *CkExpressionCreateTernary(
	CkArenaFrame *arena,
	const CkToken *op,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right,
	CkExpression *extra);

// Duplicates an expression.
CkExpression *CkExpressionDuplicate(CkArenaFrame *arena, CkExpression *source);

// Prints an expression.
void CkExpressionPrint(CkExpression *expression);

#endif
