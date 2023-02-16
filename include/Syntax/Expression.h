/*
 * Defines the CkExpression struct and
 * provides functions for expression trees.
*/

#ifndef CK_EXPRESSION_H_
#define CK_EXPRESSION_H_

#include "Lex.h"
#include "../Food.h"

/// <summary>
/// Creates an expression that only holds a type.
/// </summary>
/// <param name="type">The type.</param>
/// <returns></returns>
CkExpression *CkExpressionCreateType(
	CkArenaFrame *arena,
	CkFoodType *type);

/// <summary>
/// Creates a literal expression (an expression with no child nodes.)
/// </summary>
/// <param name="token">The literal value token.</param>
/// <param name="type">The type to be given over to the expression.</param>
/// <returns></returns>
CkExpression *CkExpressionCreateLiteral(
	CkArenaFrame *arena,
	const CkToken *token,
	const CkExpressionKind kind,
	CkFoodType *type);

/// <summary>
/// Creates a unary expression (an expression with one child node.)
/// </summary>
/// <param name="operator">The operator.</param>
/// <param name="operand">The child node.</param>
/// <param name="type">The type to be given over to the expression.</param>
/// <returns></returns>
CkExpression *CkExpressionCreateUnary(
	CkArenaFrame *arena,
	const CkToken *operator,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *operand);

/// <summary>
/// Creates a binary expression (an expression with two child nodes.)
/// </summary>
/// <param name="operator">The operator.</param>
/// <param name="left">The left child node.</param>
/// <param name="right">The right child node.</param>
/// <param name="type">The type to be given over to the expression.</param>
/// <returns></returns>
CkExpression *CkExpressionCreateBinary(
	CkArenaFrame *arena,
	const CkToken *operator,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right);

/// <summary>
/// Creates a ternary expression (an expression with three child nodes.)
/// </summary>
/// <param name="operator">The operator.</param>
/// <param name="left">The left child node.</param>
/// <param name="right">The right child node.</param>
/// <param name="extra">The third child node.</param>
/// <param name="type">The type to be given over to the expression.</param>
/// <returns></returns>
CkExpression *CkExpressionCreateTernary(
	CkArenaFrame *arena,
	const CkToken *operator,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right,
	CkExpression *extra);

/// <summary>
/// Duplicates an expression.
/// </summary>
/// <param name="arena">The arena to allocate the expression on.</param>
/// <param name="source">The source expression.</param>
/// <returns></returns>
CkExpression *CkExpressionDuplicate(CkArenaFrame *arena, CkExpression *source);

/// <summary>
/// Prints an expression.
/// </summary>
/// <param name="expression"></param>
void CkExpressionPrint(CkExpression *expression);

#endif
