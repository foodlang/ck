#include <Syntax/ParserStatements.h>
#include <Syntax/ParserExpressions.h>
#include <Syntax/Semantics.h>

#include <CDebug.h>

static CkExpression *s_ParseExpr( CkParserInstance *parser )
{
	CkExpression *base = CkParserExpression( parser );
	CkExpression *processed = CkSemanticsProcessExpression(
		parser->pDhi,
		parser->genArena,
		base
	);
	return processed;
}

static FFStatement *s_IfStatement( FFScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;
	FFStatement *returned;

	// if >>>(<<< condition ) stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be wrapped inside brackets." );
		return NULL;
	}

	// if ( >>>condition<<< ) stmt
	condition = s_ParseExpr( parser );
	if ( !condition ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be an expression." );
		return NULL;
	}
	condition = CkSemanticsProcessExpression( parser->pDhi, parser->genArena, condition );
	if (
		condition->type->id != CK_FOOD_BOOL
		&& !CK_TYPE_CLASSED_INT( condition->type->id )
		&& !CK_TYPE_CLASSED_POINTER( condition->type->id ) ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be of pointer, boolean or integer type." );
		return NULL;
	}

	// if ( condition >>>)<<< stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be wrapped inside brackets." );
		return NULL;
	}

	returned = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
	returned->stmt = FF_STMT_IF;
	returned->data.if_.condition = condition;

	// if ( condition ) >>>stmt<<<
	returned->data.if_.cThen = CkParseStmt( context, parser );

	CkParserReadToken( parser, &token );
	// if ( condition ) stmt >>>else<<< stmt
	if ( token.kind == KW_ELSE ) {
		// if (condition ) stmt else >>>stmt<<<
		returned->data.if_.cElse = CkParseStmt( context, parser );
	} else CkParserRewind( parser, 1 );

	return returned;
}

static FFStatement *s_WhileStatement( FFScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;
	FFStatement *returned;

	// while >>>(<<< condition ) stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be wrapped inside brackets." );
		return NULL;
	}

	// while ( >>>condition<<< ) stmt
	condition = s_ParseExpr( parser );
	if ( !condition ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be an expression." );
		return NULL;
	}
	condition = CkSemanticsProcessExpression( parser->pDhi, parser->genArena, condition );
	if (
		condition->type->id != CK_FOOD_BOOL
		&& !CK_TYPE_CLASSED_INT( condition->type->id )
		&& !CK_TYPE_CLASSED_POINTER( condition->type->id ) ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be of pointer, boolean or integer type." );
		return NULL;
	}

	// while ( condition >>>)<<< stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be wrapped inside brackets." );
		return NULL;
	}

	returned = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
	returned->stmt = FF_STMT_WHILE;
	returned->data.while_.condition = condition;

	// while ( condition ) >>>stmt<<<
	returned->data.while_.cWhile = CkParseStmt( context, parser );

	return returned;
}

FFStatement *CkParseStmt( FFScope *context, CkParserInstance *parser )
{
	CkToken token;

	CK_ARG_NON_NULL( parser );
	CK_ARG_NON_NULL( context );

	CkParserReadToken( parser, &token );
	switch ( token.kind ) {

		// nop
	case ';':
	{
		FFStatement *returned = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
		returned->stmt = FF_STMT_EMPTY;
		return returned;
	}

		// If statement
	case KW_IF:
		return s_IfStatement( context, parser );

		// While statement
	case KW_WHILE:
		return s_WhileStatement( context, parser );

		// Block statement
	case '{':
	{
		FFStatement *block = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
		block->stmt = FF_STMT_BLOCK;
		block->data.block.scope = FFStartScope( parser->genArena, context, TRUE, FALSE );
		block->data.block.stmts = CkListStart( parser->genArena, sizeof( FFStatement * ) );
		while ( TRUE ) {
			FFStatement *stmt;
			CkParserReadToken( parser, &token );
			if ( token.kind == '}' )
				break;

			CkParserRewind( parser, 1 );
			stmt = CkParseStmt( block->data.block.scope, parser );
			if ( !stmt ) return NULL;
			CkListAdd( block->data.block.stmts, &stmt );
		}
		return block;
	}

		// Attempts to parse an expression
	default:
	{
		CkExpression *expr;
		FFStatement *returned;

		CkParserRewind( parser, 1 );
		expr = s_ParseExpr( parser );

		// TODO: Declarations
		if ( !expr ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Unknown statement." );
			return NULL;
		}
		CkParserReadToken( parser, &token );
		if ( token.kind != ';' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Expected semicolon at end of statement." );
			return NULL;
		}
		returned = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
		returned->stmt = FF_STMT_EXPRESSION;
		returned->data.expression = expr;
		return returned;
	}

	}
}
