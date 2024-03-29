/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the lexer module, which takes source code (as a string),
 * and outputs tokens. Tokens describe the text in a manner that is
 * understandable by the parser. Whitespaces and comments are removed here.
 *
 ***************************************************************************/

#ifndef CK_LEX_H_
#define CK_LEX_H_

#include <Types.h>
#include <FileIO.h>
#include <Memory/Arena.h>

// Stores the state of a lexer.
typedef struct CkLexInstance
{
	CkSource *source;    // The source buffer of the lexer.
	size_t cursor;       // The current position of the lexer in the source buffer.
	CkArenaFrame *arena; // The arena used for allocations.

} CkLexInstance;

typedef enum CkKeyword
{
	KW_ALIGNOF = 34647,
	KW_ATOMIC,
	KW_BREAK,
	KW_BOOL,
	KW_CASE,
	KW_CHAR,
	KW_CLASS,
	KW_CONST,
	KW_CONTINUE,
	KW_DEFAULT,
	KW_DO,
	KW_ELSE,
	KW_END,
	KW_ENUM,
	KW_EXTERN,
	KW_FALSE,
	KW_FOR,
	KW_FUNCTION,
	KW_GOTO,
	KW_IF,
	KW_LENGTHOF,
	KW_NAMEOF,
	KW_NEW,
	KW_NULL,
	KW_PUBLIC,
	KW_RECORD,
	KW_RESTRICT,
	KW_RETURN,
	KW_SIZE,
	KW_SIZEOF,
	KW_START,
	KW_STATIC,
	KW_STRING,
	KW_STRUCT,
	KW_SWITCH,
	KW_TRUE,
	KW_UNION,
	KW_USING,
	KW_VOID,
	KW_VOLATILE,
	KW_WHILE,
	KW_I8,
	KW_U8,
	KW_I16,
	KW_U16,
	KW_I32,
	KW_U32,
	KW_I64,
	KW_U64,
	KW_F16,
	KW_F32,
	KW_F64,
	KW_MODULE,
	KW_INTERFACE,
	KW_IMPLEMENTS,
	KW_ASSERT,
	KW_SPONGE,
	KW_NAMESPACE,
	KW_VAR,
	KW_TRY,
	KW_CATCH,
	KW_THROW,
	KW_TYPEOF,
	KW_ASM,
	KW_REF,

	PP_DIRECTIVE_UNKNOWN,   // Unknown preprocessor directive
	PP_DIRECTIVE_MALFORMED, // Malformed preprocessor directive
	PP_DIRECTIVE_DEFINE,    // Defines a new macro
	PP_DIRECTIVE_UNDEFINE,  // Removes a macro
	PP_DIRECTIVE_IFDEF,     // Pastes code if macro is defined
	PP_DIRECTIVE_IFNDEF,    // Pastes code if macro is not defined
	PP_DIRECTIVE_ELIFDEF,   // Pastes code if macro is defined (else if)
	PP_DIRECTIVE_ELIFNDEF,  // Pastes code if macro is not defined (else if)
	PP_DIRECTIVE_ELSE,      // If all other if conditions fail, paste code
	PP_DIRECTIVE_MESSAGE,   // Adds a message to the diagnostic output
	PP_DIRECTIVE_WARNING,   // Adds a warning to the diagnostic output
	PP_DIRECTIVE_ERROR,     // Adds an error to the diagnostic output
	PP_MACRO_WILDCARD,      // Used for macro parameters (value.u64 is argument index)

} CkKeyword, CkDirectiveKind;

#define CKTOK2(a, b)       (uint64_t)((a) << 8 | (b))
#define CKTOK3(a, b, c)    (uint64_t)((a) << 16 | (b) << 8 | (c))
#define CKTOK4(a, b, c, d) (uint64_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

// Creates a new lexer instance.
void CkLexCreateInstance(CkArenaFrame *arena, CkLexInstance *dest, CkSource *source);

// Reads a token from the lexer source code.
bool CkLexReadToken(CkLexInstance *lexer, CkToken *dest, bool allow_ppdirect );

// Destroys a lexer instance.
void CkLexDestroyInstance(CkLexInstance *lexer);

#endif
