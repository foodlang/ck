#include <Driver.h>
#include <Diagnostics.h>
#include <Syntax/Lex.h>
#include <Syntax/Parser.h>
#include <Syntax/ParserStatements.h>
#include <Util/Time.h>

#include <CDebug.h>

#include <malloc.h>
#include <stdio.h>
#include <time.h>

void CkDriverCompile(
	CkArenaFrame *threadArena,
	CkArenaFrame *genArena,
	CkModule *temp_dest,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig
)
{
	CkList *tokenList; // The list used for tokens.
	CkToken current;

	CkDiagnosticHandlerInstance dh; // The diagnostic handler.
	CkLexInstance lexer;            // The lexer.
	CkParserInstance parser;        // The parser.

	CkTimePoint driverStart; // The current time when performing the driver compilation.
	CkTimePoint driverEnd;   // The current time when ending the driver compilation.

	CkStatement *temp_funcstmt;

	CK_ARG_NON_NULL( threadArena );
	CK_ARG_NON_NULL( result );
	CK_ARG_NON_NULL( startupConfig );

	CkTimeGetCurrent( &driverStart );

	// 1. Default output values
	result->successful = TRUE;

	// 2. Allocating memory for the token buffer
	tokenList = CkListStart( threadArena, sizeof( CkToken ) );

	// 3. Lexical Analysis
	CkLexCreateInstance( threadArena, &lexer, startupConfig->source );
	CkDiagnosticHandlerCreateInstance( threadArena, &dh, &lexer );
	while ( TRUE ) {
		if ( !CkLexReadToken( &lexer, &current ) ) {
			fprintf( stderr, "ck: Lexer failed to parse token (%c)\n", (char)current.kind );
			result->successful = FALSE;
		}
		if ( !current.kind )
			break;
		CkListAdd( tokenList, &current );
	}

	// 4. Parsing
	CkParserCreateInstance( threadArena, genArena, &parser, tokenList, CkListLength( tokenList ), &dh );
	temp_funcstmt = CkParseStmt( temp_dest->scope, &parser );
	CkAllocateFunction(
		temp_dest->scope,
		TRUE,
		CkFoodCreateTypeInstance( genArena, CK_FOOD_FUNCPOINTER, 0, NULL ),
		"main",
		temp_funcstmt);

	// Displaying diagnostics
	CkDiagnosticDisplay( &dh );

	if ( dh.anyErrors )
		result->successful = FALSE;
	else if ( startupConfig->wError && dh.anyWarnings )
		result->successful = FALSE;

	// Cleanup
	CkLexDestroyInstance( &lexer );
	CkParserDelete( &parser );
	CkDiagnosticHandlerDestroyInstance( &dh );

	CkTimeGetCurrent( &driverEnd );
	printf(
		"ck: Finished compiling %s (%f ms)\n",
		startupConfig->name,
		(double)(CkTimeElapsed_mcs( &driverStart, &driverEnd )) / 1000.0
	);
}
