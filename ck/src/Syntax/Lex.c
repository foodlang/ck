#include <include/Syntax/Lex.h>
#include <ckmem/CDebug.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

/// <summary>
/// Represents an entry in the keyword dictionary.
/// </summary>
typedef struct KeywordEntryPair
{
	/// <summary>
	/// The key, stored as a constant string.
	/// </summary>
	const char *key;

	/// <summary>
	/// The token kind for that specific keyword.
	/// </summary>
	uint64_t value;

} KeywordEntryPair;

static KeywordEntryPair s_keywordDict[] =
{
	{ "alignof", KW_ALIGNOF },
	{ "atomic", KW_ATOMIC },
	{ "break", KW_BREAK },
	{ "bool", KW_BOOL },
	{ "byte", KW_U8 },
	{ "case", KW_CASE },
	{ "char", KW_I8 },
	{ "class", KW_CLASS },
	{ "const", KW_CONST },
	{ "continue", KW_CONTINUE },
	{ "default", KW_DEFAULT },
	{ "do", KW_DO },
	{ "double", KW_F64 },
	{ "else", KW_ELSE },
	{ "end", KW_END },
	{ "enum", KW_ENUM },
	{ "extern", KW_EXTERN },
	{ "false", KW_FALSE },
	{ "for", KW_FOR },
	{ "function", KW_FUNCTION },
	{ "goto", KW_GOTO },
	{ "half", KW_F16 },
	{ "if", KW_IF },
	{ "int", KW_I32 },
	{ "lengthof", KW_LENGTHOF },
	{ "long", KW_I64 },
	{ "nameof", KW_NAMEOF },
	{ "new", KW_NEW },
	{ "null", KW_NULL },
	{ "public", KW_PUBLIC },
	{ "record", KW_RECORD },
	{ "restrict", KW_RESTRICT },
	{ "return", KW_RETURN },
	{ "sbyte", KW_I8 },
	{ "short", KW_I16 },
	{ "size", KW_SIZE },
	{ "sizeof", KW_SIZEOF },
	{ "start", KW_START },
	{ "static", KW_STATIC },
	{ "string", KW_STRING },
	{ "struct", KW_STRUCT },
	{ "switch", KW_SWITCH },
	{ "true", KW_TRUE },
	{ "uchar", KW_U8 },
	{ "union", KW_UNION },
	{ "uint", KW_U32 },
	{ "ulong", KW_U64 },
	{ "ushort", KW_U16 },
	{ "using", KW_USING },
	{ "void", KW_VOID },
	{ "volatile", KW_VOLATILE },
	{ "while", KW_WHILE },
	{ "i8", KW_I8 },
	{ "I8", KW_I8 },
	{ "u8", KW_U8 },
	{ "U8", KW_U8 },
	{ "i16", KW_I16 },
	{ "I16", KW_I16 },
	{ "u16", KW_U16 },
	{ "U16", KW_U16 },
	{ "f16", KW_F16 },
	{ "F16", KW_F16 },
	{ "i32", KW_I32 },
	{ "I32", KW_I32 },
	{ "u32", KW_U32 },
	{ "U32", KW_U32 },
	{ "f32", KW_F32 },
	{ "F32", KW_F32 },
	{ "i64", KW_I64 },
	{ "I64", KW_I64 },
	{ "u64", KW_U64 },
	{ "U64", KW_U64 },
	{ "f64", KW_F64 },
	{ "F64", KW_F64 },
	{ "module", KW_MODULE },
	{ "interface", KW_INTERFACE },
	{ "implements", KW_IMPLEMENTS },
	{ "assert", KW_ASSERT },
	{ "var", KW_VAR },
	{ "try", KW_TRY },
	{ "catch", KW_CATCH },
	{ "throw", KW_THROW },
	{ "typeof", KW_TYPEOF },
};

/// <summary>
/// Gets the next character in the source code.
/// </summary>
/// <param name="lexer"></param>
/// <returns></returns>
static inline char s_nextChar(CkLexInstance *lexer)
{
	if (lexer->cursor >= lexer->sourceLength)
		return 0;
	return lexer->source[lexer->cursor];
}

