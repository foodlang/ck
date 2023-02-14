#include <fflib/FFStruct.h>

#include <ckmem/CDebug.h>

FFScope *FFStartScope(
	CkArenaFrame *allocator,
	FFScope *optionalParent,
	bool_t allowedLabels,
	bool_t allowedFunctions )
{
	FFScope *yield;

	CK_ARG_NON_NULL( allocator );
	
	yield = CkArenaAllocate( allocator, sizeof( FFScope ) );
	yield->parent = optionalParent;

	// Variables
	yield->variableList = CkListStart( allocator, sizeof( FFVariable ) );

	// Labels
	if ( allowedLabels ) {
		yield->supportsLabels = TRUE;
		yield->labelList = CkListStart( allocator, sizeof( FFLabel ) );
	}

	// Functions
	if ( allowedFunctions ) {
		yield->supportsFunctions = TRUE;
		yield->functionList = CkListStart( allocator, sizeof( FFFunction ) );
	}

	return yield;
}

FFScope *FFLeaveScope( FFScope *current )
{
	CK_ARG_NON_NULL( current );

	return current->parent ? current->parent : current;
}

void FFAllocateVariable( FFScope *scope, CkFoodType *type, char *passedName )
{
	FFVariable var = {};

	CK_ARG_NON_NULL( scope );
	CK_ARG_NON_NULL( type );
	CK_ARG_NON_NULL( passedName );

	var.parentScope = scope;
	var.name = passedName;
	var.type = type;

	CkListAdd( scope->variableList, &var );
}

void FFAllocateFunction(
	FFScope *scope,
	bool_t bPublic,
	CkFoodType *returnType,
	char *passedName,
	CkList *pPassedArgumentList,
	FFStatement *body )
{
	FFFunction func = {};

	CK_ARG_NON_NULL( scope );
	CK_ARG_NON_NULL( returnType );
	CK_ARG_NON_NULL( passedName );
	CK_ARG_NON_NULL( pPassedArgumentList );

	func.parent = scope;
	func.returnType = returnType;
	func.body = body;
	func.bPublic = bPublic;
	func.name = passedName;
	func.passedArguments = pPassedArgumentList;
	CkListAdd( scope->functionList, &func );
}

FFLibrary *FFCreateLibrary( CkArenaFrame *allocator, char *passedName )
{
	FFLibrary *lib = CkArenaAllocate( allocator, sizeof( FFLibrary ) );
	lib->name = passedName;
	lib->scope = FFStartScope( allocator, NULL, FALSE, TRUE );
	lib->moduleList = CkListStart( allocator, sizeof( FFModule * ) );
	lib->dependenciesList = CkListStart( allocator, sizeof( char * ) );
	return lib;
}

FFModule *FFCreateModule(
	CkArenaFrame *allocator,
	FFLibrary *parent,
	char *passedName,
	bool_t isPublic,
	bool_t isStatic )
{
	FFModule *mod = CkArenaAllocate( allocator, sizeof( FFModule ) );
	mod->bPublic = isPublic;
	mod->bStatic = isStatic;
	mod->name = passedName;
	mod->scope = FFStartScope( allocator, parent->scope, FALSE, TRUE );
	CkListAdd( parent->moduleList, &mod );
	return mod;
}

static void s_PrintStmt( size_t indent, FFStatement *stmt )
{
	if ( !stmt )
		return;
	for ( size_t i = 0; i < indent; i++ )
		putc( '\t', stdout );
	switch ( stmt->stmt ) {

	case FF_STMT_EMPTY:
		printf_s( "Empty statement\n" );
		return;

	case FF_STMT_EXPRESSION:
		printf_s( "Expression statement\n" );
		return;

	case FF_STMT_BLOCK:
	{
		size_t len = CkListLength( stmt->data.block.stmts );
		printf_s( "Block statement:\n" );
		for ( size_t i = 0; i < len; i++ ) {
			s_PrintStmt(
				indent + 1,
				*(FFStatement **)CkListAccess( stmt->data.block.stmts, i ) );
		}
		return;
	}

	case FF_STMT_IF:
		printf_s( "If statement:\n" );
		// Then
		s_PrintStmt( indent + 1, stmt->data.if_.cThen );
		// Else
		if ( stmt->data.if_.cElse )
			s_PrintStmt( indent + 1, stmt->data.if_.cElse );
		return;

	case FF_STMT_WHILE:
		printf_s( "While statement:\n" );
		s_PrintStmt( indent + 1, stmt->data.while_.cWhile );
		return;

	case FF_STMT_DO_WHILE:
		printf_s( "Do/while statement:\n" );
		s_PrintStmt( indent + 1, stmt->data.while_.cWhile );
		return;

	default:
		printf_s( "Unsupported statement %u\n", stmt->stmt );
		return;
	}
}

void FFPrintAST( FFLibrary *library )
{
	size_t moduleCount = CkListLength( library->moduleList );
	printf_s( "Library '%s':\n", library->name );
	for ( size_t i = 0; i < moduleCount; i++ ) {
		size_t funcCount = 0;
		FFModule *mod = *(FFModule **)CkListAccess( library->moduleList, i );
		printf_s( "\tModule '%s' (public = %hhx, static = %hhx):\n", mod->name, mod->bPublic, mod->bStatic );
		funcCount = CkListLength( mod->scope->functionList );
		for ( size_t j = 0; j < funcCount; j++ ) {
			FFFunction *func = CkListAccess( mod->scope->functionList, j );
			printf_s( "\t\tFunction '%s' (public = %hhx):\n", func->name, func->bPublic );
			s_PrintStmt( 3, func->body );
		}
	}
}
