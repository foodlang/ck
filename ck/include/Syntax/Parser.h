/*
 * Core parser functions are declared here.
*/

#ifndef CK_PARSER_H_
#define CK_PARSER_H_

#include "Expression.h"
#include "../Diagnostics.h"

/// <summary>
/// A parser instance stores data about an individual parser
/// used by the Ck compiler.
/// </summary>
typedef struct CkParserInstance
{
	/// <summary>
	/// A passed pointer that points to a linear
	/// buffer of tokens to be used by the parser.
	/// </summary>
	CkToken *pPassedTokens;

	/// <summary>
	/// The amount of tokens stored in the passed
	/// buffer.
	/// </summary>
	size_t   passedTokenCount;

	/// <summary>
	/// The current position of the parser in the
	/// passed token buffer.
	/// </summary>
	size_t   position;

	/// <summary>
	/// A pointer to the passed diagnostic handler.
	/// </summary>
	CkDiagnosticHandlerInstance *pDhi;

} CkParserInstance;

/// <summary>
/// Creates a new parser from a passed token buffer. It must remain allocated
/// for the lifetime of the pointer.
/// </summary>
/// <param name="dest">A pointer to where the parser instance should be written.</param>
/// <param name="pPassedTokens">The token buffer pointer to pass.</param>
/// <param name="passedCount">The amount of tokens passed.</param>
/// <param name="pDhi">The diagnostic handler to report errors to. This is passed, meaning the parser doesn't own it.</param>
void CkParserCreateInstance(
	CkParserInstance *dest,
	CkToken *pPassedTokens,
	size_t passedCount,
	CkDiagnosticHandlerInstance *pDhi);

/// <summary>
/// Deletes a parser instance.
/// </summary>
/// <param name="dest">A pointer to the instance to be deleted.</param>
void CkParserDelete(CkParserInstance *dest);

/// <summary>
/// Reads a token from the token stream provided when creating the parser instance.
/// Advances the token pointer, meaning you should never increase parser->position
/// manually, unlike the lexer.
/// </summary>
/// <param name="parser">A pointer to the parser instance.</param>
/// <param name="token">The read token will be copied to this pointer.</param>
void CkParserReadToken(CkParserInstance *parser, CkToken *token);

/// <summary>
/// Attempts to rewind a parser's token pointer. If the parser
/// cannot be rewinded by the given amount, FALSE is returned and
/// the token pointer is set to 0.
/// </summary>
/// <param name="parser">A pointer to the parser instance.</param>
/// <param name="elems">The amount of tokens to be rewinded.</param>
bool_t CkParserRewind(CkParserInstance *parser, size_t elems);

#endif
