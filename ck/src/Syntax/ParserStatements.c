#include <include/Syntax/ParserStatements.h>
#include <include/Syntax/ParserExpressions.h>
#include <include/Syntax/Semantics.h>
#include <ckmem/CDebug.h>

static void s_IfStatement( CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;

	// if >>>(<<< condition ) stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be wrapped inside brackets." );
		return;
	}

	// if ( >>>condition<<< ) stmt
	condition = CkParserExpression( parser );
	if ( !condition ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be an expression." );
		return;
	}
	condition = CkSemanticsProcessExpression( parser->pDhi, parser->arena, condition );
	if (
		condition->type->id != CK_FOOD_BOOL
		&& !CK_TYPE_CLASSED_INT( condition->type->id )
		&& !CK_TYPE_CLASSED_POINTER( condition->type->id ) ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be of pointer, boolean or integer type." );
		return;
	}

	// if ( condition >>>)<<< stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be wrapped inside brackets." );
		return;
	}

	// if ( condition ) >>>stmt<<<
	CkParseStmt( parser );
}

void CkParseStmt( CkParserInstance *parser )
{
	CkToken token;

	CK_ARG_NON_NULL( parser );

	CkParserReadToken( parser, &token );
	switch ( token.kind ) {

	case KW_IF:
		// If statement
		s_IfStatement( parser );
		return;

		// nop
	case ';':
		return;

		// Attempts to parse an expression
	default:
	{
		CkExpression *expr = CkParserExpression( parser );
		if ( !expr ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Unknown statement." );
			return;
		}
		expr = CkSemanticsProcessExpression( parser->pDhi, parser->arena, expr );
	}

	}
}
