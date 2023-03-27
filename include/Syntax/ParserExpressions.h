/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the expression sub-parser. This sub-parser parses the
 * expressions that are used in statements and declarations. The main parser
 * is located at include/Parser.h
 *
 ***************************************************************************/

#ifndef CK_PARSER_EXPRESSIONS_H_
#define CK_PARSER_EXPRESSIONS_H_

#include "Expression.h"
#include "Parser.h"
#include "IL/FFStruct.h"

// Parses an expression in the parser source.
CkExpression *CkParserExpression( CkScope *scope, CkParserInstance *parser );

#endif
