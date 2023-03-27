#include <IL/FFStruct.h>
#include <Syntax/ParserTypes.h>
#include <Syntax/ParserExpressions.h>
#include <CDebug.h>

// A type ID pair.
typedef struct KwTypeIDPair
{
	// The keyword.

	// The type ID.
	uint64_t kw;
	uint8_t  id;

} KwTypeIDPair;

// A dictionary to look up type IDs from specific keywords.
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

	CK_ARG_NON_NULL( parser );

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

CkFoodType *CkParserType( CkScope *scope, CkParserInstance *parser )
{
	CkToken token;
	CkFoodType *acc = 0;
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
	// TODO: Handle function pointers
	if ( id == 0 ) {
		// User type
		if ( token.kind == 'I' ) {
			acc = CkFoodCreateTypeInstance( parser->arena, CK_FOOD_USER, attr, NULL );
		}
		else {
			CkParserRewind( parser, 1 );
			return NULL;
		}
	} else acc = CkFoodCreateTypeInstance( parser->arena, id, attr, NULL );

	// Handle pointers, references and arrays.
	while ( TRUE ) {
		attr = s_ParseQualifiers( parser );
		CkParserReadToken( parser, &token );
		if ( token.kind == '*' ) { // Pointers
			id = CK_FOOD_POINTER;
			acc = CkFoodCreateTypeInstance( parser->arena, id, attr, acc );
		} else if ( token.kind == '&' ) { // References
			id = CK_FOOD_REFERENCE;
			acc = CkFoodCreateTypeInstance( parser->arena, id, attr, acc );

			// References cannot be subtypes
			break;
		} else if ( token.kind == '[' ) { // Arrays
			id = CK_FOOD_ARRAY;
			acc = CkFoodCreateTypeInstance( parser->arena, id, attr, acc );
			acc->extra = CkParserExpression( scope, parser ); // [ >>>expr<<< ]
			CkParserReadToken( parser, &token );
			if ( !acc->extra ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Expected an expression for array parser." );
			}
			if ( token.kind != ']' ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Expected closing square bracket at this token." );
			}

		} else {
			CkParserRewind( parser, 1 );
			break;
		}
	}

	return acc;
}
