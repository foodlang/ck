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
bool_t CkParseDecl(
	CkArenaFrame *allocator    /* Allocator */,
	CkScope *context           /* The scope of the declaration. */,
	CkParserInstance *parser   /* The parser module instance. */,
	bool_t allowModule         /* Whether parsing modules is allowed. */,
	bool_t allowFuncStruct     /* Whether parsing functions and structures is allowed. */,
	bool_t allowNonConstAssign /* If this decl is a variable, whether it should allow non-constant assigns (FALSE for args) */,
	bool_t allowExposureQual   /* Whether the public, static or extern specifiers should be allowed */
);

#endif
