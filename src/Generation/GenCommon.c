#include <Generation/GenCommon.h>
#include <CDebug.h>

#include <string.h>
#ifdef SKIP // temporarily discontinued
/*
 * List of Lua functions:
 * ckgen_expression(expression)     -> string   :: Generates an expression
 * ckgen_scopeframe(variables)      -> string[] :: Generates two strings, the first being the start of the frame, the second the end.
 * ckgen_zero(ptr, sz)              -> string   :: Generates code to set a region of memory to zero.
 * ckgen_label(n)                   -> string   :: Generates a label.
 * ckgen_goto(n)                    -> string   :: Goes to a label.
 * ckgen_gotoaddr(ptr)              -> string   :: Goes to a specific address.
 * ckgen_reference(sym)             -> string   :: Code to reference a symbol.
 * ckgen_func(bPublic, name)        -> string   :: Generates a function header.
 * (optional) ckgen_vardecl(var)    -> string   :: Generates a variable. Optional cause of ckgen_scope.
 * (optional) ckgen_comment(string) -> string   :: Generates a comment
 * (optional) ckgen_peephole(string)-> string[] :: Allows the addition of peephole optimizations. Returns regex patterns.
*/

// A pointer to... well, an instance of an empty string.
static char* s_emptyString = NULL;

static void s_CreateExpressionTable( lua_State* script, CkExpression* expr )
{
	lua_newtable( script );
	lua_pushstring( script, "value" );
	if ( expr->kind == CK_EXPRESSION_INTEGER_LITERAL )
		lua_pushinteger( script, expr->token.value.i64 );
	else if ( expr->kind == CK_EXPRESSION_BOOL_LITERAL )
		lua_pushboolean( script, expr->token.value.boolean );
	else if ( expr->kind == CK_EXPRESSION_FLOAT_LITERAL )
		lua_pushnumber( script, expr->token.value.f64 );
	else if ( expr->kind == CK_EXPRESSION_STRING_LITERAL )
		lua_pushstring( script, expr->token.value.cstr );
	else if ( expr->kind == CK_EXPRESSION_COMPOUND_LITERAL ) {
		printf( "compound literal not support in GenCommon.c" );
		abort();
	}

	lua_pushstring( script, "left" );
	if ( expr->left ) s_CreateExpressionTable( script, expr->left );
	else lua_pushnil( script );

	if ( expr->right ) {
		lua_pushstring( script, "right" );
		s_CreateExpressionTable( script, expr->right );
	}
	if ( expr->right ) s_CreateExpressionTable( script, expr->right );
	else lua_pushnil( script );
	if ( expr->extra ) s_CreateExpressionTable( script, expr->extra );
	else lua_pushnil( script );
	lua_settable( script, -3 );
}

static char* s_GenExpression( CkDiagnosticHandlerInstance* pDhi, lua_State* script, CkExpression* expr )
{
	char* ret;

	lua_getglobal( script, "ckgen_expression" );
	s_CreateExpressionTable( script, expr );

	if ( lua_pcall( script, 1, 1, 0 ) ) {
		CkDiagnosticThrow( pDhi, 0, CK_DIAG_SEVERITY_ERROR, "",
			"The generator used does not provide the mandatory function called ckgen_expression" );
		return s_emptyString;
	}

	if ( !lua_isstring( script, -1 ) ) {
		CkDiagnosticThrow( pDhi, 0, CK_DIAG_SEVERITY_ERROR, "",
			"The generator used does not provide the mandatory function ckgen_expression that returns a string." );
		return s_emptyString; // ref to empty string
	}

	ret = (char*)lua_tostring( script, -1 );
	lua_pop( script, 1 );
	return ret;
}

