/*
 * A lexer capable of parsing Food input.
*/

#ifndef CK_LEX_H_
#define CK_LEX_H_

#include "../Types.h"
#include "../Arena.h"

/// <summary>
/// Stores the state of a lexer.
/// </summary>
typedef struct CkLexInstance
{
	/// <summary>
	/// The source buffer of the lexer.
	/// </summary>
	char *source;

	/// <summary>
	/// The length of the source buffer.
	/// </summary>
	size_t sourceLength;

	/// <summary>
	/// The current position of the lexer in the source buffer.
	/// </summary>
	size_t cursor;

	/// <summary>
	/// The arena used for allocations.
	/// </summary>
	CkArenaFrame *arena;

} CkLexInstance;

/// <summary>
/// Represents a token, an indivisible bit of text that is used to
/// represent the syntax of the source code.
/// </summary>
typedef struct CkToken
{

	/// <summary>
	/// Where the token is located.
	/// </summary>
	size_t position;

	/// <summary>
	/// The kind of the token.
	/// </summary>
	uint64_t kind;

	/// <summary>
	/// An additional value stored with the token.
	/// </summary>
	union {
		bool_t boolean;
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		float f32;
		double f64;
		char *cstr;
		void *ptr;
	} value;

} CkToken;

enum
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
};

#define CKTOK2(a, b)       (uint64_t)((a) << 8 | (b))
#define CKTOK3(a, b, c)    (uint64_t)((a) << 16 | (b) << 8 | (c))
#define CKTOK4(a, b, c, d) (uint64_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

/// <summary>
/// Creates a new lexer instance.
/// </summary>
/// <param name="dest">The destination lexer instance.</param>
/// <param name="source">The source buffer. It is copied before passing it to the lexer.</param>
void CkLexCreateInstance(CkArenaFrame *arena, CkLexInstance *dest, char *source);

/// <summary>
/// Reads a token from the lexer source code.
/// </summary>
/// <param name="lexer">The lexer to read the code from.</param>
/// <param name="dest">The destination token buffer.</param>
bool_t CkLexReadToken(CkLexInstance *lexer, CkToken *dest);

/// <summary>
/// Destroys a lexer instance.
/// </summary>
/// <param name="lexer">The lexer to destroy.</param>
void CkLexDestroyInstance(CkLexInstance *lexer);

#endif
