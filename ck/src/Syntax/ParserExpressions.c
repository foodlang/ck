#include <include/Syntax/ParserExpressions.h>
#include <include/Syntax/ParserTypes.h>
#include <include/FileIO.h>
#include <include/CDebug.h>

#include <stdio.h>
#include <string.h>

/// <summary>
/// An entry in the precedence table for binary operators.
/// </summary>
typedef struct BinaryOperatorPrecedenceEntry
{
	/// <summary>
	/// The operator.
	/// </summary>
	uint64_t op;

	/// <summary>
	/// The precedence.
	/// </summary>
	uint8_t  prec;

} BinaryOperatorPrecedenceEntry;

/// <summary>
/// A table having operators as its first entries and
/// precedences as its second.
/// </summary>
static BinaryOperatorPrecedenceEntry s_binaryPrecedences[] =
{
	{ '*', 9 },
	{ '/', 9 },
	{ '%', 9 },

	{ '-', 8 },
	{ '+', 8 },

	{ '>', 7 },
	{ '<', 7 },
	{ CKTOK2('<', '='), 7 },
	{ CKTOK2('>', '='), 7 },

	{ CKTOK2('!', '='), 6 },
	{ CKTOK2('=', '='), 6 },

	{ '&', 5 },

	{ '^', 4 },

	{ '|', 3 },

	{ CKTOK2('&', '&'), 2 },

	{ CKTOK2('|', '|'), 1 },
};

/// <summary>
/// Looks up in the binary precedence table and attempts to get 
/// the precedence of a given operator.
/// </summary>
/// <param name="op">The operator to look up.</param>
/// <returns></returns>
static uint8_t s_BinaryOpPrec(uint64_t op)
{
	for (
		size_t i = 0;
		i < sizeof(s_binaryPrecedences) / sizeof(BinaryOperatorPrecedenceEntry);
		i++) {
		if (s_binaryPrecedences[i].op == op)
			return s_binaryPrecedences[i].prec;
	}
	return 0;
}

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

/// <summary>
/// Parses binary operators. Uses Pratt parsing.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseBinary(uint8_t parentPrec, CkParserInstance *parser)
{
	CkToken token;
	CkExpression *acc = s_ParseLevel2(parser);

	while (TRUE) {
		uint8_t prec;
		CkExpression *right;

		CkParserReadToken(parser, &token);
		prec = s_BinaryOpPrec(token.kind);
		if (prec == 0 || prec <= parentPrec) {
			CkParserRewind(parser, 1);
			break;
		}

		right = s_ParseBinary(prec, parser);
		acc = CkExpressionCreateBinary(parser->arena, &token, NULL, acc, right);
	}

	return acc;
}

/// <summary>
/// Parses a Food-style cast (expr => T)
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseFoodCast(CkParserInstance *parser)
{
	CkToken op;
	CkExpression *acc = s_ParseBinary(0, parser);

	CkParserReadToken(parser, &op);
	while (op.kind == CKTOK2('=', '>')) {
		CkFoodType *t = CkParserType(parser);
		if (t == NULL) {
			CkDiagnosticThrow(parser->pDhi, op.position, CK_DIAG_SEVERITY_ERROR, "",
				"Expected a type in Food-style cast");
		}
		acc = CkExpressionCreateBinary(parser->arena, &op, NULL, acc, CkExpressionCreateType(parser->arena, t));
		CkParserReadToken(parser, &op);
	}
	CkParserRewind(parser, 1);
	return acc;
}

/// <summary>
/// Parses a conditional expression.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseConditional(CkParserInstance *parser)
{
	CkToken token;
	CkToken op;
	CkExpression *left;
	CkExpression *right;
	CkExpression *extra = s_ParseFoodCast(parser);

	CkParserReadToken(parser, &token);
	memcpy_s(&op, sizeof(CkToken), &token, sizeof(CkToken));

	// If it not a conditional expression
	if (token.kind != '?') {
		CkParserRewind(parser, 1);
		return extra;
	}

	left = s_ParseConditional(parser);
	CkParserReadToken(parser, &token);
	if (token.kind != ':') {
		CkDiagnosticThrow(parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Expected colon in conditional expression.");
	}
	right = s_ParseConditional(parser);

	return CkExpressionCreateTernary(parser->arena, &op, NULL, left, right, extra);
}

/// <summary>
/// Parses an assignment expression.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseAssign(CkParserInstance *parser)
{
	CkToken op;

	CkExpression *left = s_ParseConditional(parser);
	CkParserReadToken(parser, &op);
	switch (op.kind) {
	case '=':
	case CKTOK2('+', '='):
	case CKTOK2('-', '='):
	case CKTOK2('*', '='):
	case CKTOK2('/', '='):
	case CKTOK2('%', '='):
	case CKTOK2('&', '='):
	case CKTOK2('|', '='):
	case CKTOK2('^', '='):
	case CKTOK3('<', '<', '='):
	case CKTOK3('>', '>', '='):
		return CkExpressionCreateBinary(parser->arena, &op, NULL, left, s_ParseConditional(parser));

		// Not an assignment
	default:
		CkParserRewind(parser, 1);
		return left;
	}
}

/// <summary>
/// Parses a compound expression.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseCompound(CkParserInstance *parser)
{
	CkToken op;
	CkExpression *acc = s_ParseAssign(parser);

	CkParserReadToken(parser, &op);
	while (op.kind == ',') {
		acc = CkExpressionCreateBinary(parser->arena, &op, NULL, acc, s_ParseAssign(parser));
		CkParserReadToken(parser, &op);
	}
	CkParserRewind(parser, 1);
	return acc;
}

CkExpression *CkParserExpression(CkParserInstance *parser)
{
	CK_ARG_NON_NULL(parser)
	return s_ParseCompound(parser);
}
