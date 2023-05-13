#include <Syntax/Lex.h>
#include <Memory/List.h>
#include <Syntax/Preprocessor.h>
#include <CDebug.h>

#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>

// Represents an entry in the keyword dictionary.
typedef struct KeywordEntryPair
{
	// The key, stored as a constant string.
	const char *key;

	// The token kind for that specific keyword.
	uint64_t value;

} KeywordEntryPair;

typedef struct MacroDirectiveEntryPair
{
	const char *key;
	uint64_t value;
} MacroDirectiveEntryPair;

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
	{ "asm", KW_ASM },
	{ "ref", KW_REF },
};

static MacroDirectiveEntryPair s_macroDict[] =
{
	{ "define", PP_DIRECTIVE_DEFINE },
	{ "undef", PP_DIRECTIVE_UNDEFINE },
	{ "ifdef", PP_DIRECTIVE_IFDEF },
	{ "ifndef", PP_DIRECTIVE_IFNDEF },
	{ "elifdef", PP_DIRECTIVE_ELIFDEF },
	{ "elifndef", PP_DIRECTIVE_ELIFNDEF },
	{ "else", PP_DIRECTIVE_ELSE },
	{ "message", PP_DIRECTIVE_MESSAGE },
	{ "msg", PP_DIRECTIVE_MESSAGE },
	{ "warning", PP_DIRECTIVE_WARNING },
	{ "warn", PP_DIRECTIVE_WARNING },
	{ "error", PP_DIRECTIVE_ERROR },
	{ "err", PP_DIRECTIVE_ERROR },
};

// Gets the next character in the source code.
static inline char s_nextChar( CkLexInstance *lexer )
{
	if ( lexer->cursor >= lexer->source->len )
		return 0;
	return lexer->source->code[lexer->cursor];
}

// Parses an escape sequence.
static inline uint8_t s_escapeSequence( CkLexInstance *lexer )
{
	char cur = s_nextChar( lexer );
	switch ( cur ) {
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
		cur = s_nextChar( lexer );
		while ( (isdigit( cur )
			|| (cur <= 'F' && cur >= 'A')
			|| (cur <= 'f' && cur >= 'a'))
			&& elapsedChars <= 2 ) {
			accumulator <<= 4;
			if ( isdigit( cur ) )
				accumulator += (uint64_t)(cur - '0');
			else if ( cur <= 'F' && cur >= 'A' )
				accumulator += (uint64_t)(cur - 'A' + 10);
			else if ( cur <= 'f' && cur >= 'a' )
				accumulator += (uint64_t)(cur - 'a' + 10);
			lexer->cursor++;
			cur = s_nextChar( lexer );
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
		cur = s_nextChar( lexer );
		while ( (cur <= '7' && cur >= '0')
			&& elapsedChars <= 3
			&& ((uint16_t)accumulator << 4) + 7 <= 255 ) {
			accumulator <<= 3;
			accumulator += (cur - '0');
			lexer->cursor++;
			cur = s_nextChar( lexer );
			elapsedChars++;
		}
		lexer->cursor--;
		return accumulator;
	}
	default:
		fprintf( stderr, "Unknown escape sequence \\%c\n", cur );
		return 0;
	}
}

// Skips spaces.
static inline void _SkipSpaces( CkLexInstance *lexer )
{
	char c = s_nextChar( lexer );
	while ( isspace( c ) ) {
		lexer->cursor++;
		c = s_nextChar( lexer );
	}
}

