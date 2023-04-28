#include <Syntax/ParserStatements.h>
#include <Syntax/ParserExpressions.h>
#include <Syntax/ParserDecl.h>

#include <CDebug.h>

static inline CkExpression *s_ParseExpr( CkScope *scope, CkParserInstance *parser )
{
	CkExpression *base = CkParserExpression( scope, parser );
	return base;
}

static CkStatement *s_IfStatement( CkScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;
	CkStatement *returned;

	// if >>>(<<< condition ) stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be wrapped inside brackets." );
		return NULL;
	}

	// if ( >>>condition<<< ) stmt
	condition = s_ParseExpr( context, parser );
	if ( !condition ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be an expression." );
		return NULL;
	}

	// if ( condition >>>)<<< stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of an if statement must be wrapped inside brackets." );
		return NULL;
	}

	returned = CkArenaAllocate( parser->genArena, sizeof( CkStatement ) );
	returned->stmt = CK_STMT_IF;
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

static CkStatement *s_WhileStatement( CkScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;
	CkStatement *returned;

	// while >>>(<<< condition ) stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be wrapped inside brackets." );
		return NULL;
	}

	// while ( >>>condition<<< ) stmt
	condition = s_ParseExpr( context, parser );
	if ( !condition ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be an expression." );
		return NULL;
	}

	// while ( condition >>>)<<< stmt
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a while statement must be wrapped inside brackets." );
		return NULL;
	}

	returned = CkArenaAllocate( parser->genArena, sizeof( CkStatement ) );
	returned->stmt = CK_STMT_WHILE;
	returned->data.while_.condition = condition;

	// while ( condition ) >>>stmt<<<
	returned->data.while_.cWhile = CkParseStmt( context, parser );

	return returned;
}

static CkStatement *s_DoWhileStatement( CkScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkExpression *condition;
	CkStatement *returned;

	// do >>>stmt<<< while ( condition ) ;
	CkStatement *cWhile = CkParseStmt( context, parser );

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
	condition = CkParserExpression( context, parser );

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

	returned = CkArenaAllocate( parser->genArena, sizeof(CkStatement) );
	returned->stmt = CK_STMT_DO_WHILE;
	returned->data.doWhile.condition = condition;
	returned->data.doWhile.cWhile = cWhile;

	return returned;
}

static CkStatement *s_ForStatement( CkScope *context, CkParserInstance *parser )
{
	CkToken token;
	CkStatement *returned;
	CkStatement *init;
	CkStatement *body;
	CkExpression *condition;
	CkExpression *lead;
	CkScope *forScope;

	// for >>>(<<< init ; condition ; lead ) block
	CkParserReadToken( parser, &token );
	if ( token.kind != '(' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A opening bracket ( is expected after the for keyword in the for statement." );
		return NULL;
	}

	// for ( >>>init<<< ; condition ; lead ) block
	forScope = CkStartScope( parser->genArena, context, FALSE, FALSE );
	init = CkParseStmt( forScope, parser );

	// for ( init; >>>condition<<< ; lead ) block
	condition = CkParserExpression( context, parser );

	// for ( init ; condition >>>;<<< lead ) block
	CkParserReadToken( parser, &token );
	if ( token.kind != ';' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A semicolon ; is expected after the condition expression in the for statement." );
		return NULL;
	}

	// for ( init ; condition ; >>>lead<<< ) block
	lead = CkParserExpression( context, parser );

	// for ( init ; condition ; lead >>>)<<< block
	CkParserReadToken( parser, &token );
	if ( token.kind != ')' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A closing bracket ) is expected after the lead expression in the for statement." );
		return NULL;
	}

	// for ( init ; condition ; lead ) >>>block<<<
	body = CkParseStmt( forScope, parser );

	returned = CkArenaAllocate( parser->genArena, sizeof(CkStatement) );
	returned->stmt = CK_STMT_FOR;
	returned->data.for_.cInit = init;
	returned->data.for_.condition = condition;
	returned->data.for_.lead = lead;
	returned->data.for_.body = body;
	returned->data.for_.scope = forScope;

	return returned;
}

static CkStatement *s_ReturnStatement( CkScope *context, CkParserInstance *parser )
{
	CkToken token;            // Used for reading tokens
	CkExpression *returnexpr; // The returned expression (optional)
	CkStatement *yield;       // Destination statement

	yield = CkArenaAllocate( parser->genArena, sizeof( CkStatement ) );
	yield->stmt = CK_STMT_RETURN;

	// return >>;<<
	CkParserReadToken( parser, &token );
	if ( token.kind == ';' ) { // void return or return via yield
		
		return yield;
	}

	// return >>expr<< ;
	CkParserRewind( parser, 1 );
	returnexpr = CkParserExpression( context, parser );

	// return expr >>;<<
	CkParserReadToken( parser, &token );

	if ( token.kind != ';' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A semicolon ; is expected." );
		return NULL;
	}

	yield->data.return_ = returnexpr;
	return yield;
}

CkStatement *CkParseStmt( CkScope *context, CkParserInstance *parser )
{
	CkToken token;

	CK_ARG_NON_NULL( parser );
	CK_ARG_NON_NULL( context );

	CkParserReadToken( parser, &token );
	switch ( token.kind ) {

		// nop
	case ';': {
		CkStatement* returned = CkArenaAllocate( parser->genArena, sizeof( CkStatement ) );
		returned->stmt = CK_STMT_EMPTY;
		return returned;
	}

		// Block statement
	case '{': {
		CkStatement *block = CkArenaAllocate( parser->genArena, sizeof( CkStatement ) );
		block->stmt = CK_STMT_BLOCK;
		block->data.block.scope = CkStartScope(
			parser->genArena, context,
			TRUE,
			context == context->library->scope || context->module->scope == context ? TRUE : FALSE
		);
		block->data.block.stmts = CkListStart( parser->genArena, sizeof( CkStatement * ) );
		while ( TRUE ) {
			CkStatement *stmt;
			size_t index;

			// Checking for end of block
			CkParserReadToken( parser, &token );
			if ( token.kind == '}' )
				break;
			CkParserRewind( parser, 1 );

			// Declaration parsing
			index = parser->position;
			// TODO: Support for local funcs & typedefs
			if ( CkParseDecl(
				parser->genArena,
				block->data.block.scope,
				parser,
				FALSE,
				FALSE,
				TRUE,
				FALSE,
				block->data.block.stmts) ) continue;
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

		// Return statement
	case KW_RETURN: return s_ReturnStatement( context, parser );

		// Attempts to parse an expression/declaration
	default:
	{
		CkExpression *expr;
		CkStatement *returned;

		CkParserRewind( parser, 1 );
		expr = s_ParseExpr( context, parser );

		// Expressions
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
		returned = CkArenaAllocate( parser->genArena, sizeof( CkStatement ) );
		returned->stmt = CK_STMT_EXPRESSION;
		returned->data.expression = expr;
		return returned;
	}

	}
}
