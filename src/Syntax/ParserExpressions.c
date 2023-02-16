#include <Syntax/ParserExpressions.h>
#include <Syntax/ParserTypes.h>
#include <FileIO.h>
#include <CDebug.h>

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

	/// <summary>
	/// The kind of the operator.
	/// </summary>
	CkExpressionKind kind;

} BinaryOperatorPrecedenceEntry;

/// <summary>
/// A table having operators as its first entries and
/// precedences as its second.
/// </summary>
static BinaryOperatorPrecedenceEntry s_binaryPrecedences[] =
{
	{ '*', 10, CK_EXPRESSION_MUL },
	{ '/', 10, CK_EXPRESSION_DIV },
	{ '%', 10, CK_EXPRESSION_MOD },

	{ '-', 9, CK_EXPRESSION_SUB },
	{ '+', 9, CK_EXPRESSION_ADD },

	{ CKTOK2( '<', '<' ), 8, CK_EXPRESSION_LEFT_SHIFT },
	{ CKTOK2( '>', '>' ), 8, CK_EXPRESSION_RIGHT_SHIFT },

	{ '>', 7, CK_EXPRESSION_GREATER },
	{ '<', 7, CK_EXPRESSION_LOWER },
	{ CKTOK2( '<', '=' ), 7, CK_EXPRESSION_LOWER_EQUAL },
	{ CKTOK2( '>', '=' ), 7, CK_EXPRESSION_GREATER_EQUAL },

	{ CKTOK2( '!', '=' ), 6, CK_EXPRESSION_NOT_EQUAL },
	{ CKTOK2( '=', '=' ), 6, CK_EXPRESSION_EQUAL },

	{ '&', 5, CK_EXPRESSION_BITWISE_AND },

	{ '^', 4, CK_EXPRESSION_BITWISE_XOR },

	{ '|', 3, CK_EXPRESSION_BITWISE_OR },

	{ CKTOK2( '&', '&' ), 2, CK_EXPRESSION_LOGICAL_AND, },

	{ CKTOK2( '|', '|' ), 1, CK_EXPRESSION_LOGICAL_OR, },
};

/// <summary>
/// Looks up in the binary precedence table and attempts to get 
/// the precedence of a given operator.
/// </summary>
/// <param name="op">The operator to look up.</param>
/// <returns></returns>
static uint8_t s_BinaryOpPrec( uint64_t op )
{
	for (
		size_t i = 0;
		i < sizeof( s_binaryPrecedences ) / sizeof( BinaryOperatorPrecedenceEntry );
		i++ ) {
		if ( s_binaryPrecedences[i].op == op )
			return s_binaryPrecedences[i].prec;
	}
	return 0;
}

/// <summary>
/// Looks up the binary precedence table and attempts to get
/// the expression kind of a given operator.
/// </summary>
/// <param name="op">The operator to look up.</param>
/// <returns></returns>
static CkExpressionKind s_BinaryOpKind( uint64_t op )
{
	for (
		size_t i = 0;
		i < sizeof( s_binaryPrecedences ) / sizeof( BinaryOperatorPrecedenceEntry );
		i++ ) {
		if ( s_binaryPrecedences[i].op == op )
			return s_binaryPrecedences[i].kind;
	}
	return CK_EXPRESSION_DUMMY;
}

