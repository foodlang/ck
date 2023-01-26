#include <include/Syntax/ParserExpressions.h>
#include <include/FileIO.h>
#include <stdio.h>

/// <summary>
/// Parses a primary value.
/// </summary>
/// <returns></returns>
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
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in parenthesized expression.");
		}
		return expr;
	}
		// Anything that isn't an expression but is passed as an expression.
	default:
		CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"An expression was expected in this context.");

		return CkExpressionCreateLiteral(&token, CkFoodCreateTypeInstance(CK_FOOD_VOID, 0, NULL));

		// TODO:
		// Implement sizeof() and alignof()
	}
}

/// <summary>
/// Parses access, postfix increment and decrement operators.
/// </summary>
/// <param name="parser"></param>
/// <returns></returns>
static CkExpression *s_ParseLevel1(CkParserInstance *parser)
{
	CkToken token;
	CkExpression *acc;

	// The operand and next token are read.
	acc = s_ParsePrimaryExpression(parser);
	CkParserReadToken(parser, &token);

	while (TRUE) {
		switch (token.kind) {
			// Postfix increment and decrement
		case CKTOK2('+', '+'):
		case CKTOK2('-', '-'):
			acc = CkExpressionCreateUnary(&token, NULL, acc);
			break;

			// Member access / pointer member access
		case '.':
		case CKTOK2('-', '>'):
			acc = CkExpressionCreateBinary(&token, NULL, acc, s_ParsePrimaryExpression(parser));
			break;

			// Array subscript
		case '[':
			acc = CkExpressionCreateBinary(&token, NULL, acc, CkParserExpression(parser));
			CkParserReadToken(parser, &token);
			if (token.kind != ']') {
				CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Missing closing bracket in array subscript operation.");
			}
			break;
		
			// If not an access operator / increment / decrement
		default:
			CkParserRewind(parser, 1);
			goto Leave;
		}
		CkParserReadToken(parser, &token);
	}

Leave:
	return acc;
}

CkExpression *CkParserExpression(CkParserInstance *parser)
{
	return s_ParseLevel1(parser);
}
