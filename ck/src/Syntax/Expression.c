#include <include/Syntax/Expression.h>
#include <ckmem/CDebug.h>
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
	expr->kind = CK_EXPRESSION_TYPE;
	return expr;
}

CkExpression *CkExpressionCreateLiteral(
	CkArenaFrame *arena,
	const CkToken *token,
	const CkExpressionKind kind,
	CkFoodType *type)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(token);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), token, sizeof(CkToken));
	expr->type = type;
	expr->kind = kind;
	return expr;
}

CkExpression *CkExpressionCreateUnary(
	CkArenaFrame *arena,
	const CkToken *operator,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *operand)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(operator);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = operand;
	expr->type = type;
	expr->kind = kind;
	return expr;
}

CkExpression *CkExpressionCreateBinary(
	CkArenaFrame *arena,
	const CkToken *operator,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(operator);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = left;
	expr->right = right;
	expr->type = type;
	expr->kind = kind;
	return expr;
}

CkExpression *CkExpressionCreateTernary(
	CkArenaFrame *arena,
	const CkToken *operator,
	const CkExpressionKind kind,
	CkFoodType *type,
	CkExpression *left,
	CkExpression *right,
	CkExpression *extra)
{
	CkExpression *expr;

	CK_ARG_NON_NULL(arena);
	CK_ARG_NON_NULL(operator);

	expr = CkArenaAllocate(arena, sizeof(CkExpression));
	memcpy_s(&expr->token, sizeof(CkToken), operator, sizeof(CkToken));
	expr->left = left;
	expr->right = right;
	expr->extra = extra;
	expr->type = type;
	expr->kind = kind;
	return expr;
}

CkExpression *CkExpressionDuplicate(CkArenaFrame *arena, CkExpression *source)
{
	CkExpression *dest = CkArenaAllocate(arena, sizeof(CkExpression));
	if (source->left)
		dest->left = CkExpressionDuplicate(arena, source->left);
	if (source->right)
		dest->right = CkExpressionDuplicate(arena, source->right);
	if (source->extra)
		dest->extra = CkExpressionDuplicate(arena, source->extra);
	dest->kind = source->kind;
	dest->isLValue = source->isLValue;
	memcpy_s(&dest->token, sizeof(CkToken), &source->token, sizeof(CkToken));
	dest->type = CkFoodCopyTypeInstance(arena, source->type);
	return dest;
}

static void s_ExprPrintTab(int tab, CkExpression *expression)
{
	for (int i = 0; i < tab; i++)
		printf("  ");
	printf_s("%c:%llu\n", (char)expression->token.kind, expression->token.value.u64);
	if (expression->extra)
		s_ExprPrintTab(tab + 1, expression->extra);
	if (expression->left)
		s_ExprPrintTab(tab + 1, expression->left);
	if (expression->right)
		s_ExprPrintTab(tab + 1, expression->right);
}

void CkExpressionPrint(CkExpression *expression)
{
	CK_ARG_NON_NULL(expression);
	s_ExprPrintTab(0, expression);
}
