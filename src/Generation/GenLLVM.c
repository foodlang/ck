#include <Generation/GenLLVM.h>

#include <malloc.h>
#include <string.h>
#include <stdlib.h>

// Generates a function's name. Must be free'd.
static char *_Funcname( CkFunction *p_func )
{
	// TODO: Implement clang funcnames

	// ----- Get length -----
	char *namebuf;
	size_t namelen =
		strlen( "_?" ) +
		strlen( p_func->parent->library->name ) +
		strlen( "_" ) +
		strlen( p_func->name );

	if ( p_func->parent->module )
		namelen += strlen( "_" ) + p_func->parent->module->name;

	// ----- Allocate buffer & set name -----
	namebuf = malloc( namelen );

	strcpy( namebuf, "_?" );
	strcat( namebuf, p_func->parent->library->name );
	strcat( namebuf, "_" );
	if ( p_func->parent->module ) {
		strcat( namebuf, p_func->parent->module->name );
		strcat( namebuf, "_" );
	}
	strcat( namebuf, p_func->name );

	return namebuf;
}

// Generates a variable's name. Must be free'd.
static char *_Varname( CkVariable *p_var )
{
	// TODO: Implement clang funcnames

	// ----- Get length -----
	char *namebuf;
	size_t namelen =
		strlen( "_?" ) +
		strlen( p_var->parentScope->library->name ) +
		strlen( "_" ) +
		strlen( p_var->name );

	if ( p_var->parentScope->module )
		namelen += strlen( "_" ) + p_var->parentScope->module->name;

	// ----- Allocate buffer & set name -----
	namebuf = malloc( namelen );

	strcpy( namebuf, "_?" );
	strcat( namebuf, p_var->parentScope->library->name );
	strcat( namebuf, "_" );
	if ( p_var->parentScope->module ) {
		strcat( namebuf, p_var->parentScope->module->name );
		strcat( namebuf, "_" );
	}
	strcat( namebuf, p_var->name );

	return namebuf;
}

static LLVMTypeRef _GetLLVMType( CkFoodType *T )
{
	switch ( T->id ) {
	case CK_FOOD_VOID: return LLVMVoidType();

	case CK_FOOD_I8:
	case CK_FOOD_U8:
	case CK_FOOD_BOOL:
		return LLVMInt8Type();
	case CK_FOOD_I16:
	case CK_FOOD_U16:
		return LLVMInt16Type();
	case CK_FOOD_I32:
	case CK_FOOD_U32:
	case CK_FOOD_ENUM:
		return LLVMInt32Type();
	case CK_FOOD_I64:
	case CK_FOOD_U64:
		return LLVMInt64Type();

	case CK_FOOD_F16: return LLVMHalfType();
	case CK_FOOD_F32: return LLVMFloatType();
	case CK_FOOD_F64: return LLVMDoubleType();

	case CK_FOOD_POINTER:
	case CK_FOOD_REFERENCE:
	case CK_FOOD_STRING:
		return LLVMPointerTypeInContext( LLVMGetGlobalContext(), 0 );

	default: abort();
	}
}

static void _GenStmt( LLVMBuilderRef, CkStatement *stmt )
{

}

static void _CompileFunc( LLVMBuilderRef builder, CkFunction *func )
{
	size_t param_count;
	LLVMTypeRef* params;
	LLVMTypeRef ret;
	LLVMTypeRef functype;
	LLVMBasicBlockRef block;

	param_count = CkListLength( (CkList *)func->signature->extra );
	params = malloc( param_count * sizeof(LLVMTypeRef) );
	ret = _GetLLVMType( func->signature->child );

	for ( size_t i = 0; i < param_count; i++ )
		params[i] = _GetLLVMType(
			*(CkFoodType **)CkListAccess( (CkList *)func->signature->extra )
		);

	functype = LLVMFunctionType( ret, params, param_count, FALSE );
	block = LLVMAppendBasicBlock( functype, "entry" );
	LLVMPositionBuilderAtEnd( builder, entry );


}

// Compiles a library.
static void _CompileLib( CkLibrary *lib, LLVMModuleRef m )
{
	for ( CkList *node = lib->moduleList; node != NULL; node = node->next ) {
		CkModule *p_module = *(CkModule **)(node + 1);

		for ( CkList *fnode = p_module->scope->functionList; fnode != NULL; fnode = fnode->next )
			_CompileFunc( (CkFunction *)(node + 1) );
	}

	for ( CkList *fnode = p_module->scope->functionList; fnode != NULL; fnode = fnode->next )
		_CompileFunc( (CkFunction *)(node + 1) );
}

LLVMModuleRef CkCompileLibrariesLLVM(
	CkDiagnosticHandlerInstance *pDhi,
	CkList *libs/* elem=CkLibrary* */ )
{
	char *error;
	CkToken zero = {};
	LLVMModuleRef m = LLVMModuleCreateWithName( "program" );

	// Compiling libraries
	for ( CkList *node = libs; node != NULL; node = node->next )
		_CompileLib( *(CkLibrary **)(node + 1), m );

	if (LLVMVerifyModule( m, LLVMAbortProcessAction, &error ))
		CkDiagnosticThrow( pDhi, zero, CK_DIAG_SEVERITY_ERROR, "", "LLVM error: %s", error );
	LLVMDisposeMessage( error );

	return m;
}
