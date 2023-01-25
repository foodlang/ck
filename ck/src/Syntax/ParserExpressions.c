#include <include/Syntax/ParserExpressions.h>
#include <include/FileIO.h>
#include <stdio.h>

static CkExpression *s_ParsePrimaryExpression(CkParserInstance *parser)
{
	CkToken token;

	CkParserReadToken(parser, &token);

	switch (token.kind) {

		// Literal integer
	case '0':
		// Default is int
		if (token.value.u32 == token.value.u64)
			return CkExpressionCreateLiteral(
				&token,
				CkFoodCreateTypeInstance(CK_FOOD_I32, 0, NULL)
			);

		// If the value cannot be stored as a 32-bit int, use a 64-bit
		return CkExpressionCreateLiteral(
			&token,
			CkFoodCreateTypeInstance(CK_FOOD_I64, 0, NULL)
		);

	case 'F':
		// Default is float
		if (token.value.f64 == (double)(float)token.value.f64)
			return CkExpressionCreateLiteral(
				&token,
				CkFoodCreateTypeInstance(CK_FOOD_F32, 0, NULL)
			);

		// If the value cannot be stored as a 32-bit float, use a 64-bit
		return CkExpressionCreateLiteral(
			&token,
			CkFoodCreateTypeInstance(CK_FOOD_F64, 0, NULL)
		);

		// True is a boolean constant
	case KW_TRUE:
		return CkExpressionCreateLiteral(
			&token,
			CkFoodCreateTypeInstance(CK_FOOD_BOOL, 0, NULL));

		// False is a boolean constant
	case KW_FALSE:
		return CkExpressionCreateLiteral(
			&token,
			CkFoodCreateTypeInstance(CK_FOOD_BOOL, 0, NULL));

		// Null is a pointer constant
	case KW_NULL:
		return CkExpressionCreateLiteral(
			&token,
			CkFoodCreateTypeInstance(
				CK_FOOD_POINTER,
				0,
				CkFoodCreateTypeInstance(
					CK_FOOD_VOID,
					0,
					NULL
				)
			));

		// Parenthesized expressions
	case '(':
	{
		CkExpression *expr = CkParserExpression(parser);
		CkParserReadToken(parser, &token);
		if (token.kind != ')') {
			
		}
	}

	}
}

CkExpression *CkParserExpression(CkParserInstance *parser)
{
	
}
