/*
 * Parses types (such as int *) and converts them to FoodType instances.
*/

#ifndef CK_PARSER_TYPES_H_
#define CK_PARSER_TYPES_H_

#include "Parser.h"
#include "../Food.h"

/// <summary>
/// Attempts to parse a type. Returns NULL and rewinds
/// if no type can be successfully parsed.
/// </summary>
/// <param name="parser">A pointer to the parser instance.</param>
CkFoodType *CkParserType(CkParserInstance *parser);

#endif

