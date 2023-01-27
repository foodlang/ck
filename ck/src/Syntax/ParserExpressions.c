#include <include/Syntax/ParserExpressions.h>
#include <include/Syntax/ParserTypes.h>
#include <include/FileIO.h>
#include <include/CDebug.h>

#include <stdio.h>
#include <string.h>

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
				parser->arena,
				&token,
				CkFoodCreateTypeInstance(parser->arena, CK_FOOD_I32, 0, NULL)
			);

		// If the value cannot be stored as a 32-bit int, use a 64-bit
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CkFoodCreateTypeInstance(parser->arena, CK_FOOD_I64, 0, NULL)
		);

	case 'F':
		// Default is float
		if (FloatEqual((double)token.value.f32, token.value.f64))
			return CkExpressionCreateLiteral(
				parser->arena,
				&token,
				CkFoodCreateTypeInstance(parser->arena, CK_FOOD_F32, 0, NULL)
			);

		// If the value cannot be stored as a 32-bit float, use a 64-bit
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CkFoodCreateTypeInstance(parser->arena, CK_FOOD_F64, 0, NULL)
		);

		// True is a boolean constant
	case KW_TRUE:
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CkFoodCreateTypeInstance(parser->arena, CK_FOOD_BOOL, 0, NULL));

		// False is a boolean constant
	case KW_FALSE:
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CkFoodCreateTypeInstance(parser->arena, CK_FOOD_BOOL, 0, NULL));

		// Null is a pointer constant
	case KW_NULL:
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CkFoodCreateTypeInstance(
				parser->arena,
				CK_FOOD_POINTER,
				0,
				CkFoodCreateTypeInstance(
					parser->arena,
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

		return CkExpressionCreateLiteral(parser->arena, &token, CkFoodCreateTypeInstance(parser->arena, CK_FOOD_VOID, 0, NULL));

	case KW_SIZEOF:
	{
		CkFoodType *type;
		CkToken exprToken;
		memcpy_s(&exprToken, sizeof(CkToken), &token, sizeof(CkToken));
		CkParserReadToken(parser, &token);
		if (token.kind != '(') {
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing opening bracket in sizeof() operator.");
		}
		type = CkParserType(parser);
		if (!type) {
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The operand of sizeof() must a type.");
		}
		CkParserReadToken(parser, &token);
		if (token.kind != ')') {
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in sizeof() operator.");
		}
		return CkExpressionCreateLiteral(parser->arena, &exprToken, type);
	}

	case KW_ALIGNOF:
	{
		CkFoodType *type;
		CkToken exprToken;
		memcpy_s(&exprToken, sizeof(CkToken), &token, sizeof(CkToken));
		CkParserReadToken(parser, &token);
		if (token.kind != '(') {
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing opening bracket in alignof() operator.");
		}
		type = CkParserType(parser);
		if (!type) {
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The operand of alignof() must a type.");
		}
		CkParserReadToken(parser, &token);
		if (token.kind != ')') {
			CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in alignof() operator.");
		}
		return CkExpressionCreateLiteral(parser->arena, &exprToken, type);
	}

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
			acc = CkExpressionCreateUnary(parser->arena, &token, NULL, acc);
			break;

			// Member access / pointer member access
		case '.':
		case CKTOK2('-', '>'):
			acc = CkExpressionCreateBinary(parser->arena, &token, NULL, acc, s_ParsePrimaryExpression(parser));
			break;

			// Array subscript
		case '[':
			acc = CkExpressionCreateBinary(parser->arena, &token, NULL, acc, CkParserExpression(parser));
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

/// <summary>
/// Parses prefix unary operators and casts.
/// </summary>
/// <param name="parser"></param>
/// <returns></returns>
static CkExpression *s_ParseLevel2(CkParserInstance *parser)
{
	CkToken token;
	CkExpression *accumulator;

	CkParserReadToken(parser, &token);

	// Associativity: RIGHT -> LEFT
	// Therefore, no loop is required, only a recursive call.
	switch (token.kind) {

	case CKTOK2('+', '+'):
	case CKTOK2('-', '-'):
	case '+':
	case '-':
	case '!':
	case '~':
	case '*':
	case '&':
		accumulator = CkExpressionCreateUnary(parser->arena, &token, NULL, s_ParseLevel2(parser));
		break;
		
		// TODO: Implement C-style casting

		// No level-2 operator
	default:
		CkParserRewind(parser, 1);
		accumulator = s_ParseLevel1(parser);
		break;
	}

	return accumulator;
}

CkExpression *CkParserExpression(CkParserInstance *parser)
{
	CK_ARG_NON_NULL(parser)
	return s_ParseLevel2(parser);
}
