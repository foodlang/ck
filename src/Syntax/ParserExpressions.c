#include <Syntax/ParserExpressions.h>
#include <Syntax/ParserTypes.h>

#include <FileIO.h>
#include <CDebug.h>

#include <stdio.h>
#include <string.h>

// An entry in the precedence table for binary operators.
typedef struct BinaryOperatorPrecedenceEntry
{
	// The operator.
	uint64_t op;

	// The precedence.
	uint8_t  prec;

	// The kind of the operator.
	CkExpressionKind kind;

} BinaryOperatorPrecedenceEntry;

// A table having operators as its first entries and
// precedences as its second.
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

// Looks up in the binary precedence table and attempts to get 
// the precedence of a given operator.
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

// Looks up the binary precedence table and attempts to get
// the expression kind of a given operator.
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

// Parses a primary value.
static CkExpression *s_ParsePrimaryExpression( CkScope* scope, CkParserInstance *parser )
{
	CkToken token;

	CkParserReadToken( parser, &token );

	switch ( token.kind ) {

		// Identifier
	case 'I':
	{
		CkExpression *scoped;
		CkToken op;

		// Scope resolution
		CkParserReadToken( parser, &op );
		scoped =
			op.kind == CKTOK2(':', ':')
			? CkExpressionCreateLiteral(parser->arena, &token, CK_EXPRESSION_SCOPED_REFERENCE, NULL)
			: CkExpressionCreateLiteral(
				parser->arena,
				&token,
				CK_EXPRESSION_IDENTIFIER,
				NULL // types are figured out later
			);
		CkParserRewind( parser, 1 );
		while ( TRUE ) {
			CkParserReadToken( parser, &op );
			if ( op.kind == CKTOK2( ':', ':' ) ) {
				scoped = CkExpressionCreateUnary( parser->arena, &token, CK_EXPRESSION_SCOPED_REFERENCE, NULL, scoped );
			} else {
				CkParserRewind( parser, 1 );
				break;
			}
		}
		return scoped;
	}

		// String literal
	case 'S':
		return CkExpressionCreateLiteral(
			parser->arena,
			&token,
			CK_EXPRESSION_STRING_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_STRING, 0, NULL )
		);

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
	case KW_TRUE: {
		CkToken trueToken = token;
		trueToken.value.u64 = 1;
		return CkExpressionCreateLiteral(
			parser->arena,
			&trueToken,
			CK_EXPRESSION_BOOL_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_BOOL, 0, NULL ) );
	}

		// False is a boolean constant
	case KW_FALSE: {
		CkToken falseToken = token;
		falseToken.value.u64 = 0;
		return CkExpressionCreateLiteral(
			parser->arena,
			&falseToken,
			CK_EXPRESSION_BOOL_LITERAL,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_BOOL, 0, NULL ) );
	}

		// Null is a pointer constant
	case KW_NULL: {
		CkToken nullToken = token;
		nullToken.value.u64 = 0;
		return CkExpressionCreateLiteral(
			parser->arena,
			&nullToken,
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
	}

		// Parenthesized expressions
	case '(':
	{
		CkExpression *expr = CkParserExpression( scope, parser );
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
		type = CkParserType( scope, parser );
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
		type = CkParserType( scope, parser );
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

// Parses a conditional expression.
static CkExpression *s_ParseConditional( CkScope* scope, CkParserInstance *parser );

// Parses access, postfix increment and decrement operators.
static CkExpression *s_ParseLevel1( CkScope* scope, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *acc;

	// The operand and next token are read.
	acc = s_ParsePrimaryExpression( scope, parser );
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
				s_ParsePrimaryExpression( scope, parser ) );
			break;

		case CKTOK2( '-', '>' ):
			acc = CkExpressionCreateBinary(
				parser->arena,
				&token,
				CK_EXPRESSION_POINTER_MEMBER_ACCESS,
				NULL,
				acc,
				s_ParsePrimaryExpression( scope, parser ) );
			break;

			// Array subscript
		case '[':
			acc = CkExpressionCreateBinary(
				parser->arena,
				&token,
				CK_EXPRESSION_SUBSCRIPT,
				NULL,
				acc,
				CkParserExpression( scope, parser ) );
			CkParserReadToken( parser, &token );
			if ( token.kind != ']' ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Missing closing bracket in array subscript operation." );
			}
			break;

			// Function call
		case '(':
			acc = CkExpressionCreateUnary(
				parser->arena,
				&token,
				CK_EXPRESSION_FUNCCALL,
				NULL,
				acc );
			acc->extended_extra = CkListStart( parser->arena, sizeof( CkExpression * ) );
			// Arguments
			while ( TRUE ) {
				CkExpression *param;
				CkParserReadToken( parser, &token );
				if ( token.kind == ')' ) break;
				else if ( token.kind == 0 ) {
					CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
						"Missing closing bracket in function call." );
					break;
				}
				CkParserRewind( parser, 1 );
				param = s_ParseConditional( scope, parser );
				CkListAdd( (CkList *)acc->extended_extra, &param );
				CkParserReadToken( parser, &token );
				if ( token.kind == ')' ) CkParserRewind( parser, 1 );
				else if ( token.kind == ',' ) continue;
				else {
					CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
						"Expecting a colon , or a closing bracket" );
				}
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

// Parses prefix unary operators and casts.
static CkExpression *s_ParseLevel2( CkScope *scope, CkParserInstance *parser )
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
	case CKTOK2('&', '&'): // Opaque referencing
		kind = token.kind == CKTOK2( '+', '+' ) ? CK_EXPRESSION_PREFIX_INC
			: token.kind == CKTOK2( '-', '-' ) ? CK_EXPRESSION_PREFIX_DEC
			: token.kind == '+' ? CK_EXPRESSION_UNARY_PLUS
			: token.kind == '-' ? CK_EXPRESSION_UNARY_MINUS
			: token.kind == '!' ? CK_EXPRESSION_LOGICAL_NOT
			: token.kind == '~' ? CK_EXPRESSION_BITWISE_NOT
			: token.kind == '*' ? CK_EXPRESSION_DEREFERENCE
			: token.kind == CKTOK2('&', '&') ? CK_EXPRESSION_OPAQUE_ADDRESS_OF
			: CK_EXPRESSION_ADDRESS_OF;

		accumulator = CkExpressionCreateUnary( parser->arena, &token, kind, NULL, s_ParseLevel2( scope, parser ) );
		break;

		// C-style casting
	case '(':
	{
		CkFoodType* t = NULL;
		CkToken exprToken = token;

		// Check if it's a parenthesized expression
		CkParserReadToken( parser, &token );
		if ( !(token.kind == 'I' && CkSymbolDeclared( scope, token.value.cstr )) ) {
			CkParserRewind( parser, 1 );
			t = CkParserType( scope, parser );
		}
		else CkParserRewind( parser, 1 );
		if ( !t ) {
			CkParserRewind( parser, 1 );
			accumulator = s_ParsePrimaryExpression( scope, parser ); // Skipping level 1 because it's parenthesized expression
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
			s_ParseLevel2( scope, parser ),
			CkExpressionCreateType( parser->arena, t ) );
		break;
	}

	// No level-2 operator
	default:
		CkParserRewind( parser, 1 );
		accumulator = s_ParseLevel1( scope, parser );
		break;
	}

	return accumulator;
}

