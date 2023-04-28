#include <IL/FFStruct.h>
#include <Syntax/Expression.h>

#include <string.h>

#include <CDebug.h>

CkScope *CkStartScope(
	CkArenaFrame *allocator,
	CkScope *optionalParent,
	bool_t allowedLabels,
	bool_t allowedFunctions )
{
	CkScope *yield;

	CK_ARG_NON_NULL( allocator );
	
	yield = CkArenaAllocate( allocator, sizeof( CkScope ) );
	yield->parent = optionalParent;
	if ( optionalParent ) {
		yield->library = optionalParent->library;
		yield->module = optionalParent->module;
		CkListAdd( optionalParent->children, &yield );
	}

	// Variables
	yield->variableList = CkListStart( allocator, sizeof( CkVariable ) );

	// Labels
	if ( allowedLabels ) {
		yield->supportsLabels = TRUE;
		yield->labelList = CkListStart( allocator, sizeof( CkLabel ) );
	}

	// Functions
	if ( allowedFunctions ) {
		yield->supportsFunctions = TRUE;
		yield->functionList = CkListStart( allocator, sizeof( CkFunction ) );
		yield->usertypeList = CkListStart( allocator, sizeof( CkUserType ) );
	}

	yield->children = CkListStart( allocator, sizeof( CkScope * ) );

	return yield;
}

CkScope *CkLeaveScope( CkScope *current )
{
	CK_ARG_NON_NULL( current );

	return current->parent ? current->parent : current;
}

void CkAllocateVariable( CkScope *scope, CkFoodType *type, char *passedName, bool_t param )
{
	CkVariable var = {};

	CK_ARG_NON_NULL( scope );
	CK_ARG_NON_NULL( type );
	CK_ARG_NON_NULL( passedName );

	var.parentScope = scope;
	var.name = passedName;
	var.type = type;
	var.param = param;

	CkListAdd( scope->variableList, &var );
}

CkFunction *CkAllocateFunction(
	CkArenaFrame *allocator,
	CkScope *scope,
	bool_t bPublic,
	CkFoodType *signature,
	char *passedName,
	CkStatement *body )
{
	CkFunction func = {};

	CK_ARG_NON_NULL( scope );
	CK_ARG_NON_NULL( passedName );
	CK_ARG_NON_NULL( signature );

	func.parent = scope;
	func.signature = signature;
	func.body = body;
	func.bPublic = bPublic;
	func.name = passedName;
	func.funscope = CkStartScope( allocator, scope, TRUE, FALSE ); // TODO: nested functions
	CkListAdd( scope->functionList, &func );
	return CkListAccess( scope->functionList, CkListLength( scope->functionList ) - 1 );
}

CkLibrary *CkCreateLibrary( CkArenaFrame *allocator, char *passedName )
{
	CkLibrary *lib = CkArenaAllocate( allocator, sizeof( CkLibrary ) );
	lib->name = passedName;
	lib->scope = CkStartScope( allocator, NULL, FALSE, TRUE );
	lib->scope->library = lib;
	lib->scope->module = NULL;
	lib->moduleList = CkListStart( allocator, sizeof( CkModule * ) );
	lib->dependenciesList = CkListStart( allocator, sizeof( char * ) );
	return lib;
}

CkModule *CkCreateModule(
	CkArenaFrame *allocator,
	CkLibrary *parent,
	char *passedName,
	bool_t isPublic,
	bool_t isStatic )
{
	CkModule *mod = CkArenaAllocate( allocator, sizeof( CkModule ) );
	mod->bPublic = isPublic;
	mod->bStatic = isStatic;
	mod->name = passedName;
	mod->scope = CkStartScope( allocator, parent->scope, FALSE, TRUE );
	mod->scope->module = mod;
	CkListAdd( parent->moduleList, &mod );
	return mod;
}

static void s_PrintStmt( size_t indent, CkStatement *stmt )
{
	if ( !stmt )
		return;
	for ( size_t i = 0; i < indent; i++ )
		putc( '\t', stdout );
	switch ( stmt->stmt ) {

	case CK_STMT_EMPTY:
		printf( "Empty statement\n" );
		return;

	case CK_STMT_EXPRESSION:
		printf( "Expression:\n" );
		CkExpressionPrint( stmt->data.expression );
		return;

	case CK_STMT_BLOCK:
	{
		size_t len = CkListLength( stmt->data.block.stmts );
		printf( "Block statement:\n" );
		for ( size_t i = 0; i < len; i++ ) {
			s_PrintStmt(
				indent + 1,
				*(CkStatement **)CkListAccess( stmt->data.block.stmts, i ) );
		}
		return;
	}

	case CK_STMT_IF:
		printf( "If statement:\n" );
		// Then
		s_PrintStmt( indent + 1, stmt->data.if_.cThen );
		// Else
		if ( stmt->data.if_.cElse )
			s_PrintStmt( indent + 1, stmt->data.if_.cElse );
		return;

	case CK_STMT_WHILE:
		printf( "While statement:\n" );
		s_PrintStmt( indent + 1, stmt->data.while_.cWhile );
		return;

	case CK_STMT_DO_WHILE:
		printf( "Do/while statement:\n" );
		s_PrintStmt( indent + 1, stmt->data.while_.cWhile );
		return;

	case CK_STMT_FOR:
		printf( "For statement:\n" );
		s_PrintStmt( indent + 1, stmt->data.for_.cInit );
		CkExpressionPrint( stmt->data.for_.condition );
		CkExpressionPrint( stmt->data.for_.lead );
		s_PrintStmt( indent + 1, stmt->data.for_.body );
		return;

	default:
		printf( "Unsupported statement %u\n", stmt->stmt );
		return;
	}
}

void CkPrintAST( CkLibrary *library )
{
	size_t moduleCount = CkListLength( library->moduleList );
	printf( "Library '%s':\n", library->name );
	for ( size_t i = 0; i < moduleCount; i++ ) {
		size_t funcCount = 0;
		CkModule *mod = *(CkModule **)CkListAccess( library->moduleList, i );
		printf( "\tModule '%s' (public = %hhx, static = %hhx):\n", mod->name, mod->bPublic, mod->bStatic );
		funcCount = CkListLength( mod->scope->functionList );
		for ( size_t j = 0; j < funcCount; j++ ) {
			CkFunction *func = CkListAccess( mod->scope->functionList, j );
			printf( "\t\tFunction '%s' (public = %hhx):\n", func->name, func->bPublic );
			s_PrintStmt( 3, func->body );
		}
	}
}

bool_t CkSymbolDeclared( CkScope* current, char* passedName )
{
	const size_t varCount = CkListLength( current->variableList );
	const size_t funcCount = current->supportsFunctions ? CkListLength( current->functionList ) : 0;

	for ( size_t i = 0; i < varCount; i++ ) {
		CkVariable *var = CkListAccess( current->variableList, i );
		if ( !strcmp( var->name, passedName ) )
			return TRUE;
	}

	for ( size_t i = 0; i < funcCount; i++ ) {
		CkFunction *func = CkListAccess( current->functionList, i );
		if ( !strcmp( func->name, passedName ) )
			return TRUE;
	}

	return current->parent ? CkSymbolDeclared( current->parent, passedName ) : FALSE;
}
