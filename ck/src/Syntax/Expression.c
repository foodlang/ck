#include <include/Syntax/Expression.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

CkExpression *CkExpressionCreateLiteral(
	const CkToken *token,
	CkFoodType *type)
{
	CkExpression *expr = calloc(1, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), token, sizeof(CkToken));
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateUnary(
	const CkToken *operator,
	CkFoodType *type,
	CkExpression *operand)
{
	CkExpression *expr = calloc(1, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = operand;
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateBinary(
	const CkToken *operator,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right)
{
	CkExpression *expr = calloc(1, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = left;
	expr->right = right;
	expr->type = type;
	return expr;
}

CkExpression *CkExpressionCreateTernary(
	const CkToken *operator,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right,
	CkExpression *extra)
{
	CkExpression *expr = calloc(1, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = left;
	expr->right = right;
	expr->extra = extra;
	expr->type = type;
	return expr;
}

void CkExpressionDelete(CkExpression *expression)
{
	// Child nodes must also be deleted.
	if (expression->left)
		CkExpressionDelete(expression->left);
	if (expression->right)
		CkExpressionDelete(expression->right);
	if (expression->extra)
		CkExpressionDelete(expression->extra);
	if (expression->type)
		CkFoodDeleteTypeInstance(expression->type);
	free(expression);
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
	s_ExprPrintTab(0, expression);
}
