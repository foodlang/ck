#include <include/Driver.h>
#include <include/Syntax/Lex.h>
#include <include/Syntax/Parser.h>
#include <include/Syntax/ParserExpressions.h>
#include <include/Syntax/Semantics.h>
#include <include/Diagnostics.h>
#include <include/CDebug.h>

#include <malloc.h>
#include <stdio.h>
#include <time.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void CkDriverCompile(
	CkArenaFrame *threadArena,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig)
{
	lua_State *luavm;

	CkArenaFrame tokenListArena; // The arena storing the list of the tokens.
	size_t   tokenCount = 0;     // The amount of source tokens.

	CkDiagnosticHandlerInstance dh; // The diagnostic handler.
	CkLexInstance lexer;            // The lexer.
	CkParserInstance parser;        // The parser.

	CK_ARG_NON_NULL(threadArena);
	CK_ARG_NON_NULL(result);
	CK_ARG_NON_NULL(startupConfig);

	luavm = luaL_newstate();

	// 1. Default output values
	result->successful = TRUE;

	// 2. Allocating memory for the token buffer
	CkArenaStartFrame(&tokenListArena, 0);

	// 3. Lexical Analysis
	CkLexCreateInstance(threadArena, &lexer, startupConfig->source);
	CkDiagnosticHandlerCreateInstance(threadArena, &dh, &lexer);
	while (TRUE) {
		register CkToken *current = CkArenaAllocate(&tokenListArena, sizeof(CkToken));
		if (!CkLexReadToken(&lexer, current)) {
			fprintf_s(stderr, "ck: Lexer failed to parse token (%c)\n", current->kind);
			result->successful = FALSE;
		}
		if (!current->kind)
			break;
		tokenCount++;
	}
	// Locking pages at MMU level to prevent write operations
	CkArenaWriteLock(&tokenListArena);

	// 4. Parsing
	CkParserCreateInstance(threadArena, &parser, (CkToken *)tokenListArena.base, tokenCount, &dh);
	CkExpressionPrint(CkSemanticsProcessExpression(&dh, threadArena, CkParserExpression(&parser)));

	// Displaying diagnostics
	CkDiagnosticDisplay(&dh);

	if (dh.anyErrors)
		result->successful = FALSE;
	else if (startupConfig->wError && dh.anyWarnings)
		result->successful = FALSE;

	printf_s("ck: Finished compiling %s\n", startupConfig->name);

	// Cleanup
	CkArenaEndFrame(&tokenListArena);
	CkLexDestroyInstance(&lexer);
	CkParserDelete(&parser);
	CkDiagnosticHandlerDestroyInstance(&dh);
	lua_close(luavm);
}
