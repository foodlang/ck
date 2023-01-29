/*
 * Defines the CkExpression struct and
 * provides functions for expression trees.
*/

#ifndef CK_EXPRESSION_H_
#define CK_EXPRESSION_H_

#include "Lex.h"
#include "../Food.h"

/// <summary>
/// The kind of an expression (its operator.)
/// </summary>
typedef enum CkExpressionKind
{

	/// <summary>
	/// Misc. unknown expression identifier
	/// </summary>
	CK_EXPRESSION_DUMMY,

	CK_EXPRESSION_COMPOUND_LITERAL,
	CK_EXPRESSION_INTEGER_LITERAL,
	CK_EXPRESSION_FLOAT_LITERAL,
	CK_EXPRESSION_STRING_LITERAL,
	CK_EXPRESSION_BOOL_LITERAL,
	CK_EXPRESSION_TYPE,
	CK_EXPRESSION_SIZEOF,
	CK_EXPRESSION_ALIGNOF,
	CK_EXPRESSION_NAMEOF,
	CK_EXPRESSION_TYPEOF,

	CK_EXPRESSION_POSTFIX_INC,
	CK_EXPRESSION_POSTFIX_DEC,
	CK_EXPRESSION_FUNCCALL,
	CK_EXPRESSION_SUBSCRIPT,
	CK_EXPRESSION_MEMBER_ACCESS,
	CK_EXPRESSION_POINTER_MEMBER_ACCESS,

	CK_EXPRESSION_PREFIX_INC,
	CK_EXPRESSION_PREFIX_DEC,
	CK_EXPRESSION_UNARY_PLUS,
	CK_EXPRESSION_UNARY_MINUS,
	CK_EXPRESSION_LOGICAL_NOT,
	CK_EXPRESSION_BITWISE_NOT,
	CK_EXPRESSION_C_CAST,
	CK_EXPRESSION_DEREFERENCE,
	CK_EXPRESSION_ADDRESS_OF,

	CK_EXPRESSION_MUL,
	CK_EXPRESSION_DIV,
	CK_EXPRESSION_MOD,

	CK_EXPRESSION_ADD,
	CK_EXPRESSION_SUB,

	CK_EXPRESSION_LEFT_SHIFT,
	CK_EXPRESSION_RIGHT_SHIFT,

	CK_EXPRESSION_LOWER,
	CK_EXPRESSION_LOWER_EQUAL,
	CK_EXPRESSION_GREATER,
	CK_EXPRESSION_GREATER_EQUAL,

	CK_EXPRESSION_EQUAL,
	CK_EXPRESSION_NOT_EQUAL,

	CK_EXPRESSION_BITWISE_AND,

	CK_EXPRESSION_BITWISE_XOR,

	CK_EXPRESSION_BITWISE_OR,

	CK_EXPRESSION_LOGICAL_AND,

	CK_EXPRESSION_LOGICAL_OR,

	CK_EXPRESSION_FOOD_CAST,

	CK_EXPRESSION_CONDITIONAL,

	CK_EXPRESSION_ASSIGN,
	CK_EXPRESSION_ASSIGN_SUM,
	CK_EXPRESSION_ASSIGN_DIFF,
	CK_EXPRESSION_ASSIGN_PRODUCT,
	CK_EXPRESSION_ASSIGN_QUOTIENT,
	CK_EXPRESSION_ASSIGN_REMAINDER,
	CK_EXPRESSION_ASSIGN_LEFT_SHIFT,
	CK_EXPRESSION_ASSIGN_RIGHT_SHIFT,
	CK_EXPRESSION_ASSIGN_AND,
	CK_EXPRESSION_ASSIGN_XOR,
	CK_EXPRESSION_ASSIGN_OR,

	CK_EXPRESSION_COMPOUND,

} CkExpressionKind;

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
	/// The kind of the expression (its operator.)
	/// This cannot be stored with the token, cause the
	/// main expression token of two operators can be the same.
	/// (e.g. both x++ and ++x have ++ has their main token.)
	/// </summary>
	CkExpressionKind kind;

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