// Asks the script to generate a comment. The returned string
// is managed by the Lua environment (and its GC).
static char* s_GenComment( CkDiagnosticHandlerInstance* pDhi, lua_State* script, char *comment )
{
	char* ret; // returned value

	// Preparing the function & pushing arguments
	lua_getglobal( script, "ckgen_comment" );
	lua_pushstring( script, comment );

	// Calling the function
	if ( lua_pcall( script, 1, 1, 0 ) ) {
		CkDiagnosticThrow( pDhi, 0, CK_DIAG_SEVERITY_WARNING, "missing-optional-generator-functions",
			"The generator used does not provide a function called ckgen_comment(str)." );
		return s_emptyString; // ref to empty string
	}

	// Getting the returned value
	if ( !lua_isstring( script, -1 ) ) {
		CkDiagnosticThrow( pDhi, 0, CK_DIAG_SEVERITY_WARNING, "missing-optional-generator-functions",
			"The generator used does not provide a function called ckgen_comment(str) that returns a string." );
		return s_emptyString; // ref to empty string
	}
	ret = (char *)lua_tostring( script, -1 );
	lua_pop( script, 1 );
	return ret;
}

static char* s_MakeFuncSymName( CkArenaFrame *frame, CkFunction* pFunc )
{
	CkStrBuilder sb;
	char* ret;

	// Building function name
	CkStrBuilderCreate( &sb, 8 );
	CkStrBuilderAppendString( &sb, pFunc->parent->library->name );
	CkStrBuilderAppendChar( &sb, '_' );
	CkStrBuilderAppendString( &sb, pFunc->parent->module->name );
	CkStrBuilderAppendString( &sb, "_f_" );
	CkStrBuilderAppendString( &sb, pFunc->name );

	// Copying to arena & disposing the builder
	ret = CkArenaAllocate( frame, sb.length );
	strcpy( ret, sb.base );
	CkStrBuilderDispose( &sb );

	return ret;
}

static char* s_GenFuncHeader( CkArenaFrame *frame, CkDiagnosticHandlerInstance* pDhi, lua_State* script, CkFunction* pFunc )
{
	char* ret;

	lua_getglobal( script, "ckgen_func" );

	lua_pushboolean( script, pFunc->bPublic );
	lua_pushstring( script, s_MakeFuncSymName( frame, pFunc ) );

	if ( lua_pcall( script, 1, 1, 0 ) ) {
		CkDiagnosticThrow( pDhi, 0, CK_DIAG_SEVERITY_ERROR, "",
			"The generator used does not provide the mandatory function called ckgen_func." );
		return s_emptyString;
	}

	if ( !lua_isstring( script, -1 ) ) {
		CkDiagnosticThrow( pDhi, 0, CK_DIAG_SEVERITY_ERROR, "",
			"The generator used does not provide the mandatory function ckgen_func that returns a string." );
		return s_emptyString; // ref to empty string
	}

	return ret;
}

static char* s_GenStatement( CkArenaFrame* frame, CkDiagnosticHandlerInstance* pDhi, lua_State* script, CkStatement* stmt )
{
	// Generating statement
	CkStatement* stmt = *(CkStatement **)CkListAccess( blk->data.block.stmts, i );

	switch ( stmt->stmt ) {
		// Block statement {}
	case CK_STMT_BLOCK:
		return s_GenBlock( frame, pDhi, script, stmt );

		// Empty statement ;
	case CK_STMT_EMPTY:
		return s_emptyString;

		// Expression statement  d = 1
	case CK_STMT_EXPRESSION:
		return s_GenExpression( pDhi, script, stmt->data.expression );

		// Sponge statement sponge <stmt>
	case CK_STMT_SPONGE:
		return s_GenStatement( frame, pDhi, script, stmt->data.sponge.statement );
	}

	return s_GenComment( pDhi, script, "Unknown statement" );
}

static void s_GenScopeFrame(
	CkArenaFrame* frame,
	CkDiagnosticHandlerInstance* pDhi,
	lua_State* script, 
	CkScope* scope,
	char** ppEnter,
	char** ppLeave
	)
{
	size_t variableCount; // The variable count
	
	variableCount = CkListLength( scope->variableList );
	
	for ( size_t i = 0; i < variableCount; i++ ) {

	}
}

