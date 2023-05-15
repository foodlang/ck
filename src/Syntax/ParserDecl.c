#include <Syntax/ParserDecl.h>
#include <Syntax/ParserExpressions.h>
#include <Syntax/ParserStatements.h>
#include <Syntax/Parser.h>
#include <Syntax/ParserTypes.h>
#include <Syntax/Binder.h>
#include <FileIO.h>
#include <CDebug.h>

#include <string.h>

bool CkParseDecl(
	CkArenaFrame *allocator    /* Allocator */,
	CkScope *context           /* The scope of the declaration. */,
	CkParserInstance* parser   /* The parser module instance. */,
	bool allowModule         /* Whether parsing modules is allowed. */,
	bool allowFuncStruct     /* Whether parsing functions and structures is allowed. */,
	bool allowNonConstAssign /* If this decl is a variable, whether it should allow non-constant assigns (false for args) */,
	bool allowExposureQual   /* Whether the public, static or extern specifiers should be allowed */,
	CkList *stmtList           /* A list of statements. Used to add statements if needed, elem = CkStatement * */
) /* ret true => decl, false => not decl */
{
	CkToken token;          // The Current token.
	CkToken name;           // The name of the declaration.
	CkFoodType *declType;   // The declaration type.

	bool bPublic = false; // Whether this symbol is public.
	bool bStatic = false; // Whether this module is static.
	bool bExtern = false; // Whether this symbol is external.

	// TODO: Add support for implicit typed variable declarations

	// 1. Exposure qualifiers + extern
	while ( true ) {
		CkParserReadToken( parser, &token );
		if (token.kind == KW_PUBLIC) {
			if ( !allowExposureQual ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Exposure qualifiers (like public) are not allowed in this context." );
				return true;
			}
			// Duplicate checking
			if (bPublic) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Duplicate public exposure qualifier." );
				return true;
			}

			bPublic = true;
		} else if ( token.kind == KW_STATIC ) {
			if ( !allowExposureQual ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Exposure qualifiers (like static) are not allowed in this context." );
				return true;
			}
			// Duplicate checking
			if ( bStatic ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Duplicate static exposure qualifier." );
				return true;
			}

			bStatic = true;
		} else if ( token.kind == KW_EXTERN ) {
			// Duplicate checking
			if ( bExtern ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Duplicate extern qualifier." );
				return true;
			}

			bExtern = true;
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
			CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
				"Module name must be an identifier." );
			return true;
		}

		// Return value is ignored
		pModule = CkCreateModule( allocator, context->library, name.value.cstr, bPublic, bStatic );

		// module name >>>{<<< decls }
		CkParserReadToken( parser, &token );
		if ( token.kind != '{' ) {
			CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
				"Module declaration must be followed by module member declarations." );
			return true;
		}

		// module name { >>decls }<<
		while ( true ) {
			CkParserReadToken( parser, &token );
			if ( token.kind == '}' )
				break;
			if ( token.kind == 0 ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Module member declarations must be terminated with a closing curly bracket }." );
				return true;
			}
			CkParserRewind( parser, 1 );
			CkParseDecl( parser->genArena, pModule->scope, parser, false, true, false, true, NULL );
		}

		return true;
	} else CkParserRewind( parser, 1 );

	// Static is not allowed for non-module decl
	if ( bStatic ) {
		CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
			"Static exposure qualifier is invalid for non-module declarations. "
			"You might be looking for internal (local to the translation unit) exposure, "
		    "which is the default exposure in Food." );
		return true;
	}

	// 3. Parsing the type
	CkParserReadToken( parser, &token );
	if ( token.kind == 0 ) return false;
	if ( token.kind == 'I' && CkSymbolDeclared(context, token.value.cstr) ) return false; // Is symbol?
	CkParserRewind( parser, 1 );
	declType = CkParserType( context, parser );
	if ( !declType ) return false;

	// 4. Parsing the name
	CkParserReadToken( parser, &name );
	if ( name.kind != 'I' ) {
		CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
			"A declaration's name must be an identifier." );
		return true;
	}

	// 5. Parsing arguments or inline assignment
	CkParserReadToken( parser, &token );
	if ( token.kind == ';' ) { // T name; (un-assigned variable)
		CkAllocateVariable( context, declType, name.value.cstr, false );
		return true;
	} else if ( token.kind == '=' && !bExtern ) {
		CkStatement *assign;
		CkAllocateVariable( context, declType, name.value.cstr, false );
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
			CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
				"Only compiler-time constant expressions are allowed in this context." );
			return true;
		}
		CkListAdd( stmtList, &assign );
		return true;
	} else if ( token.kind == '(' ) {
		CkList *params;
		CkStatement *body;
		CkFoodType *signature;
		CkFunction *dest;
		size_t params_len = 0;

		CkParserRewind( parser, 1 );

		// Argument parsing
		params = CkListStart( allocator, sizeof( CkVariable ) );
		while ( true ) {
			CkFoodType *T;
			CkVariable tmp;

			CkParserReadToken( parser, &token );
			if ( token.kind == ')' ) break;
			else if ( token.kind == ',' );
			else if ( token.kind == '(' ) {
				CkParserReadToken( parser, &token );
				if ( token.kind == ')' )
					break;
				else CkParserRewind( parser, 1 );
			}
			else {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a closing bracket ) or comma , in function parameter list" );
				break;
			}

			// Type
			T = CkParserType( context, parser );
			if ( !T ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a typename" );
				break;
			}
			
			// Name
			CkParserReadToken( parser, &token );
			if ( token.kind != 'I' ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Expected an identifier" );
				CkParserRewind( parser, 1 );
				break;
			}

			// TODO: default arguments
			tmp.name = CkStrDup(allocator, token.value.cstr);
			tmp.type = T;
			tmp.parentScope = NULL;
			CkListAdd( params, &tmp );
			params_len++;

			// Comma
			CkParserReadToken( parser, &token );
			if ( token.kind == ',' || token.kind == ')' )
				CkParserRewind( parser, 1 );
			else {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a comma" );
				CkParserRewind( parser, 1 );
				break;
			}
		}
		signature = CkFoodCreateTypeInstance(
			allocator,
			CK_FOOD_FUNCPOINTER,
			CK_QUALIFIER_CONST_BIT,
			declType
		);
		signature->extra = CkListStart( allocator, sizeof( CkFoodType *) );
		for ( size_t i = 0; i < params_len; i++ ) {
			CkListAdd(
				(CkList *)signature->extra,
				&((CkVariable*)CkListAccess( params, i ))->type
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
		for ( size_t i = 0; i < params_len; i++ ) {
			CkVariable *var = CkListAccess( params, i );
			CkAllocateVariable(
				dest->funscope,
				var->type,
				var->name,
				true );
		}

		// Body
		CkParserReadToken( parser, &token );
		// Expression body
		if ( bExtern ) {
			body = NULL;
			if ( token.kind != ';' ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Extern functions cannot have a body, expected semicolon ;" );
				return true;
			}
		}
		if ( token.kind == CKTOK2( '=', '>' ) ) {
			body = CkArenaAllocate(allocator, sizeof(CkStatement));
			body->stmt = CK_STMT_EXPRESSION;
			body->data.expression = CkParserExpression( context, parser );
			memcpy(&body->prim, &body->data.expression->token, sizeof(CkToken));

			CkParserReadToken( parser, &token );
			if ( token.kind != ';' ) {
				CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
					"Expected a semicolon" );
				return true;
			}
		// Block body
		} else if ( token.kind == '{' ) {
			CkParserRewind( parser, 1 );
			body = CkParseStmt( dest->funscope, parser );
		} else {
			CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
				"Expected a thick arrow `=>` or a block `{}`." );
			return true;
		}
		dest->body = body;
		
		return true;
	} else {
		CkDiagnosticThrow( parser->pDhi, &token, CK_DIAG_SEVERITY_ERROR, "",
			"Expected a semicolon ;" );
		return true;
	}
}
