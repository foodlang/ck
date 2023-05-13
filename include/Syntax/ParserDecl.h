/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the declaration sub-parser. This sub-parser parses the
 * declarations of the program.
 *
 ***************************************************************************/

#ifndef CK_PARSER_DECL_H_
#define CK_PARSER_DECL_H_

#include <IL/FFStruct.h>
#include <Syntax/Parser.h>

// Parses a declaration.
bool CkParseDecl(
	CkArenaFrame *allocator    /* Allocator */,
	CkScope *context           /* The scope of the declaration. */,
	CkParserInstance *parser   /* The parser module instance. */,
	bool allowModule         /* Whether parsing modules is allowed. */,
	bool allowFuncStruct     /* Whether parsing functions and structures is allowed. */,
	bool allowNonConstAssign /* If this decl is a variable, whether it should allow non-constant assigns (false for args) */,
	bool allowExposureQual   /* Whether the public, static or extern specifiers should be allowed */,
	CkList *stmtList           /* A list of statements. Used to add statements if needed, elem type = CkStatement * */
);

#endif
