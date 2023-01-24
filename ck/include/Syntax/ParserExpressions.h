/*
 * Declares the expression parser functions.
*/

#ifndef CK_PARSER_EXPRESSIONS_H_
#define CK_PARSER_EXPRESSIONS_H_

#include "Expression.h"
#include "Parser.h"

/// <summary>
/// Parses an expression in the parser source.
/// </summary>
/// <param name="parser">A pointer to the parser instance.</param>
/// <returns>A heap-allocated expression tree. The user is responsible for freeing the data with CkExpressionDelete().</returns>
CkExpression *CkParserExpression(CkParserInstance *parser);

#endif