/// <summary>
/// Parses a primary value.
/// </summary>
/// <returns></returns>
static CkExpression *s_ParsePrimaryExpression( CkParserInstance *parser )
{
	CkToken token;

	CkParserReadToken( parser, &token );

	switch ( token.kind ) {

		// Literal integer
	case '0':

		// Default is int
		if ( token.value.u32 == token.value.u64 )
			return CkExpressionCreateLiteral(
				parser->arena,
				&token,
				CK_EXPRESSION_INTEGER_LITERAL,
				CkFoodCreateTypeInstance( parser->arena, CK_FOOD_I32, 0, NULL )
			);

		// If the value cannot be stored as a 32-bit int, use a 64-bit
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_INTEGER_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_I64, 0, NULL )
		);

	case 'F':
		// Default is float
		if ( FloatEqual( (double)token.value.f32, token.value.f64 ) )
			return CkExpressionCreateLiteral(
				parser->arena,
				&token,
				CK_EXPRESSION_FLOAT_LITERAL,
				CkFoodCreateTypeInstance( parser->arena, CK_FOOD_F32, 0, NULL )
			);

		// If the value cannot be stored as a 32-bit float, use a 64-bit
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_FLOAT_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_F64, 0, NULL )
		);

		// True is a boolean constant
	case KW_TRUE:
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_BOOL_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_BOOL, 0, NULL ) );

		// False is a boolean constant
	case KW_FALSE:
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_BOOL_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_BOOL, 0, NULL ) );

		// Null is a pointer constant
	case KW_NULL:
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_INTEGER_LITERAL,
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
			) );

		// Parenthesized expressions
	case '(':
	{
		CkExpression *expr = CkParserExpression( parser );
		CkParserReadToken( parser, &token );
		if ( token.kind != ')' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in parenthesized expression." );
		}
		return expr;
	}

	case KW_SIZEOF:
	{
		CkFoodType *type;
		CkToken exprToken = token;
		CkParserReadToken( parser, &token );
		if ( token.kind != '(' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing opening bracket in sizeof() operator." );
		}
		type = CkParserType( parser );
		if ( !type ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The operand of sizeof() must a type." );
		}
		CkParserReadToken( parser, &token );
		if ( token.kind != ')' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in sizeof() operator." );
		}
		return CkExpressionCreateUnary(
			parser->arena,
			&exprToken,
			CK_EXPRESSION_SIZEOF,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_U64, 0, NULL ),
			CkExpressionCreateType( parser->arena, type ) );
	}

	case KW_ALIGNOF:
	{
		CkFoodType *type;
		CkToken exprToken = token;
		CkParserReadToken( parser, &token );
		if ( token.kind != '(' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing opening bracket in alignof() operator." );
		}
		type = CkParserType( parser );
		if ( !type ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The operand of alignof() must a type." );
		}
		CkParserReadToken( parser, &token );
		if ( token.kind != ')' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in alignof() operator." );
		}
		return CkExpressionCreateUnary(
			parser->arena,
			&exprToken,
			CK_EXPRESSION_ALIGNOF,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_U64, 0, NULL ),
			CkExpressionCreateType( parser->arena, type ) );
	}

	// Anything that isn't an expression but is passed as an expression.
	default:
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"An expression was expected in this context." );

		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_DUMMY,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_VOID, 0, NULL ) );

	}
}

/// <summary>
/// Parses access, postfix increment and decrement operators.
/// </summary>
/// <param name="parser"></param>
/// <returns></returns>
static CkExpression *s_ParseLevel1( CkParserInstance *parser )
{
	CkToken token;
	CkExpression *acc;

	// The operand and next token are read.
	acc = s_ParsePrimaryExpression( parser );
	CkParserReadToken( parser, &token );

	while ( TRUE ) {
		switch ( token.kind ) {
			// Postfix increment and decrement
		case CKTOK2( '+', '+' ):
		case CKTOK2( '-', '-' ):
			acc = CkExpressionCreateUnary( parser->arena, &token, CK_EXPRESSION_POSTFIX_INC, NULL, acc );
			break;

			// Member access / pointer member access
		case '.':
			acc = CkExpressionCreateBinary(
				parser->arena,
				&token,
				CK_EXPRESSION_MEMBER_ACCESS,
				NULL,
				acc,
				s_ParsePrimaryExpression( parser ) );
			break;

		case CKTOK2( '-', '>' ):
			acc = CkExpressionCreateBinary(
				parser->arena,
				&token,
				CK_EXPRESSION_POINTER_MEMBER_ACCESS,
				NULL,
				acc,
				s_ParsePrimaryExpression( parser ) );
			break;

			// Array subscript
		case '[':
			acc = CkExpressionCreateBinary( parser->arena, &token, CK_EXPRESSION_SUBSCRIPT, NULL, acc, CkParserExpression( parser ) );
			CkParserReadToken( parser, &token );
			if ( token.kind != ']' ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Missing closing bracket in array subscript operation." );
			}
			break;

			// If not an access operator / increment / decrement
		default:
			CkParserRewind( parser, 1 );
			goto Leave;
		}
		CkParserReadToken( parser, &token );
	}

Leave:
	return acc;
}

