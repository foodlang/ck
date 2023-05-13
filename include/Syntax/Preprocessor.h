/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the preprocessor module, which takes in a list of
 * tokens (from the lexer) and outputs another list of tokens, with all macros
 * pasted.
 *
 ***************************************************************************/

#ifndef CK_PREPROCESSOR_H_
#define CK_PREPROCESSOR_H_

#include "../Memory/List.h"
#include <Diagnostics.h>

// A macro.
typedef struct CkMacro
{
	char  *name;     // The name of the macro (will match idents against)
	size_t argcount; // The amount of arguments required.
	CkList *expands; // The list of tokens to expand to.

} CkMacro;

// An if(-else) preprocessor branch.
typedef struct CkPreprocessorIf
{
	char   *condition; // If this macro is defined, expand
	bool  negative;  // If this is true, the condition is reversed
	CkList *expands;   // The code to expand

	struct CkPreprocessorIf *elseBranch; // The else branch
} CkPreprocessorIf;

// A preprocessor instance.
typedef struct CkPreprocessor
{
	CkList *input;  // The input list of tokens. (elem type = CkToken)
	CkList *macros; // The list of macros. (elem type = CkMacro)
	CkList *output; // The output list of tokens.
	bool  errors; // If true, some errors were encountered.
	CkDiagnosticHandlerInstance *pDhi; // Preprocessor DHI

} CkPreprocessor;

// Expands all found macros in the preprocessor. Result = number of expansions performed
size_t CkPreprocessorExpand( CkArenaFrame *allocator, CkPreprocessor *preprocessor );

// Prepares a new pass for the preprocessor (plugs its own output into its input.)
static inline void CkPreprocessorPrepareNextPass( CkPreprocessor *preprocessor )
{
	preprocessor->input = preprocessor->output;
}

static inline void CkDeclareCompileTimeMacro( CkArenaFrame *allocator, CkList *defines, char *name, CkToken *constant )
{
	CkMacro new = {};
	new.name = name;
	new.argcount = 0;
	new.expands = CkListStart( allocator, sizeof( CkToken ) );
	CkListAdd( new.expands, constant );
}

#endif