// Parses a preprocessor directive.
static bool _ParsePreprocessorDirective(
	CkArenaFrame *arena,
	CkLexInstance *lexer,
	CkToken *token )
{
	char c;
	char *direct = token->value.cstr;
	CkToken ctoken;

	// ---- Skipping spaces ----
	_SkipSpaces( lexer );

	c = s_nextChar( lexer );
	if ( c == 0 ) {
		token->kind = PP_DIRECTIVE_MALFORMED;
		return false;
	}

	// ---- Parsing define ----
	if ( token->kind == PP_DIRECTIVE_DEFINE ) {
		size_t   pos_backup;
		CkList  *expands;
		CkList  *binder_params;
		size_t   binder_params_count = 0;
		CkMacro *dest;
		char    *name;

		CkLexReadToken( lexer, &ctoken, false ); // Yes, actually calling the lexer inside itself.
		if (ctoken.kind != 'I' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}
		name = ctoken.value.cstr;

		expands = CkListStart( arena, sizeof( CkToken ) );

		pos_backup = lexer->cursor;
		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind == '(' ) { // Parsing arguments
			binder_params = CkListStart( arena, sizeof( char * ) );
			while ( true ) {
				CkLexReadToken( lexer, &ctoken, false );
				if ( ctoken.kind == 'I' ) {
					CkListAdd( binder_params, &ctoken.value.cstr );
					binder_params_count++;
				} else if ( ctoken.kind == ')' ) break;
				else {
					token->kind = PP_DIRECTIVE_MALFORMED;
					token->value.cstr = direct;
					return false;
				}
				pos_backup = lexer->cursor;
				CkLexReadToken( lexer, &ctoken, false );
				if ( ctoken.kind == ',' );
				else if ( ctoken.kind == ')' ) lexer->cursor = pos_backup;
				else {
					token->kind = PP_DIRECTIVE_MALFORMED;
					token->value.cstr = direct;
					return false;
				}
			}
		} else lexer->cursor = pos_backup;

		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind != '$' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}

		// Parsing body, until $
		while ( true ) {
			CkLexReadToken( lexer, &ctoken, false );
			if ( ctoken.kind == 'I' ) {
				for ( size_t i = 0; i < binder_params_count; i++ ) {
					char *name = *(char **)CkListAccess( binder_params, i );
					if ( !strcmp( ctoken.value.cstr, name ) ) {
						CkToken wildcard = ctoken;
						wildcard.value.u64 = i;
						wildcard.kind = PP_MACRO_WILDCARD;
						CkListAdd( expands, &wildcard );
						goto LBreakOutNoAdd;
					}
				}
			} else if ( ctoken.kind == '$' )
				break;
			else if ( ctoken.kind == 0 ) {
				token->kind = PP_DIRECTIVE_MALFORMED;
				token->value.cstr = direct;
				return false;
			}
			CkListAdd( expands, &ctoken );
		LBreakOutNoAdd:;
		}

		token->value.ptr = CkArenaAllocate( arena, sizeof( CkMacro ) );
		dest = (CkMacro *)token->value.ptr;
		dest->argcount = binder_params_count;
		dest->name = name;
		dest->expands = expands;
		return true;
	}
	// ----- Parsing undef -----
	else if ( token->kind == PP_DIRECTIVE_UNDEFINE) {
		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind != 'I' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}
		token->value.cstr = ctoken.value.cstr;
		return true;
 	}
	// ----- Parsing ifdef and ifndef -----
	else if ( token->kind == PP_DIRECTIVE_IFDEF
		   || token->kind == PP_DIRECTIVE_IFNDEF
		   || token->kind == PP_DIRECTIVE_ELIFDEF
		   || token->kind == PP_DIRECTIVE_ELIFNDEF ) {
		size_t            pos_backup;
		CkList           *expands;
		CkPreprocessorIf *ppif;

		token->value.ptr = CkArenaAllocate( arena, sizeof( CkPreprocessorIf ) );
		ppif = (CkPreprocessorIf *)token->value.ptr;
		ppif->negative =
			token->kind == PP_DIRECTIVE_IFNDEF || token->kind == PP_DIRECTIVE_ELIFNDEF;
		expands = CkListStart( arena, sizeof( CkToken ) );
		ppif->expands = expands;

		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind != 'I' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}
		ppif->condition = ctoken.value.cstr;
		
		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind != '$' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}

		while ( true ) {
			CkLexReadToken( lexer, &ctoken, true );
			if ( ctoken.kind == '$' ) break;
			else if ( ctoken.kind == 0 ) {
				token->kind = PP_DIRECTIVE_MALFORMED;
				token->value.cstr = direct;
				return false;
			}
			CkListAdd( expands, &ctoken );
		}

		pos_backup = lexer->cursor;
		CkLexReadToken( lexer, &ctoken, true );
		if ( ctoken.kind == PP_DIRECTIVE_ELIFDEF
			|| ctoken.kind == PP_DIRECTIVE_ELIFNDEF
			|| ctoken.kind == PP_DIRECTIVE_ELSE )
			ppif->elseBranch = (CkPreprocessorIf *)ctoken.value.ptr;
		else lexer->cursor = pos_backup;
		return true;
	}
	else if ( token->kind == PP_DIRECTIVE_ELSE ) {
		CkList           *expands;
		CkPreprocessorIf *ppif;

		token->value.ptr = CkArenaAllocate( arena, sizeof( CkPreprocessorIf ) );
		ppif = (CkPreprocessorIf *)token->value.ptr;
		expands = CkListStart( arena, sizeof( CkToken ) );
		ppif->expands = expands;

		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind != '$' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}

		while ( true ) {
			CkLexReadToken( lexer, &ctoken, true );
			if ( ctoken.kind == '$' ) break;
			else if ( ctoken.kind == 0 ) {
				token->kind = PP_DIRECTIVE_MALFORMED;
				token->value.cstr = direct;
				return false;
			}
			CkListAdd( expands, &ctoken );
		}
		return true;
	} else if ( token->kind == PP_DIRECTIVE_MESSAGE
		   || token->kind == PP_DIRECTIVE_WARNING
		   || token->kind == PP_DIRECTIVE_ERROR ) {
		CkLexReadToken( lexer, &ctoken, false );
		if ( ctoken.kind != 'S' ) {
			token->kind = PP_DIRECTIVE_MALFORMED;
			token->value.cstr = direct;
			return false;
		}
		token->value.cstr = ctoken.value.cstr;
		return true;
	}

	return false;
}

