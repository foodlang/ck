#include <include/Syntax/Expression.h>
#include <include/CDebug.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

CkExpression *CkExpressionCreateType(
	CkArenaFrame *arena,
	CkFoodType *type)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateLiteral(
	CkArenaFrame *arena,
	const CkToken *token,
	CkFoodType *type)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(token);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), token, sizeof(CkToken));
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateUnary(
	CkArenaFrame *arena,
	const CkToken *operator,
	CkFoodType *type,
	CkExpression *operand)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(operator);
	CK_ARG_NON_NULL(operand);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = operand;
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateBinary(
	CkArenaFrame *arena,
	const CkToken *operator,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(operator);
	CK_ARG_NON_NULL(left);
	CK_ARG_NON_NULL(right);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = left;
	expr->right = right;
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateTernary(
	CkArenaFrame *arena,
	const CkToken *operator,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right,
	CkExpression *extra)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(operator);
	CK_ARG_NON_NULL(left);
	CK_ARG_NON_NULL(right);
	CK_ARG_NON_NULL(extra);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = left;
	expr->right = right;
	expr->extra = extra;
	expr->type = type;
	return expr;
}

static void s_ExprPrintTab(int tab, CkExpression *expression)
{
	for (int i = 0; i < tab; i++)
		printf("  ");
	printf_s("%c:%llu\n", (char)expression->token.kind, expression->token.value.u64);
	if (expression->left)
		s_ExprPrintTab(tab + 1, expression->left);
	if (expression->right)
		s_ExprPrintTab(tab + 1, expression->right);
	if (expression->extra)
		s_ExprPrintTab(tab + 1, expression->extra);
}

void CkExpressionPrint(CkExpression *expression)
{
	CK_ARG_NON_NULL(expression);
	s_ExprPrintTab(0, expression);
}
