#include <Syntax/ParserStatements.h>
#include <Syntax/ParserExpressions.h>
#include <Syntax/Semantics.h>
#include <Syntax/ParserDecl.h>

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

static FFStatement *s_DoWhileStatement( FFScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;
	FFStatement *returned;

	// do >>>stmt<<< while ( condition ) ;
	FFStatement *cWhile = CkParseStmt( context, parser );

	// do stmt >>>while<<< ( condition ) ;
	CkParserReadToken( parser, &token );
	if ( token.kind != KW_WHILE ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The while keyword is expected, as a do ... while statement was started." );
		return NULL;
	}

	// do stmt while >>>(<<< condition ) ;
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A opening bracket ( is expected after the while keyword in the do ... while statement." );
		return NULL;
	}

	// do stmt while ( >>>condition<<< ) ;
	condition = CkParserExpression( parser );

	// do stmt while ( condition >>>)<<< ;
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A closing bracket ) is expected after the condition in the do ... while statement." );
		return NULL;
	}

	// do stmt while ( condition ) >>>;<<<
	CkParserReadToken( parser, &token );
	if ( token.kind != ';' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A semicolon is expected at the end of the do ... while statement." );
		return NULL;
	}

	returned = CkArenaAllocate( parser->genArena, sizeof(FFStatement) );
	returned->stmt = FF_STMT_DO_WHILE;
	returned->data.doWhile.condition = condition;
	returned->data.doWhile.cWhile = cWhile;

	return returned;
}

static FFStatement *s_ForStatement( FFScope *context, CkParserInstance *parser )
{
	CkToken token;
	FFStatement *returned;
	FFStatement *init;
	FFStatement *body;
	CkExpression *condition;
	CkExpression *lead;
	FFScope *forScope;

	// for >>>(<<< init ; condition ; lead ) block
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A opening bracket ( is expected after the for keyword in the for statement." );
		return NULL;
	}

	// for ( >>>init<<< ; condition ; lead ) block
	forScope = FFStartScope( parser->genArena, context, FALSE, FALSE );
	init = CkParseStmt( forScope, parser );

	// for ( init; >>>condition<<< ; lead ) block
	condition = CkParserExpression( parser );

	// for ( init ; condition >>>;<<< lead ) block
	CkParserReadToken( parser, &token );
	if ( token.kind != ';' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A semicolon ; is expected after the condition expression in the for statement." );
		return NULL;
	}

	// for ( init ; condition ; >>>lead<<< ) block
	lead = CkParserExpression( parser );

	// for ( init ; condition ; lead >>>)<<< block
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A closing bracket ) is expected after the lead expression in the for statement." );
		return NULL;
	}

	// for ( init ; condition ; lead ) >>>block<<<
	body = CkParseStmt( forScope, parser );

	returned = CkArenaAllocate( parser->genArena, sizeof(FFStatement) );
	returned->stmt = FF_STMT_FOR;
	returned->data.for_.cInit = init;
	returned->data.for_.condition = condition;
	returned->data.for_.lead = lead;
	returned->data.for_.body = body;

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
	case ';': {
		FFStatement* returned = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
		returned->stmt = FF_STMT_EMPTY;
		return returned;
	}

		// Block statement
	case '{': {
		FFStatement *block = CkArenaAllocate( parser->genArena, sizeof( FFStatement ) );
		block->stmt = FF_STMT_BLOCK;
		block->data.block.scope = FFStartScope( parser->genArena, context, TRUE, FALSE );
		block->data.block.stmts = CkListStart( parser->genArena, sizeof( FFStatement * ) );
		while ( TRUE ) {
			FFStatement *stmt;
			size_t index;

			// Checking for end of block
			CkParserReadToken( parser, &token );
			if ( token.kind == '}' )
				break;
			CkParserRewind( parser, 1 );

			// Declaration parsing
			index = parser->position;
			if ( CkParseDecl( parser->genArena, context, parser, FALSE, FALSE, TRUE, FALSE ) ) continue;
			else CkParserGoto( parser, index );

			// Statement parsing
			stmt = CkParseStmt( block->data.block.scope, parser );
			if ( !stmt ) return NULL;
			CkListAdd( block->data.block.stmts, &stmt );
		}
		return block;
	}

		// If statement
	case KW_IF: return s_IfStatement( context, parser );

		// While statement
	case KW_WHILE: return s_WhileStatement( context, parser );

		// Do ... while statement
	case KW_DO: return s_DoWhileStatement( context, parser );
	
		// For statement
	case KW_FOR: return s_ForStatement( context, parser );

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