/// <summary>
/// Parses prefix unary operators and casts.
/// </summary>
/// <param name="parser"></param>
/// <returns></returns>
static CkExpression *s_ParseLevel2( CkParserInstance *parser )
{
	CkToken token;
	CkExpression *accumulator;
	CkExpressionKind kind;

	CkParserReadToken( parser, &token );

	// Associativity: RIGHT -> LEFT
	// Therefore, no loop is required, only a recursive call.
	switch ( token.kind ) {

	case CKTOK2( '+', '+' ):
	case CKTOK2( '-', '-' ):
	case '+':
	case '-':
	case '!':
	case '~':
	case '*':
	case '&':
		kind = token.kind == CKTOK2( '+', '+' ) ? CK_EXPRESSION_PREFIX_INC
			: token.kind == CKTOK2( '-', '-' ) ? CK_EXPRESSION_PREFIX_DEC
			: token.kind == '+' ? CK_EXPRESSION_UNARY_PLUS
			: token.kind == '-' ? CK_EXPRESSION_UNARY_MINUS
			: token.kind == '!' ? CK_EXPRESSION_LOGICAL_NOT
			: token.kind == '~' ? CK_EXPRESSION_BITWISE_NOT
			: token.kind == '*' ? CK_EXPRESSION_DEREFERENCE
			: CK_EXPRESSION_ADDRESS_OF;

		accumulator = CkExpressionCreateUnary( parser->arena, &token, kind, NULL, s_ParseLevel2( parser ) );
		break;

		// C-style casting
	case '(':
	{
		CkFoodType *t = CkParserType( parser );
		CkToken exprToken = token;
		// In case its a parenthesized expression
		if ( t == NULL ) {
			CkParserRewind( parser, 1 );
			accumulator = s_ParsePrimaryExpression( parser ); // Skipping level 1 because its parenthesized expr
			break;
		}
		CkParserReadToken( parser, &token );
		if ( token.kind != ')' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Missing closing bracket in C-style cast." );
		}
		accumulator = CkExpressionCreateBinary(
			parser->arena,
			&exprToken,
			CK_EXPRESSION_C_CAST,
			t,
			s_ParseLevel2( parser ),
			CkExpressionCreateType( parser->arena, t ) );
		break;
	}

	// No level-2 operator
	default:
		CkParserRewind( parser, 1 );
		accumulator = s_ParseLevel1( parser );
		break;
	}

	return accumulator;
}

/// <summary>
/// Parses binary operators. Uses Pratt parsing.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseBinary( uint8_t parentPrec, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *acc = s_ParseLevel2( parser );

	while ( TRUE ) {
		uint8_t prec;
		CkExpression *right;

		CkParserReadToken( parser, &token );
		prec = s_BinaryOpPrec( token.kind );
		if ( prec == 0 || prec <= parentPrec ) {
			CkParserRewind( parser, 1 );
			break;
		}

		right = s_ParseBinary( prec, parser );
		acc = CkExpressionCreateBinary( parser->arena, &token, s_BinaryOpKind( token.kind ), NULL, acc, right );
	}

	return acc;
}

/// <summary>
/// Parses a Food-style cast (expr => T)
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseFoodCast( CkParserInstance *parser )
{
	CkToken op;
	CkExpression *acc = s_ParseBinary( 0, parser );

	CkParserReadToken( parser, &op );
	while ( op.kind == CKTOK2( '=', '>' ) ) {
		CkFoodType *t = CkParserType( parser );
		if ( t == NULL ) {
			CkDiagnosticThrow( parser->pDhi, op.position, CK_DIAG_SEVERITY_ERROR, "",
				"Expected a type in Food-style cast." );
		}
		acc = CkExpressionCreateBinary( parser->arena, &op, CK_EXPRESSION_FOOD_CAST, NULL, acc, CkExpressionCreateType( parser->arena, t ) );
		CkParserReadToken( parser, &op );
	}
	CkParserRewind( parser, 1 );
	return acc;
}

