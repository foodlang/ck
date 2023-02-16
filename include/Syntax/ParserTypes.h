/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the type sub-parser. This sub-parser parses the types.
 * A type is how the code should interpret some data, or what kind of data
 * a function returns or requires. The main parser is located at
 * include/Parser.h
 *
 ***************************************************************************/

#ifndef CK_PARSER_TYPES_H_
#define CK_PARSER_TYPES_H_

#include "Parser.h"
#include "../Food.h"

// Attempts to parse a type. Returns NULL and rewinds
// if no type can be successfully parsed.
CkFoodType *CkParserType(CkParserInstance *parser);

#endif

