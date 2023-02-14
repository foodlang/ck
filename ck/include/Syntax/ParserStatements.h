/*
 * Parses one or many statements. Declarations don't count
 * as statements.
*/

#ifndef CK_PARSER_STATEMENTS_H_
#define CK_PARSER_STATEMENTS_H_

#include "Parser.h"
#include <fflib/FFStruct.h>

/// <summary>
/// Parses a statement, and outputs it to a TAC function declaration.
/// </summary>
/// <param name="parser">A pointer to the parser instance.</param>
/// <param name="function">A pointer to the function descriptor.</param>
FFStatement *CkParseStmt(FFScope *context, CkParserInstance *parser);

#endif
