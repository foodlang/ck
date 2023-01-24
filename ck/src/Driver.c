#include <include/Driver.h>
#include <include/Syntax/Lex.h>
#include <include/Syntax/Parser.h>
#include <malloc.h>
#include <stdio.h>

#define LEXBLOCK 256

void CkDriverCompile(
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig)
{
	CkToken *tokenBatch;       // An array storing the source tokens.
	size_t   tokenCount = 0;   // The amount of source tokens.
	size_t   tokenBatchLength; // The length of the array storing the source tokens.

	CkLexInstance lexer;     // The lexer.
	CkParserInstance parser; // The parser.

	// 1. Default output values
	result->successful = TRUE;

	// 2. Allocating memory for the token buffer
	tokenBatchLength = LEXBLOCK;
	tokenBatch = calloc(tokenBatchLength, sizeof(CkToken));

	// 3. Lexical Analysis
	CkLexCreateInstance(&lexer, startupConfig->source);
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
	CkParserCreateInstance(&parser, tokenBatch, tokenBatchLength);

	// Cleaning up the tokens. They are no longer required after parsing.
	for (size_t i = 0; i < tokenCount; i++)
		CkLexDeleteToken(tokenBatch + tokenCount);
	
	// Cleanup
	free(tokenBatch);
	CkLexDestroyInstance(&lexer);
	CkParserDelete(&parser);
}
