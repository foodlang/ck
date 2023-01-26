#include <include/Driver.h>
#include <include/Syntax/Lex.h>
#include <include/Syntax/Parser.h>
#include <include/Syntax/ParserExpressions.h>
#include <include/Diagnostics.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>

#define LEXBLOCK 256

void CkDriverCompile(
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig)
{
	clock_t driverStart; // Stores the starting time of the driver.

	CkToken *tokenBatch;       // An array storing the source tokens.
	size_t   tokenCount = 0;   // The amount of source tokens.
	size_t   tokenBatchLength; // The length of the array storing the source tokens.

	CkDiagnosticHandlerInstance dh; // The diagnostic handler.
	CkLexInstance lexer;            // The lexer.
	CkParserInstance parser;        // The parser.

	driverStart = clock();

	// 1. Default output values
	result->successful = TRUE;

	// 2. Allocating memory for the token buffer
	tokenBatchLength = LEXBLOCK;
	tokenBatch = calloc(tokenBatchLength, sizeof(CkToken));

	// 3. Lexical Analysis
	CkLexCreateInstance(&lexer, startupConfig->source);
	CkDiagnosticHandlerCreateInstance(&dh, &lexer);
	while (TRUE) {
		register CkToken *current = tokenBatch + tokenCount;
		if (!CkLexReadToken(&lexer, current)) {
			fprintf_s(stderr, "ck: Lexer failed to parse token (%c)\n", current->kind);
			result->successful = FALSE;
		}
		if (!current->kind)
			break;
		tokenCount++;

		// Resizing token batch
		if (tokenBatchLength == tokenCount) {
			tokenBatchLength += LEXBLOCK;
			tokenBatch = realloc(tokenBatch, tokenBatchLength * sizeof(CkToken));
		}
	}

	// 4. Parsing
	CkParserCreateInstance(&parser, tokenBatch, tokenBatchLength, &dh);
	{
		CkExpression *expr = CkParserExpression(&parser);
		CkExpressionPrint(expr);
		CkExpressionDelete(expr);
	}

	// Cleaning up the tokens. They are no longer required after parsing.
	for (size_t i = 0; i < tokenCount; i++)
		CkLexDeleteToken(tokenBatch + tokenCount);

	result->executionTime = ( (double)( clock() - driverStart ) ) / CLOCKS_PER_SEC * 1000.0;

	// Displaying diagnostics
	CkDiagnosticDisplay(&dh);

	if (dh.anyErrors)
		result->successful = FALSE;
	else if (startupConfig->wError && dh.anyWarnings)
		result->successful = FALSE;

	printf_s("ck: Finished compiling %s, elapsed %lf ms\n", startupConfig->name, result->executionTime);
	
	// Cleanup
	free(tokenBatch);
	CkLexDestroyInstance(&lexer);
	CkParserDelete(&parser);
	CkDiagnosticHandlerDestroyInstance(&dh);
}