static char* s_GenBlock( CkArenaFrame* frame, CkDiagnosticHandlerInstance* pDhi, lua_State* script, CkStatement* blk )
{
	CkStrBuilder sb;  // Output string builder
	char* ret;        // Output string
	size_t varCount;  // Count of variables in scope
	size_t blkLength; // Count of elements in block
	char* scopeEnter; // Enter code for scope
	char* scopeLeave; // Leave code for scope

	blkLength = CkListLength( blk->data.block.stmts );
	varCount = CkListLength( blk->data.block.scope->variableList );

	// Handling empty block (identical to nop)
	if ( blkLength == 0 ) return s_emptyString;

	// Creating string builder
	CkStrBuilderCreate( &sb, 32 );

	// Generating scope & variables
	if ( varCount != 0 ) {
		s_GenScopeFrame( frame, pDhi, script, blk->data.block.scope, &scopeEnter, &scopeLeave );
		CkStrBuilderAppendString( &sb, scopeEnter );
	}

	// Generating statements
	for ( size_t i = 0; i < blkLength; i++ ) {
		CkStatement* stmt = *(CkStatement **)CkListAccess( blk->data.block.stmts, i );
		CkStrBuilderAppendString( &sb, s_GenStatement( frame, pDhi, script, stmt ) );
	}

	// Scope leave
	if ( varCount != 0 ) CkStrBuilderAppendString( &sb, scopeLeave );

	// Returning
	ret = CkArenaAllocate( frame, sb.length );
	strcpy( ret, sb.base );
	CkStrBuilderDispose( &sb );
	return ret;
}

char* CkGenProgram(
	CkDiagnosticHandlerInstance *pDhi,
	CkArenaFrame *allocator,
	CkList* libraries,
	lua_State* script )
{
	CkStrBuilder outsb;   // The output code (string buffer)
	char* out;            // The output code
	char* current;        // The currently generated string
	size_t libCount;      // The number of libraries to generate

	CK_ARG_NON_NULL( pDhi );
	CK_ARG_NON_NULL( allocator );
	CK_ARG_NON_NULL( libraries );
	CK_ARG_NON_NULL( script );

	// Allocating an empty string in the current arena if not available.
	// Arena memory is zero'd by default.
	if ( s_emptyString < (char *)allocator->base
		|| s_emptyString > (char *)allocator->base + allocator->size )
		s_emptyString = CkArenaAllocate( allocator, 1 );

	// Creating the string buffer
	CkStrBuilderCreate( &outsb, 4096 );

	// Add APIs
	luaL_dofile( script, "API.lua" );

	// Creating CK header
	CkStrBuilderAppendString( &outsb, s_GenComment( pDhi, script, "Compiled with CK, Food's Official Compiler" ) );

	// Flattening
	libCount = CkListLength( libraries );

	for ( size_t i = 0; i < libCount; i++ ) {
		CkLibrary* curlib = *(CkLibrary**)CkListAccess( libraries, i );
		size_t modCount = CkListLength( curlib->moduleList );
		size_t funcCount = curlib->scope->supportsFunctions ? CkListLength( curlib->scope->functionList ) : 0;

		for ( size_t i = 0; i < funcCount; i++ ) {
			CkFunction* func = CkListAccess( curlib->scope->functionList, i );

			// Header
			CkStrBuilderAppendString( &outsb, s_GenFuncHeader( allocator, pDhi, script, func ) );

			// Body
			if ( func->body->stmt == CK_STMT_EXPRESSION ) // expr
				CkStrBuilderAppendString( &outsb, s_GenExpression( pDhi, script, func->body->data.expression ) );
			else // block
				CkStrBuilderAppendString( &outsb, s_GenBlock( allocator, pDhi, script, func->body ) );
		}
		
		for ( size_t i = 0; i < modCount; i++ ) {
			CkModule* curmod = *(CkModule**)CkListAccess( curlib->moduleList, i );
			funcCount = curmod->scope->supportsFunctions ? CkListLength( curmod->scope->functionList ) : 0;

			for ( size_t j = 0; j < funcCount; j++ ) {
				CkFunction* func = CkListAccess( curmod->scope->functionList, i );
				CkListAdd( flatFuncList, &func );
			}
		}
	}

	// Deleting the string buffer & copying its contents to
	// the static arena buffer
	out = CkArenaAllocate( allocator, outsb.length + 1 );
	strcpy( out, outsb.base );
	CkStrBuilderDispose( &outsb );
	return out;
}
#endif

void CkGenerate( CkGenerator *pGen )
{
	// TODO
}