/// <summary>
/// Parses a conditional expression.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseConditional( CkParserInstance *parser )
{
	CkToken token;
	CkToken op;
	CkExpression *left;
	CkExpression *right;
	CkExpression *extra = s_ParseFoodCast( parser );

	CkParserReadToken( parser, &token );
	op = token;

	// If it not a conditional expression
	if ( token.kind != '?' ) {
		CkParserRewind( parser, 1 );
		return extra;
	}

	left = s_ParseConditional( parser );
	CkParserReadToken( parser, &token );
	if ( token.kind != ':' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Expected colon in conditional expression." );
	}
	right = s_ParseConditional( parser );

	return CkExpressionCreateTernary( parser->arena, &op, CK_EXPRESSION_CONDITIONAL, NULL, left, right, extra );
}

/// <summary>
/// Parses an assignment expression.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseAssign( CkParserInstance *parser )
{
	CkToken op;
	CkExpressionKind kind;

	CkExpression *left = s_ParseConditional( parser );
	CkParserReadToken( parser, &op );
	switch ( op.kind ) {
	case '=':
	case CKTOK2( '+', '=' ):
	case CKTOK2( '-', '=' ):
	case CKTOK2( '*', '=' ):
	case CKTOK2( '/', '=' ):
	case CKTOK2( '%', '=' ):
	case CKTOK2( '&', '=' ):
	case CKTOK2( '|', '=' ):
	case CKTOK2( '^', '=' ):
	case CKTOK3( '<', '<', '=' ):
	case CKTOK3( '>', '>', '=' ):
		kind = op.kind == '=' ? CK_EXPRESSION_ASSIGN
			: op.kind == CKTOK2( '+', '=' ) ? CK_EXPRESSION_ASSIGN_SUM
			: op.kind == CKTOK2( '-', '=' ) ? CK_EXPRESSION_ASSIGN_DIFF
			: op.kind == CKTOK2( '*', '=' ) ? CK_EXPRESSION_ASSIGN_PRODUCT
			: op.kind == CKTOK2( '/', '=' ) ? CK_EXPRESSION_ASSIGN_QUOTIENT
			: op.kind == CKTOK2( '%', '=' ) ? CK_EXPRESSION_ASSIGN_REMAINDER
			: op.kind == CKTOK2( '&', '=' ) ? CK_EXPRESSION_ASSIGN_AND
			: op.kind == CKTOK2( '|', '=' ) ? CK_EXPRESSION_ASSIGN_OR
			: op.kind == CKTOK2( '^', '=' ) ? CK_EXPRESSION_ASSIGN_XOR
			: op.kind == CKTOK3( '<', '<', '=' ) ? CK_EXPRESSION_ASSIGN_LEFT_SHIFT
			: CK_EXPRESSION_ASSIGN_RIGHT_SHIFT;

		return CkExpressionCreateBinary( parser->arena, &op, kind, NULL, left, s_ParseConditional( parser ) );

		// Not an assignment
	default:
		CkParserRewind( parser, 1 );
		return left;
	}
}

/// <summary>
/// Parses a compound expression.
/// </summary>
/// <param name="parser">A pointer to the parser.</param>
/// <returns></returns>
static CkExpression *s_ParseCompound( CkParserInstance *parser )
{
	CkToken op;
	CkExpression *acc = s_ParseAssign( parser );

	CkParserReadToken( parser, &op );
	while ( op.kind == ',' ) {
		acc = CkExpressionCreateBinary( parser->arena, &op, CK_EXPRESSION_COMPOUND, NULL, acc, s_ParseAssign( parser ) );
		CkParserReadToken( parser, &op );
	}
	CkParserRewind( parser, 1 );
	return acc;
}

CkExpression *CkParserExpression( CkParserInstance *parser )
{
	CK_ARG_NON_NULL( parser );
	return s_ParseCompound( parser );
}
