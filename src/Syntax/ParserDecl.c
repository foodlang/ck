#include <Syntax/ParserDecl.h>
#include <Syntax/ParserExpressions.h>
#include <Syntax/ParserStatements.h>
#include <Syntax/Parser.h>
#include <Syntax/ParserTypes.h>
#include <Syntax/Binder.h>
#include <FileIO.h>
#include <CDebug.h>

bool_t CkParseDecl(
	CkArenaFrame *allocator    /* Allocator */,
	CkScope *context           /* The scope of the declaration. */,
	CkParserInstance* parser   /* The parser module instance. */,
	bool_t allowModule         /* Whether parsing modules is allowed. */,
	bool_t allowFuncStruct     /* Whether parsing functions and structures is allowed. */,
	bool_t allowNonConstAssign /* If this decl is a variable, whether it should allow non-constant assigns (FALSE for args) */,
	bool_t allowExposureQual   /* Whether the public, static or extern specifiers should be allowed */,
	CkList *stmtList           /* A list of statements. Used to add statements if needed, elem = CkStatement * */
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
	CkParserReadToken(parser, &token);
	if ( token.kind == KW_MODULE ) {
		CkModule *pModule; // Pointer to module instance

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
		pModule = CkCreateModule( allocator, context->library, name.value.cstr, bPublic, bStatic );
		
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
			CkParseDecl( parser->genArena, pModule->scope, parser, FALSE, TRUE, FALSE, TRUE, NULL );
		}

		return TRUE;
	} else CkParserRewind( parser, 1 );

	// Static is not allowed for non-module decl
	if ( bStatic ) {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Static exposure qualifier is invalid for non-module declarations. "
			"You might be looking for internal (local to the translation unit) exposure, "
		    "which is the default exposure in Food." );
		return TRUE;
	}

	// 3. Parsing the type
	CkParserReadToken( parser, &token );
	if ( token.kind == 'I' && CkSymbolDeclared(context, token.value.cstr) ) return FALSE; // Is symbol?
	CkParserRewind( parser, 1 );
	declType = CkParserType( context, parser );
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
		CkAllocateVariable( context, declType, name.value.cstr, FALSE );
		return TRUE;
	} else if ( token.kind == '=' ) {
		CkStatement *assign;
		CkAllocateVariable( context, declType, name.value.cstr, FALSE );
		assign = CkArenaAllocate( allocator, sizeof( CkStatement ) );
		assign->stmt = CK_STMT_EXPRESSION;
		// The assignment expression, "synthesized"
		assign->data.expression = CkExpressionCreateBinary(
			allocator,
			&token,
			CK_EXPRESSION_ASSIGN,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_VOID, 0, NULL ),
			CkExpressionCreateLiteral(
				allocator,
				&name,
				CK_EXPRESSION_IDENTIFIER,
				declType
			),
			CkParserExpression( context, parser )
		);
		if ( !allowNonConstAssign && !assign->data.expression->isConstant ) {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Only compiler-time constant expressions are allowed in this context." );
			return TRUE;
		}
		CkListAdd( stmtList, &assign );
		return TRUE;
	} else if ( token.kind == '(' ) {
		CkList *params;
		CkStatement *body;
		CkFoodType *signature;
		CkFunction *dest;
		size_t paramsLen = 0;
		// Argument parsing
		params = CkListStart( allocator, sizeof( CkVariable ) );
		while ( TRUE ) {
			CkFoodType *T;
			CkVariable tmp;

			CkParserReadToken( parser, &token );
			if ( token.kind == ')' ) break;

			// Type
			CkParserRewind( parser, 1 );
			T = CkParserType( context, parser );
			if ( !T ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a typename" );
				continue;
			}
			
			// Name
			CkParserReadToken( parser, &token );
			if ( token.kind != 'I' ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Expected an identifier" );
				continue;
			}

			// TODO: default arguments
			tmp.name = CkStrDup(allocator, token.value.cstr);
			tmp.type = T;
			tmp.parentScope = NULL;
			CkListAdd( params, &tmp );
			paramsLen++;

			// Comma
			CkParserReadToken( parser, &token );
			if ( token.kind == ',' )
				continue;
			else if ( token.kind == ')' ) {
				CkParserRewind( parser, 1 );
			} else {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a comma" );
			}
		}
		signature = CkFoodCreateTypeInstance(
			allocator,
			CK_FOOD_FUNCPOINTER,
			CK_QUALIFIER_CONST_BIT,
			declType
		);
		signature->extra = CkListStart( allocator, sizeof( CkFoodType *) );
		for ( size_t i = 0; i < paramsLen; i++ ) {
			CkListAdd(
				(CkList *)signature->extra,
				((CkVariable*)CkListAccess( params, i ))->type
			);
		}

		// Declaring
		dest = CkAllocateFunction(
			allocator,
			context,
			bPublic,
			signature,
			CkStrDup( allocator, name.value.cstr ),
			NULL );
		for ( size_t i = 0; i < paramsLen; i++ ) {
			CkVariable *var = CkListAccess( params, i );
			CkAllocateVariable(
				dest->funscope,
				var->type,
				var->name,
				TRUE );
		}

		// Body
		CkParserReadToken( parser, &token );
		// Expression body
		if ( token.kind == CKTOK2( '=', '>' ) ) {
			body = CkArenaAllocate( allocator, sizeof( CkStatement ) );
			body->stmt = CK_STMT_EXPRESSION;
			body->data.expression = CkParserExpression( context, parser );

			CkParserReadToken( parser, &token );
			if ( token.kind != ';' ) {
				CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a semicolon" );
				return TRUE;
			}
		// Block body
		} else if ( token.kind == '{' ) {
			CkParserRewind( parser, 1 );
			body = CkParseStmt( dest->funscope, parser );
		} else {
			CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Expected a thick arrow `=>` or a block `{}`." );
			return TRUE;
		}
		dest->body = body;
		
		return TRUE;
	} else {
		CkDiagnosticThrow( parser->pDhi, token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Functions and user types are not yet supported (TODO)" );
		return TRUE;
	}
}
