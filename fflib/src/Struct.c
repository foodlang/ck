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

void FFAllocateFunction( FFScope *scope, CkFoodType *returnType, char *passedName, CkList *pPassedArgumentList )
{
	FFFunction func = {};

	CK_ARG_NON_NULL( scope );
	CK_ARG_NON_NULL( returnType );
	CK_ARG_NON_NULL( passedName );
	CK_ARG_NON_NULL( pPassedArgumentList );

	func.parent = scope;
	func.returnType = returnType;
}
