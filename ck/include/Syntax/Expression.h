/*
 * Defines the CkExpression struct and
 * provides functions for expression trees.
*/

#ifndef CK_EXPRESSION_H_
#define CK_EXPRESSION_H_

#include "Lex.h"
#include "../Food.h"

/// <summary>
/// A parser expression.
/// </summary>
typedef struct CkExpression
{
	/// <summary>
	/// The main token of the expression.
	/// Usually an operator, or the literal
	/// value token with literal expressions.
	/// </summary>
	CkToken token;

	/// <summary>
	/// The type of the expression. This is a passed
	/// pointer. Ideally, this should be allocated on 
	/// the same arena as the expression to prevent
	/// object lifetime issues.
	/// </summary>
	CkFoodType *type;

	/// <summary>
	/// The left child node.
	/// </summary>
	struct CkExpression *left;

	/// <summary>
	/// The right child node.
	/// </summary>
	struct CkExpression *right;

	/// <summary>
	/// This child node is used to store the third
	/// member of ternary expressions.
	/// </summary>
	struct CkExpression *extra;

	/// <summary>
	/// If true, this expression can be referenced.
	/// </summary>
	bool_t isLValue;

} CkExpression;

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
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right,
	CkExpression *extra);

/// <summary>
/// Prints an expression.
/// </summary>
/// <param name="expression"></param>
void CkExpressionPrint(CkExpression *expression);

#endif