/// <summary>
/// Parses an escape sequence.
/// </summary>
/// <param name="cur"></param>
/// <returns></returns>
static inline uint8_t s_escapeSequence(CkLexInstance *lexer)
{
	char cur = s_nextChar(lexer);
	switch (cur) {
	case 'a':
	case 'A':
		return 0x07;
	case 'b':
	case 'B':
		return 0x08;
	case 'e':
	case 'E':
		return 0x1B;
	case 'f':
	case 'F':
		return 0x0C;
	case 'n':
	case 'N':
		return 0x0A;
	case 'r':
	case 'R':
		return 0x0D;
	case 't':
	case 'T':
		return 0x09;
	case 'v':
	case 'V':
		return 0x0B;
	case '\\':
		return 0x5C;
	case '\'':
		return 0x27;
	case '"':
		return 0x22;
	case 'x':
	case 'X':
	{
		uint8_t accumulator = 0;
		uint8_t elapsedChars = 1;
		lexer->cursor++;
		cur = s_nextChar(lexer);
		while ((isdigit(cur)
			|| ( cur <= 'F' && cur >= 'A' )
			|| ( cur <= 'f' && cur >= 'a' ))
			&& elapsedChars <= 2) {
			accumulator <<= 4;
			if (isdigit(cur))
				accumulator += (uint64_t)( cur - '0' );
			else if (cur <= 'F' && cur >= 'A')
				accumulator += (uint64_t)( cur - 'A' + 10 );
			else if (cur <= 'f' && cur >= 'a')
				accumulator += (uint64_t)( cur - 'a' + 10 );
			lexer->cursor++;
			cur = s_nextChar(lexer);
			elapsedChars++;
		}
		lexer->cursor--;
		return accumulator;
	}

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	{
		uint8_t accumulator = 0;
		uint8_t elapsedChars = 1;
		cur = s_nextChar(lexer);
		while ((cur <= '7' && cur >= '0')
			&& elapsedChars <= 3
			&& ((uint16_t)accumulator << 4) + 7 <= 255) {
			accumulator <<= 3;
			accumulator += ( cur - '0' );
			lexer->cursor++;
			cur = s_nextChar(lexer);
			elapsedChars++;
		}
		lexer->cursor--;
		return accumulator;
	}
	default:
		fprintf(stderr, "Unknown escape sequence \\%c\n", cur);
		return 0;
	}
}

void CkLexCreateInstance(CkArenaFrame *arena, CkLexInstance *dest, char *source)
{
	CK_ARG_NON_NULL(arena)
	CK_ARG_NON_NULL(dest)
	CK_ARG_NON_NULL(source)

	dest->cursor = 0;
	dest->sourceLength = strlen(source);
	dest->source = CkArenaAllocate(arena, dest->sourceLength + 1);
	dest->arena = arena;
	strcpy_s(dest->source, dest->sourceLength + 1, source);
}

void CkLexDestroyInstance(CkLexInstance *lexer)
{
	(void)lexer;
}

