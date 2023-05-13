/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the parser module. The parser processes a list of
 * token into an IL abstract syntax tree, which can then be used to generate
 * native code. This header does not define the sub-parsers (expression
 * parser, statement parser, type parser) and only the module that makes them
 * work.
 *
 ***************************************************************************/

#ifndef CK_PARSER_H_
#define CK_PARSER_H_

#include "Expression.h"
#include "../Diagnostics.h"
#include <Memory/Arena.h>

// A parser instance stores data about an individual parser
// used by the Ck compiler.
typedef struct CkParserInstance
{
	// A passed pointer that points to a linked list
	// of tokens.
	CkList *pPassedTokens;

	// The amount of tokens stored in the passed
	// buffer.
	size_t   passedTokenCount;

	// The current position of the parser in the
	// passed token buffer.
	size_t   position;

	// A pointer to the passed diagnostic handler.
	CkDiagnosticHandlerInstance *pDhi;

	// The arena used for allocations.
	CkArenaFrame *arena;

	// The arena that will be used for generated code.
	CkArenaFrame *genArena;

} CkParserInstance;

// Creates a new parser from a passed token buffer. It must remain allocated
// for the lifetime of the pointer.
void CkParserCreateInstance(
	CkArenaFrame *arena,
	CkArenaFrame *genArena,
	CkParserInstance *dest,
	CkList *pPassedTokens,
	size_t passedCount,
	CkDiagnosticHandlerInstance *pDhi );

// Deletes a parser instance.
void CkParserDelete( CkParserInstance *dest );

// Reads a token from the token stream provided when creating the parser instance.
// Advances the token pointer, meaning you should never increase parser->position
// manually, unlike the lexer.
void CkParserReadToken( CkParserInstance *parser, CkToken *token );

// Attempts to rewind a parser's token pointer. If the parser
// cannot be rewinded by the given amount, false is returned and
// the token pointer is set to 0.
bool CkParserRewind( CkParserInstance *parser, size_t elems );

// Goes to a specific token. Used to rewind the parser.
void CkParserGoto( CkParserInstance *parser, size_t index );

#endif
