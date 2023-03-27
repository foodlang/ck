#include <Driver.h>
#include <Diagnostics.h>
#include <Syntax/Lex.h>
#include <Syntax/Parser.h>
#include <Syntax/ParserStatements.h>
#include <Syntax/Binder.h>
#include <Util/Time.h>

#include <CDebug.h>

#include <malloc.h>
#include <stdio.h>
#include <time.h>

void CkDriverCompile(
	CkDiagnosticHandlerInstance *pDhi,
	CkArenaFrame *threadArena,
	CkArenaFrame *genArena,
	CkModule *temp_dest,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig
)
{
	CkList *tokenList; // The list used for tokens.
	CkToken current;

	CkLexInstance lexer;            // The lexer.
	CkParserInstance parser;        // The parser.

	CkStatement *temp_funcstmt;

	CK_ARG_NON_NULL( pDhi );
	CK_ARG_NON_NULL( threadArena );
	CK_ARG_NON_NULL( result );
	CK_ARG_NON_NULL( startupConfig );

	// 1. Default output values
	result->successful = TRUE;

	// 2. Allocating memory for the token buffer
	tokenList = CkListStart( threadArena, sizeof( CkToken ) );

	// 3. Lexical Analysis
	CkLexCreateInstance( threadArena, &lexer, startupConfig->source );
	CkDiagnosticHandlerCreateInstance( threadArena, pDhi, &lexer );
	while ( TRUE ) {
		if ( !CkLexReadToken( &lexer, &current ) ) {
			CkDiagnosticThrow( pDhi, current.position, CK_DIAG_SEVERITY_ERROR, "",
				"Failed to parse token (%c)\n", (char)current.kind );
			result->successful = FALSE;
		}
		if ( !current.kind )
			break;
		CkListAdd( tokenList, &current );
	}

	// 4. Parsing
	CkParserCreateInstance( threadArena, genArena, &parser, tokenList, CkListLength( tokenList ), pDhi );
	temp_funcstmt = CkParseStmt( temp_dest->scope, &parser );
	CkDiagnosticDisplay( pDhi ); // temp
	CkAllocateFunction(
		temp_dest->scope,
		TRUE,
		CkFoodCreateTypeInstance( genArena, CK_FOOD_FUNCPOINTER, 0, NULL ),
		"main",
		temp_funcstmt);

	// Cleanup
	CkLexDestroyInstance( &lexer );
	CkParserDelete( &parser );
}
