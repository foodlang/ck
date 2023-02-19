#include <Syntax/ParserDecl.h>
#include <Syntax/ParserTypes.h>
#include <CDebug.h>

bool_t CkParseDecl(
	CkArenaFrame *allocator    /* Allocator */,
	FFScope *context           /* The scope of the declaration. */,
	CkParserInstance* parser   /* The parser module instance. */,
	bool_t allowModule         /* Whether parsing modules is allowed. */,
	bool_t allowFuncStruct     /* Whether parsing functions and structures is allowed. */,
	bool_t allowNonConstAssign /* If this decl is a variable, whether it should allow non-constant assigns (FALSE for args) */,
	bool_t allowExposureQual   /* Whether the public, static or extern specifiers should be allowed */
) /* ret TRUE => decl, FALSE => not decl */
{
	CkToken token;          // The Current token.
	CkToken name;           // The name of the declaration.
	CkFoodType *declType;   // The declaration type.

	bool_t bPublic = FALSE; // Whether this symbol is public.
	bool_t bStatic = FALSE; // Whether this module is static.
	bool_t bExtern = FALSE; // Whether this symbol is external.

	// TODO: Add support for implicit typed variable declarations

	// 1. Exposure qualifiers + extern
	while ( TRUE ) {
		CkParserReadToken( parser, &token );
		if (token.kind == KW_PUBLIC) {
			if ( !allowExposureQual ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Exposure qualifiers (like public) are not allowed in this context." );
				return TRUE;
			}
			// Duplicate checking
			if (bPublic) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Duplicate public exposure qualifier." );
				return TRUE;
			}

			bPublic = TRUE;
		} else if ( token.kind == KW_STATIC ) {
			if ( !allowExposureQual ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Exposure qualifiers (like static) are not allowed in this context." );
				return TRUE;
			}
			// Duplicate checking
			if ( bPublic ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Duplicate static exposure qualifier." );
				return TRUE;
			}

			bStatic = TRUE;
		} else if ( token.kind == KW_EXTERN ) {
			// Duplicate checking
			if ( bExtern ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Duplicate extern qualifier." );
				return TRUE;
			}

			bExtern = TRUE;
		} else {
			CkParserRewind( parser, 1 );
			break;
		}
	}

	// 2. Parsing potential module
	if ( token.kind == KW_MODULE ) {
		FFModule *pModule; // Pointer to module instance

		// Compiler bug catching
		CK_ASSERT( !context->module );
		CK_ASSERT( context->library );

		// module >>>name<<< { decls }
		CkParserReadToken( parser, &name );
		if ( name.kind != 'I' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Module name must be an identifier." );
			return TRUE;
		}

		// Return value is ignored
		pModule = FFCreateModule( allocator, context->library, name.value.cstr, bPublic, bStatic );
		
		// module name >>>{<<< decls }
		CkParserReadToken( parser, &token );
		if ( token.kind != '{' ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Module declaration must be followed by module member declarations." );
			return TRUE;
		}

		// module name { >>decls }<<
		while ( TRUE ) {
			CkParserReadToken( parser, &token );
			if ( token.kind == '}' )
				break;
			if ( token.kind == 0 ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Module member declarations must be terminated with a closing curly bracket }." );
				return TRUE;
			}
			CkParserRewind( parser, 1 );
			CkParseDecl( parser->genArena, pModule->scope, parser, FALSE, TRUE, FALSE, TRUE );
		}

		return TRUE;
	}

	// Static is not allowed for non-module decl
	if ( bStatic ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Static exposure qualifier is invalid for non-module declarations. "
			"You might be looking for internal (local to the translation unit) exposure, "
		    "which is the default exposure in Food." );
		return TRUE;
	}

	// 3. Parsing the type
	declType = CkParserType( parser );
	if ( !declType ) return FALSE;

	// 4. Parsing the name
	CkParserReadToken( parser, &name );
	if ( name.kind != 'I' ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"A declaration's name must be an identifier." );
		return TRUE;
	}

	// 5. Parsing arguments or inline assignment
	CkParserReadToken( parser, &token );
	if ( token.kind == ';' ) { // T name; (un-assigned variable)
		FFAllocateVariable( context, declType, name.value.cstr );
		return TRUE;
	} else {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Only un-assigned variables can declared for now. TODO: Add support for functions and more" );
		return TRUE;
	}
}
