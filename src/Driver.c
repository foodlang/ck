#include <Driver.h>
#include <Diagnostics.h>
#include <Syntax/Lex.h>
#include <Syntax/Parser.h>
#include <Syntax/ParserDecl.h>
#include <Syntax/ParserStatements.h>
#include <Syntax/Binder.h>
#include <Syntax/Preprocessor.h>
#include <Util/Time.h>

#include <CDebug.h>

#include <malloc.h>
#include <stdio.h>
#include <time.h>

void CkDriverCompile(
	CkDiagnosticHandlerInstance *pDhi,
	CkArenaFrame *threadArena,
	CkArenaFrame *genArena,
	CkLibrary *lib,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig
	)
{
	CkList *tokenList; // The list used for tokens.
	CkToken current;
	
	CkLexInstance lexer;          // The lexer.
	CkParserInstance parser;      // The parser.
	CkPreprocessor pp;            // The preprocessor.
	size_t expansion_counter = 1; // Counts the number of expansions by the preprocessor. 1 is a magic value.
	
	CK_ARG_NON_NULL( pDhi );
	CK_ARG_NON_NULL( threadArena );
	CK_ARG_NON_NULL( result );
	CK_ARG_NON_NULL( startupConfig );
	
	// 1. Default output values
	result->successful = true;
	
	// 2. Allocating memory for the token buffer
	tokenList = CkListStart( threadArena, sizeof( CkToken ) );
	
	// 3. Lexical Analysis
	CkLexCreateInstance( threadArena, &lexer, startupConfig->source );
	while ( true ) {
		if ( !CkLexReadToken( &lexer, &current, true ) ) {
			if ( current.kind == 'S' ) {
				CkDiagnosticThrow( pDhi, &current, CK_DIAG_SEVERITY_ERROR, "",
					"Newline is not allowed in string literal" );
			} else if ( current.kind == PP_DIRECTIVE_UNKNOWN ) {
				CkDiagnosticThrow( pDhi, &current, CK_DIAG_SEVERITY_ERROR, "",
					"Unknown preprocessor directive '#%s'", current.value.cstr );
			} else if (current.kind == PP_DIRECTIVE_MALFORMED ) {
				CkDiagnosticThrow( pDhi, &current, CK_DIAG_SEVERITY_ERROR, "",
					"Malformed preprocessor directive '#%s'", current.value.cstr );
			} else {
				CkDiagnosticThrow( pDhi, &current, CK_DIAG_SEVERITY_ERROR, "",
					 "Failed to parse token '%c' (%hhu)", (char)current.kind, current.kind );
			}
			result->successful = false;
		}
		if ( !current.kind )
			break;
		CkListAdd( tokenList, &current );
	}

	if ( !result->successful ) {
		CkDiagnosticThrow( pDhi, &current, CK_DIAG_SEVERITY_MESSAGE, "",
			"Preprocessing will not be performed if the tokenizer has failed in any capacity.", (char)current.kind );
		CkLexDestroyInstance( &lexer );
		return;
	}

	pp.input = tokenList;
	pp.macros = CkListStart( threadArena, sizeof( CkMacro ) );
	pp.pDhi = pDhi;
	pp.errors = false;

	// Default macros
	CkListAddRange( pp.macros, startupConfig->defines );

	while ( expansion_counter != 0 ) {
		expansion_counter = CkPreprocessorExpand( threadArena, &pp );
		CkPreprocessorPrepareNextPass( &pp );
		if ( pp.errors ) result->successful = false;
	}
	
	if ( !result->successful ) {
		CkDiagnosticThrow( pDhi, &current, CK_DIAG_SEVERITY_MESSAGE, "",
			"Parsing will not be performed if the preprocessor has failed in any capacity.", (char)current.kind );
		return;
	}
	
	// 4. Parsing
	CkParserCreateInstance(
		threadArena,
		genArena,
		&parser,
		pp.output,
		CkListLength( pp.output ),
		pDhi );
	// Parsing all declarations
	while ( parser.position < parser.passedTokenCount )
		if ( !CkParseDecl( genArena, lib->scope, &parser, true, true, false, true, NULL ) ) {
			result->successful = false;
			break;
		}
	
	// Cleanup
	CkLexDestroyInstance( &lexer );
	CkParserDelete( &parser );
}