void CkLexCreateInstance( CkArenaFrame *arena, CkLexInstance *dest, CkSource *source )
{
	CK_ARG_NON_NULL( arena );
	CK_ARG_NON_NULL( dest );
	CK_ARG_NON_NULL( source );

	dest->cursor = 0;
	dest->source = source;
	dest->arena = arena;
}

void CkLexDestroyInstance( CkLexInstance *lexer )
{
	(void)lexer;
}

bool CkLexReadToken( CkLexInstance *lexer, CkToken *token, bool allow_ppdirect )
{
	char cur;
	size_t base;

	CK_ARG_NON_NULL( lexer );
	CK_ARG_NON_NULL( token );

	token->source = lexer->source;

	_SkipSpaces( lexer );
	cur = s_nextChar( lexer );
	base = lexer->cursor;

	if ( !cur ) {
		// EOF
		token->position = lexer->cursor;
		token->kind = 0;
		return true;
	}

	// Operators
	switch ( cur ) {

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
	case '$':
		token->kind = (uint64_t)cur;
		token->position = lexer->cursor++;
		return true;

		// Tokens that use $$, $= and $
	case '+':
	case '&':
	case '|':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == cur ) {
			token->kind = CKTOK2( cur, cur );
			lexer->cursor++;
		} else if ( next == '=' ) {
			token->kind = CKTOK2( cur, '=' );
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// Tokens that use $$, $=, $> and $
	case '-':
	case '=':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == cur ) {
			token->kind = CKTOK2( cur, cur );
			lexer->cursor++;
		} else if ( next == '=' ) {
			token->kind = CKTOK2( cur, '=' );
			lexer->cursor++;
		} else if ( next == '>' ) {
			token->kind = CKTOK2( cur, '>' );
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// Tokens that use $= and $
	case '*':
	case '%':
	case '^':
	case '!':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == '=' ) {
			token->kind = CKTOK2( cur, '=' );
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// Uses $=, $ and comments
	case '/':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == '=' ) { // assign quotient
			token->kind = CKTOK2( cur, '=' );
			lexer->cursor++;
		} else if ( next == '/' ) { // C++ comment
			while ( next != '\n' ) {
				lexer->cursor++;
				next = s_nextChar( lexer );
			}
			return CkLexReadToken( lexer, token, true );
		} else if ( next == '*' ) { // C comment
			while ( true ) {
				lexer->cursor++;
				next = s_nextChar( lexer );

				if ( next == '*' ) {
					lexer->cursor++;
					next = s_nextChar( lexer );
					if ( next == '/' ) {
						lexer->cursor++;
						return CkLexReadToken( lexer, token, true );
					} else {
						lexer->cursor--;
						continue;
					}
				} else if ( next == 0 ) { // EOF not allowed
					token->position = base;
					token->kind = 0;
					token->value.u64 = 0;
					return false;
				}
			}
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// Tokens that use $$, $$=, $= and $
	case '<':
	case '>':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == cur ) {
			lexer->cursor++;
			if ( s_nextChar( lexer ) == '=' ) {
				token->kind = CKTOK3( cur, cur, '=' );
				lexer->cursor++;
			} else token->kind = CKTOK2( cur, cur );
		} else if ( next == '=' ) {
			token->kind = CKTOK2( cur, '=' );
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// Tokens that use $$ and $
	case ':':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == cur ) {
			token->kind = CKTOK2( cur, cur );
			lexer->cursor++;
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// Tokens that use $$, $$$ and $
	case '.':
	{
		char next;
		lexer->cursor++;
		next = s_nextChar( lexer );
		if ( next == cur ) {
			lexer->cursor++;
			if ( s_nextChar( lexer ) == cur ) {
				token->kind = CKTOK3( cur, cur, cur );
				lexer->cursor++;
			} else token->kind = CKTOK2( cur, cur );
		} else token->kind = (uint64_t)cur;

		token->position = base;
		return true;
	}

	// END OF SWITCH
	}

	// Number literals (ints and floats)
	if ( isdigit( cur ) ) {
		uint64_t accumulator = 0;

		// Parse octal, binary and hexadecimal
		if ( cur == '0' ) {
			lexer->cursor++;
			cur = s_nextChar( lexer );

			// Binary
			if ( cur == 'b' || cur == 'B' ) {
				lexer->cursor++;
				cur = s_nextChar( lexer );
				while ( cur == '0' || cur == '1' ) {
					accumulator <<= 1;
					accumulator += (uint64_t)(cur - '0');
					lexer->cursor++;
					cur = s_nextChar( lexer );
				}
				// Hexadecimal
			} else if ( cur == 'x' || cur == 'X' ) {
				lexer->cursor++;
				cur = s_nextChar( lexer );
				while ( isdigit( cur )
					|| (cur <= 'F' && cur >= 'A')
					|| (cur <= 'f' && cur >= 'a') ) {
					accumulator <<= 4;
					if ( cur <= '9' && cur >= '0' )
						accumulator += (uint64_t)(cur - '0');
					else if ( cur <= 'F' && cur >= 'A' )
						accumulator += (uint64_t)(cur - 'A' + 10);
					else if ( cur <= 'f' && cur >= 'a' )
						accumulator += (uint64_t)(cur - 'a' + 10);
					lexer->cursor++;
					cur = s_nextChar( lexer );
				}
				// Octal
			} else {
				while ( cur <= '7' && cur >= '0' ) {
					accumulator <<= 3;
					accumulator += (uint64_t)(cur - '0');
					lexer->cursor++;
					cur = s_nextChar( lexer );
				}
			}
			token->kind = '0';
			token->position = base;
			token->value.u64 = accumulator;
			return true;
		}

		// Decimal
		while ( isdigit( cur ) ) {
			accumulator *= 10;
			accumulator += (uint64_t)(cur - '0');
			lexer->cursor++;
			cur = s_nextChar( lexer );
		}
		// Float
		if ( cur == '.' ) {
			long double scale = 0.1;
			long double accFloat = 0.0;
			accFloat = (long double)accumulator;
			lexer->cursor++;
			cur = s_nextChar( lexer );
			while ( isdigit( cur ) ) {
				accFloat += (long double)(cur - '0') * scale;
				scale *= 0.1;
				lexer->cursor++;
				cur = s_nextChar( lexer );
			}
			if ( cur == 'e' || cur == 'E' ) {
				long double exponent = 0.0;
				long double sign = 1;
				lexer->cursor++;
				cur = s_nextChar( lexer );
				if ( cur == '-' ) {
					lexer->cursor++;
					cur = s_nextChar( lexer );
					sign = -1;
				}
				while ( isdigit( cur ) ) {
					exponent *= 10;
					exponent += (long double)(cur - '0');
					lexer->cursor++;
					cur = s_nextChar( lexer );
				}
				accFloat *= (long double)pow( 10, exponent * sign );
			} else if ( cur == 'p' || cur == 'P' ) {
				long double exponent = 0.0;
				long double sign = 1;
				lexer->cursor++;
				cur = s_nextChar( lexer );
				if ( cur == '-' ) {
					lexer->cursor++;
					cur = s_nextChar( lexer );
					sign = -1;
				}
				while ( isdigit( cur ) ) {
					exponent *= 10;
					exponent += (long double)(cur - '0');
					lexer->cursor++;
					cur = s_nextChar( lexer );
				}
				accFloat *= (long double)pow( 2, exponent * sign );
			}
			token->position = base;
			token->kind = 'F';
			token->value.f64 = accFloat;
			return true;
		}
		// Int return
		token->position = base;
		token->kind = '0';
		token->value.u64 = accumulator;
		return true;
	}

	// Identifiers and keywords
	if ( isalpha( cur ) || cur == '_' ) {
		size_t length = 0;
		char *strbuf;
		while ( isalnum( cur ) || cur == '_' ) {
			lexer->cursor++;
			cur = s_nextChar( lexer );
			length++;
		}

		strbuf = CkArenaAllocate( lexer->arena, length + 1 );
		memcpy( strbuf, lexer->source->code + base, length );
		strbuf[length] = 0;
		for ( size_t i = 0; i < sizeof( s_keywordDict ) / sizeof( KeywordEntryPair ); i++ ) {
			if ( !strcmp( strbuf, s_keywordDict[i].key ) ) {
				token->position = base;
				token->kind = s_keywordDict[i].value;
				return true;
			}
		}
		token->position = base;
		token->kind = 'I';
		token->value.cstr = strbuf;
		return true;
	}

	// Character literal
	if ( cur == '\'' ) {
		uint64_t accumulator = 0;
		lexer->cursor++;
		cur = s_nextChar( lexer );
		while ( cur != '\'' && cur != 0 ) {
			uint8_t value = (uint8_t)cur;
			accumulator <<= 8;
			if ( value == '\\' ) {
				lexer->cursor++;
				cur = s_nextChar( lexer );
				if ( cur == 0 ) {
					return false;
				}
				value = s_escapeSequence( lexer );
			}
			accumulator += value;
			lexer->cursor++;
			cur = s_nextChar( lexer );
		}
		lexer->cursor++;
		token->position = base;
		token->kind = '0';
		token->value.u64 = accumulator;
		return true;
	}

	if ( cur == '\"' ) {
		size_t length = 0;
		size_t end = 0;
		lexer->cursor++;
		cur = s_nextChar( lexer );
		while ( cur != '\"' && cur != 0 ) {
			if ( cur == '\\' ) {
				lexer->cursor++;
				(void)s_escapeSequence( lexer );
			} else if ( cur == '\n' ) { // Newline not allowed
				token->position = base;
				token->kind = 'S';
				lexer->cursor++;
				return false;
			}
			lexer->cursor++;
			length++;
			cur = s_nextChar( lexer );
		}
		end = lexer->cursor + 1;
		token->value.cstr = CkArenaAllocate( lexer->arena, length + 1 );
		lexer->cursor = base + 1;
		for ( size_t i = 0; i < length; i++ ) {
			token->value.cstr[i] = s_nextChar( lexer );
			if ( token->value.cstr[i] == '\\' ) {
				lexer->cursor++;
				token->value.cstr[i] = (char)s_escapeSequence( lexer );
			}
			lexer->cursor++;
		}
		lexer->cursor = end;
		token->position = base;
		token->kind = 'S';
		return true;
	}

	if ( cur == '#' && allow_ppdirect ) {
		size_t length = 0;
		char *strbuf;
		lexer->cursor++; // #
		cur = s_nextChar( lexer );
		base = lexer->cursor;
		while ( isalnum( cur ) || cur == '_' ) {
			lexer->cursor++;
			cur = s_nextChar( lexer );
			length++;
		}

		strbuf = CkArenaAllocate( lexer->arena, length + 1 );
		memcpy( strbuf, lexer->source->code + base, length );
		strbuf[length] = 0;
		token->value.cstr = strbuf; // used for error reporting (malformed + unknown)
		for ( size_t i = 0; i < sizeof( s_macroDict ) / sizeof( MacroDirectiveEntryPair ); i++ ) {
			if ( !strcmp( strbuf, s_macroDict[i].key ) ) {
				token->position = base;
				token->kind = s_macroDict[i].value;
				return _ParsePreprocessorDirective( lexer->arena, lexer, token );
			}
		}
		token->position = base;
		token->kind = PP_DIRECTIVE_UNKNOWN;
		return false;
	}

	token->position = base;
	token->kind = (uint64_t)cur;
	lexer->cursor++;
	return false;
}