bool_t CkLexReadToken(CkLexInstance *lexer, CkToken *token)
{
	char cur;
	size_t base;

	CK_ARG_NON_NULL(lexer)
	CK_ARG_NON_NULL(token)

	cur = s_nextChar(lexer);

	while (isspace(cur)) {
		lexer->cursor++;
		cur = s_nextChar(lexer);
	}
	base = lexer->cursor;

	if (!cur) {
		// EOF
		token->position = lexer->cursor;
		token->kind = 0;
		return TRUE;
	}

	// Operators
	switch (cur) {

		// Single-character tokens
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	case ';':
	case ',':
	case '~':
	case '?':
		token->kind = (uint64_t)cur;
		token->position = lexer->cursor++;
		return TRUE;

		// Tokens that use $$, $= and $
	case '+':
	case '&':
	case '|':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar(lexer);
		if (next == cur) {
			token->kind = CKTOK2(cur, cur);
			lexer->cursor++;
		} else if (next == '=') {
			token->kind = CKTOK2(cur, '=');
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return TRUE;
	}

		// Tokens that use $$, $=, $> and $
	case '-':
	case '=':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar(lexer);
		if (next == cur) {
			token->kind = CKTOK2(cur, cur);
			lexer->cursor++;
		} else if (next == '=') {
			token->kind = CKTOK2(cur, '=');
			lexer->cursor++;
		} else if (next == '>') {
			token->kind = CKTOK2(cur, '>');
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return TRUE;
	}

		// Tokens that use $= and $
	case '*':
	case '/':
	case '%':
	case '^':
	case '!':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar(lexer);
		if (next == '=') {
			token->kind = CKTOK2(cur, '=');
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return TRUE;
	}

		// Tokens that use $$, $$=, $= and $
	case '<':
	case '>':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar(lexer);
		if (next == cur) {
			lexer->cursor++;
			if (s_nextChar(lexer) == '=') {
				token->kind = CKTOK3(cur, cur, '=');
				lexer->cursor++;
			} else token->kind = CKTOK2(cur, cur);
		} else if (next == '=') {
			token->kind = CKTOK2(cur, '=');
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return TRUE;
	}

		// Tokens that use $$ and $
	case ':':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar(lexer);
		if (next == cur) {
			token->kind = CKTOK2(cur, cur);
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return TRUE;
	}

		// Tokens that use $$, $$$ and $
	case '.':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar(lexer);
		if (next == cur) {
			lexer->cursor++;
			if (s_nextChar(lexer) == cur) {
				token->kind = CKTOK3(cur, cur, cur);
				lexer->cursor++;
			} else token->kind = CKTOK2(cur, cur);
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return TRUE;
	}

	// END OF SWITCH
	}

	// Number literals (ints and floats)
	if (isdigit(cur)) {
		uint64_t accumulator = 0;

		// Parse octal, binary and hexadecimal
		if (cur == '0') {
			lexer->cursor++;
			cur = s_nextChar(lexer);

			// Binary
			if (cur == 'b' || cur == 'B') {
				lexer->cursor++;
				cur = s_nextChar(lexer);
				while (cur == '0' || cur == '1') {
					accumulator <<= 1;
					accumulator += (uint64_t)( cur - '0' );
					lexer->cursor++;
					cur = s_nextChar(lexer);
				}
			// Hexadecimal
			} else if (cur == 'x' || cur == 'X') {
				lexer->cursor++;
				cur = s_nextChar(lexer);
				while (isdigit(cur)
				   || ( cur <= 'F' && cur >= 'A' )
				   || ( cur <= 'f' && cur >= 'a' )) {
					accumulator <<= 4;
					if (cur <= '9' && cur >= '0')
						accumulator += (uint64_t)( cur - '0' );
					else if (cur <= 'F' && cur >= 'A')
						accumulator += (uint64_t)( cur - 'A' + 10 );
					else if (cur <= 'f' && cur >= 'a')
						accumulator += (uint64_t)( cur - 'a' + 10 );
					lexer->cursor++;
					cur = s_nextChar(lexer);
				}
			// Octal
			} else {
				while (cur <= '7' && cur >= '0') {
					accumulator <<= 3;
					accumulator += (uint64_t)( cur - '0' );
					lexer->cursor++;
					cur = s_nextChar(lexer);
				}
			}
			token->kind = '0';
			token->position = base;
			token->value.u64 = accumulator;
			return TRUE;
		}

		// Decimal
		while (isdigit(cur)) {
			accumulator *= 10;
			accumulator += (uint64_t)( cur - '0' );
			lexer->cursor++;
			cur = s_nextChar(lexer);
		}
		// Float
		if (cur == '.') {
			long double scale = 0.1;
			long double accFloat = 0.0;
			accFloat = (long double)accumulator;
			lexer->cursor++;
			cur = s_nextChar(lexer);
			while (isdigit(cur)) {
				accFloat += (long double)( cur - '0' ) * scale;
				scale *= 0.1;
				lexer->cursor++;
				cur = s_nextChar(lexer);
			}
			if (cur == 'e' || cur == 'E') {
				long double exponent = 0.0;
				long double sign = 1;
				lexer->cursor++;
				cur = s_nextChar(lexer);
				if (cur == '-') {
					lexer->cursor++;
					cur = s_nextChar(lexer);
					sign = -1;
				}
				while (isdigit(cur)) {
					exponent *= 10;
					exponent += (long double)( cur - '0' );
					lexer->cursor++;
					cur = s_nextChar(lexer);
				}
				accFloat *= (long double)pow(10, exponent * sign);
			} else if (cur == 'p' || cur == 'P') {
				long double exponent = 0.0;
				long double sign = 1;
				lexer->cursor++;
				cur = s_nextChar(lexer);
				if (cur == '-') {
					lexer->cursor++;
					cur = s_nextChar(lexer);
					sign = -1;
				}
				while (isdigit(cur)) {
					exponent *= 10;
					exponent += (long double)( cur - '0' );
					lexer->cursor++;
					cur = s_nextChar(lexer);
				}
				accFloat *= (long double)pow(2, exponent * sign);
			}
			token->position = base;
			token->kind = 'F';
			token->value.f64 = accFloat;
			return TRUE;
		}
		// Int return
		token->position = base;
		token->kind = '0';
		token->value.u64 = accumulator;
		return TRUE;
	}

	// Identifiers and keywords
	if (isalpha(cur) || cur == '_') {
		size_t length = 0;
		char *strbuf;
		while (isalnum(cur) || cur == '_') {
			lexer->cursor++;
			cur = s_nextChar(lexer);
			length++;
		}

		strbuf = CkArenaAllocate(lexer->arena, length + 1);
		memcpy_s(strbuf, length + 1, lexer->source + base, length);
		strbuf[length] = 0;
		for (size_t i = 0; i < sizeof(s_keywordDict) / sizeof(KeywordEntryPair); i++) {
			if (!strcmp(strbuf, s_keywordDict[i].key)) {
				token->position = base;
				token->kind = s_keywordDict[i].value;
				return TRUE;
			}
		}
		token->position = base;
		token->kind = 'I';
		token->value.cstr = strbuf;
		return TRUE;
	}

	// Character literal
	if (cur == '\'') {
		uint64_t accumulator = 0;
		lexer->cursor++;
		cur = s_nextChar(lexer);
		while (cur != '\'' && cur != 0) {
			uint8_t value = (uint8_t)cur;
			accumulator <<= 8;
			if (value == '\\') {
				lexer->cursor++;
				cur = s_nextChar(lexer);
				if (cur == 0) {
					return FALSE;
				}
				value = s_escapeSequence(lexer);
			}
			accumulator += value;
			lexer->cursor++;
			cur = s_nextChar(lexer);
		}
		lexer->cursor++;
		token->position = base;
		token->kind = '0';
		token->value.u64 = accumulator;
		return TRUE;
	}

	if (cur == '\"') {
		size_t length = 0;
		size_t end = 0;
		lexer->cursor++;
		cur = s_nextChar(lexer);
		while (cur != '\"' && cur != 0) {
			if (cur == '\\') {
				lexer->cursor++;
				(void)s_escapeSequence(lexer);
			}
			lexer->cursor++;
			length++;
			cur = s_nextChar(lexer);
		}
		end = lexer->cursor + 1;
		token->value.cstr = CkArenaAllocate(lexer->arena, length + 1);
		lexer->cursor = base;
		for (size_t i = 0; i <= length; i++) {
			token->value.cstr[i] = s_nextChar(lexer);
			if (cur == '\\') {
				lexer->cursor++;
				token->value.cstr[i] = (char)s_escapeSequence(lexer);
			}
			lexer->cursor++;
		}
		lexer->cursor = end;
		token->position = base;
		token->kind = 'S';
		return TRUE;
	}

	token->position = base;
	token->kind = (uint64_t)cur;
	lexer->cursor++;
	return FALSE;
}