// Parses binary operators. Uses Pratt parsing.
static CkExpression *s_ParseBinary( CkScope* scope, uint8_t parentPrec, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *acc = s_ParseLevel2( scope, parser );

	while ( TRUE ) {
		uint8_t prec;
		CkExpression *right;

		CkParserReadToken( parser, &token );
		prec = s_BinaryOpPrec( token.kind );
		if ( prec == 0 || prec <= parentPrec ) {
			CkParserRewind( parser, 1 );
			break;
		}

		right = s_ParseBinary( scope, prec, parser );
		acc = CkExpressionCreateBinary( parser->arena, &token, s_BinaryOpKind( token.kind ), NULL, acc, right );
	}

	return acc;
}

// Parses a Food-style cast (expr => T)
static CkExpression *s_ParseFoodCast( CkScope* scope, CkParserInstance *parser )
{
	CkToken op;
	CkExpression *acc = s_ParseBinary( scope, 0, parser );

	CkParserReadToken( parser, &op );
	while ( op.kind == CKTOK2( '=', '>' ) ) {
		CkFoodType *t = CkParserType( scope, parser );
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

// Parses a conditional expression.
static CkExpression *s_ParseConditional( CkScope* scope, CkParserInstance *parser )
{
	CkToken token;
	CkToken op;
	CkExpression *left;
	CkExpression *right;
	CkExpression *extra = s_ParseFoodCast( scope, parser );

	CkParserReadToken( parser, &token );
	op = token;

	// If it not a conditional expression
	if ( token.kind != '?' ) {
		CkParserRewind( parser, 1 );
		return extra;
	}

	left = s_ParseConditional( scope, parser );
	CkParserReadToken( parser, &token );
	if ( token.kind != ':' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Expected colon in conditional expression." );
	}
	right = s_ParseConditional( scope, parser );

	return CkExpressionCreateTernary( parser->arena, &op, CK_EXPRESSION_CONDITIONAL, NULL, left, right, extra );
}

// Parses an assignment expression.
static CkExpression *s_ParseAssign( CkScope* scope, CkParserInstance *parser )
{
	CkToken op;
	CkExpressionKind kind;

	CkExpression *left = s_ParseConditional( scope, parser );
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

		return CkExpressionCreateBinary(
			parser->arena,
			&op,
			kind,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_VOID, 0, NULL ),
			left,
			s_ParseConditional( scope, parser ) );

		// Not an assignment
	default:
		CkParserRewind( parser, 1 );
		return left;
	}
}

// Parses a compound expression.
static CkExpression *s_ParseCompound( CkScope* scope, CkParserInstance *parser )
{
	CkToken op;
	CkExpression *acc = s_ParseAssign( scope, parser );

	CkParserReadToken( parser, &op );
	while ( op.kind == ',' ) {
		acc = CkExpressionCreateBinary(
			parser->arena,
			&op,
			CK_EXPRESSION_COMPOUND,
			CkFoodCreateTypeInstance( parser->arena, CK_FOOD_VOID, 0, NULL ),
			acc,
			s_ParseAssign( scope, parser ) );
		CkParserReadToken( parser, &op );
	}
	CkParserRewind( parser, 1 );
	return acc;
}

CkExpression *CkParserExpression( CkScope* scope, CkParserInstance *parser )
{
	CK_ARG_NON_NULL( parser );
	return s_ParseCompound( scope, parser );
}
