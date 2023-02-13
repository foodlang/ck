#include <include/Syntax/ParserTypes.h>
#include <ckmem/CDebug.h>

/// <summary>
/// A type ID pair.
/// </summary>
typedef struct KwTypeIDPair
{
	/// <summary>
	/// The keyword.
	/// </summary>

	/// <summary>
	/// The type ID.
	/// </summary>
	uint64_t kw;
	uint8_t  id;

} KwTypeIDPair;

/// <summary>
/// A dictionary to look up type IDs from specific keywords.
/// </summary>
static const KwTypeIDPair s_kwTypeIDPair[] =
{
	{ KW_VOID, CK_FOOD_VOID },
	{ KW_BOOL, CK_FOOD_BOOL },
	{ KW_I8, CK_FOOD_I8 },
	{ KW_U8, CK_FOOD_U8 },
	{ KW_I16, CK_FOOD_I16 },
	{ KW_U16, CK_FOOD_U16 },
	{ KW_F16, CK_FOOD_F16 },
	{ KW_I32, CK_FOOD_I32 },
	{ KW_U32, CK_FOOD_U32 },
	{ KW_F32, CK_FOOD_F32 },
	{ KW_I64, CK_FOOD_I64 },
	{ KW_U64, CK_FOOD_U64 },
	{ KW_F64, CK_FOOD_F64 },
	{ KW_STRING, CK_FOOD_STRING },
};

static uint8_t s_ParseQualifiers( CkParserInstance *parser )
{
	CkToken token;
	uint8_t attr = 0;

	CK_ARG_NON_NULL( parser )

		while ( TRUE ) {
			CkParserReadToken( parser, &token );
			switch ( token.kind ) {
				// Const qualifier
			case KW_CONST:

				// Const is already specified
				if ( attr & CK_QUALIFIER_CONST_BIT ) {
					CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
						"Duplicate const qualifier." );
				}
				attr |= CK_QUALIFIER_CONST_BIT;
				break;

				// Volatile qualifier
			case KW_VOLATILE:

				// Volatile is already specified
				if ( attr & CK_QUALIFIER_VOLATILE_BIT ) {
					CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
						"Duplicate volatile qualifier." );
				}
				attr |= CK_QUALIFIER_VOLATILE_BIT;
				break;

				// Restrict qualifier
			case KW_RESTRICT:

				// Restrict is already specified
				if ( attr & CK_QUALIFIER_RESTRICT_BIT ) {
					CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
						"Duplicate restrict qualifier." );
				}
				attr |= CK_QUALIFIER_RESTRICT_BIT;
				break;

				// Atomic qualifier
			case KW_ATOMIC:
				if ( attr & CK_QUALIFIER_ATOMIC_BIT ) {
					CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
						"Duplicate atomic qualifier." );
				}
				attr |= CK_QUALIFIER_ATOMIC_BIT;
				break;

			default:
				goto Leave;
			}
		}
Leave:
	CkParserRewind( parser, 1 );
	return attr;
}

CkFoodType *CkParserType( CkParserInstance *parser )
{
	CkToken token;
	CkFoodType *acc;
	uint8_t attr = 0;
	uint8_t id = 0;

	// Attempts to parse qualifiers
	attr = s_ParseQualifiers( parser );

	// Type parsing
	CkParserReadToken( parser, &token );
	for ( size_t i = 0; i < sizeof( s_kwTypeIDPair ) / sizeof( KwTypeIDPair ); i++ ) {
		const KwTypeIDPair *pPair = s_kwTypeIDPair + i;
		if ( token.kind == pPair->kw ) {
			id = pPair->id;
			break;
		}
	}
	// TODO: Handle user-defined types and function pointers
	if ( id == 0 ) return NULL;
	acc = CkFoodCreateTypeInstance( parser->arena, id, attr, NULL );

	// Handle pointers and references
	// TODO: Handle arrays
	while ( TRUE ) {
		attr = s_ParseQualifiers( parser );
		CkParserReadToken( parser, &token );
		if ( token.kind == '*' ) {
			id = CK_FOOD_POINTER;
			acc = CkFoodCreateTypeInstance( parser->arena, id, attr, acc );
		} else if ( token.kind == '&' ) {
			id = CK_FOOD_REFERENCE;
			acc = CkFoodCreateTypeInstance( parser->arena, id, attr, acc );
			break; // References cannot be subtypes
		} else {
			CkParserRewind( parser, 1 );
			break;
		}
	}

	return acc;
}
