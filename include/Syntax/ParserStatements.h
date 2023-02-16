/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the statement sub-parser. This sub-parser parses the
 * statements, which are bits of code that are actively executed in the
 * program. The main parser is located at include/Parser.h
 *
 ***************************************************************************/

#ifndef CK_PARSER_STATEMENTS_H_
#define CK_PARSER_STATEMENTS_H_

#include "Parser.h"
#include <IL/FFStruct.h>

// Parses a statement, and outputs it to a Fast Food statement AST object.
FFStatement *CkParseStmt(FFScope *context, CkParserInstance *parser);

#endif
