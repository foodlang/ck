#include <Syntax/Binder.h>
#include <Syntax/Expression.h>
#include <Diagnostics.h>
#include <CDebug.h>
#include <Food.h>

#include <string.h>

// Tries to get the type of a symbol. Returns NULL if unresolved.
static const CkFoodType *s_TryGetSymbolType( char *symbol, CkScope *context )
{
	const size_t varCount  = CkListLength( context->variableList );
	const size_t funcCount = CkListLength( context->functionList );
	
	// Searching through variables
	for ( size_t i = 0; i < varCount; i++ ) {
		const CkVariable *pVar = CkListAccess( context->variableList, i );
		if ( !strcmp( pVar->name, symbol ) )
			return pVar->type;
	}

	// Searching through functions
	for ( size_t i = 0; i < funcCount; i++ ) {
		const CkFunction *pFunc = CkListAccess( context->functionList, i );
		/*
			It would be wrong to assume that we want to return type of the
			function. We actually want its signature, because we have no idea
			if this is a return type. We simply want to know whats the function's
			signature.
		*/
		if ( !strcmp( pFunc->name, symbol ) )
			return pFunc->signature;
	}

	// Search in parent (if parent exists)
	if ( context->parent )
		return s_TryGetSymbolType( symbol, context->parent );

	// Symbol not found
	return NULL;
}

static inline void s_GetSizeFoodNativeT( CkToken *dest, CkFoodType *T )
{
	switch ( T->id ) {
	case CK_FOOD_BOOL:
	case CK_FOOD_I8:
	case CK_FOOD_U8:
	case CK_FOOD_VOID:
		dest->value.u64 = 1;
		break;

	case CK_FOOD_I16:
	case CK_FOOD_U16:
	case CK_FOOD_F16:
		dest->value.u64 = 2;
		break;

	case CK_FOOD_I32:
	case CK_FOOD_U32:
	case CK_FOOD_F32:
		dest->value.u64 = 4;
		break;

	case CK_FOOD_I64:
	case CK_FOOD_U64:
	case CK_FOOD_F64:
	case CK_FOOD_POINTER:
	case CK_FOOD_REFERENCE:
	case CK_FOOD_FUNCPOINTER:
		dest->value.u64 = 8;
		break;
	}
}

static inline void s_GetAlignFoodNativeT( CkToken *dest, CkFoodType *T )
{
	printf( "alignof() is currently unsupported, alignof(T) = 0\n" );
	dest->value.u64 = 0;
	break;
}

static inline FoodTypeID s_GetFloatTContainsIntU( FoodTypeID U )
{
	switch ( U ) {
	case CK_FOOD_I8:
	case CK_FOOD_U8:
	case CK_FOOD_I16:
	case CK_FOOD_U16:
		return CK_FOOD_F16;

	case CK_FOOD_I32:
	case CK_FOOD_U32:
		return CK_FOOD_F32;

	case CK_FOOD_I64:
	case CK_FOOD_U64:
		return CK_FOOD_F64;

	default:
		printf( "ck internal error: attempted to get float equivalent for non-integer type\n" );
		abort();
	}
}

static CkExpression *s_ValidateExpression(
	CkScope *scope,
	CkDiagnosticHandlerInstance *pDhi,
	CkArenaFrame *allocator,
	CkExpression *expression )
{
	CkExpression *left = expression->left ? s_ValidateExpression( allocator, expression->left ) : NULL;
	CkExpression *right = expression->right ? s_ValidateExpression( allocator, expression->right ) : NULL;
	CkExpression *extra = expression->extra ? s_ValidateExpression( allocator, expression->extra ) : NULL;

	switch ( expression->kind ) {
		// Literals have their types figured out at parser-level
	case CK_EXPRESSION_INTEGER_LITERAL:
	case CK_EXPRESSION_FLOAT_LITERAL:
	case CK_EXPRESSION_BOOL_LITERAL:
	case CK_EXPRESSION_STRING_LITERAL:
		return CkExpressionDuplicate( allocator, expression );

		// An identifier
	case CK_EXPRESSION_IDENTIFIER:
	{
		CkFoodType *symType;
		CkExpression *result;
		symType = s_TryGetSymbolType( expression->token.value.cstr, scope );
		if ( !symType )
			result = CkExpressionCreateLiteral(
				allocator,
				&expression->token,
				CK_EXPRESSION_IDENTIFIER,
				CkFoodCopyTypeInstance(allocator, symType)
			);
		else {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The symbol '%s' cannot be found in this scope.", expression->token.value.cstr );
			result = CkExpressionCreateLiteral( // int type by default. Still shouldn't compile.
					allocator,
					&expression->token,
					CK_EXPRESSION_IDENTIFIER,
					CkFoodCreateTypeInstance( allocator, CK_FOOD_I32, 0, NULL ) );
		}
		result->isLValue = TRUE;
		return result;
	}

		// sizeof() and alignof()
	case CK_EXPRESSION_SIZEOF:
	case CK_EXPRESSION_ALIGNOF:
	{
		CkFoodType *refT;     // Type to get the size or alignment of
		CkToken literal = {}; // Literal buffer
		CK_ASSERT( left );

		refT = left->type;
		literal.kind = '0';
		literal.position = expression->token.position;

		// User type
		if ( refT == NULL ) {
			// TODO: User types. Requires alignment constraints
		} else {
			if ( expression->kind == CK_EXPRESSION_SIZEOF )
				s_GetSizeFoodNativeT( &literal, refT );
			else s_GetAlignFoodNativeT( &literal, refT );
		}

		return CkExpressionCreateLiteral(
			allocator,
			&literal,
			CK_EXPRESSION_INTEGER_LITERAL,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_U64, 0, NULL )
		);
	}

		// Addition and subtraction
	case CK_EXPRESSION_ADD:
	case CK_EXPRESSION_SUB:
	{
		FoodTypeID newTypeID = CK_FOOD_VOID;
		CK_ASSERT( left );
		CK_ASSERT( right );

		/*
			Allowed cases:
			int + int     => int   :: Return biggest integer type.
			int + float   => float :: Return smallest integer type that can store an integer. Minimum the size of the float.
			float + float => float :: Return biggest float type.
			ptr + int     => ptr   :: Return same pointer.
		*/

		if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) ) // int + int => int
			newTypeID = max( left->type->id, right->type->id );
		else if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_FLOAT( right->type->id ) ) // int + float => float
			newTypeID = max( s_GetFloatTContainsIntU( left->type->id ), right->type->id );
		else if ( CK_TYPE_CLASSED_FLOAT( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) ) // float + int => float
			newTypeID = max( left->type->id, s_GetFloatTContainsIntU( right->type->id ) );
		else if ( CK_TYPE_CLASSED_INT( left->type->id ) && right->type->id == CK_FOOD_POINTER ) // ptr + int => ptr
			newTypeID = right->type->id;
		else if ( left->type->id == CK_FOOD_POINTER && CK_TYPE_CLASSED_INT( right->type->id ) ) // int + ptr => ptr
			newTypeID = left->type->id;
		else {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Addition and subtraction require arithmetic types on both operands. A pointer type is allowed on one of the two operands." );
		}

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, NULL ),
			left,
			right
		);
	}

		// Multiplication and division
	case CK_EXPRESSION_MUL:
	case CK_EXPRESSION_DIV:
	{
		FoodTypeID newTypeID = CK_FOOD_VOID;
		CK_ASSERT( left );
		CK_ASSERT( right );

		/*
			Allowed cases:
			int * int     => int   :: Return biggest integer type.
			int * float   => float :: Return smallest integer type that can store an integer. Minimum the size of the float.
			float * float => float :: Return biggest float type.
		*/

		if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) ) // int * int => int
			newTypeID = max( left->type->id, right->type->id );
		else if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_FLOAT( right->type->id ) ) // int * float => float
			newTypeID = max( s_GetFloatTContainsIntU( left->type->id ), right->type->id );
		else if ( CK_TYPE_CLASSED_FLOAT( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) ) // float * int => float
			newTypeID = max( left->type->id, s_GetFloatTContainsIntU( right->type->id ) );
		else {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Multiplication and division require arithmetic types on both operands." );
		}
	}

		// Modulo
	case CK_EXPRESSION_MOD:
	{
		FoodTypeID newTypeID = CK_FOOD_VOID;
		CK_ASSERT( left );
		CK_ASSERT( right );

		/*
			Allowed cases:
			int % int => int :: Return biggest integer type
		*/
		if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) )
			newTypeID = max( left->type->id, right->type->id );
		else {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Modulo requires both its operands to be of integer type." );
		}
	}

	}
}

static void s_ValidateFunc( CkArenaFrame *allocator, CkFunction *func )
{
	if ( func->body->stmt == CK_STMT_EXPRESSION )
		func->body->data.expression = s_ValidateExpression( allocator, func->body->data.expression );
	else { // Block

	}
}

void CkBinderValidateAndBind( CkArenaFrame *allocator, CkLibrary *library )
{
	size_t moduleCount;
	size_t glblFuncCount;

	CK_ARG_NON_NULL( allocator );
	CK_ARG_NON_NULL( library );

	moduleCount = CkListLength( library->moduleList );
	glblFuncCount = CkListLength( library->scope->functionList );
	
	// Modules
	for ( size_t i = 0; i < moduleCount; i++ ) {
		const CkModule *module = *(CkModule **)CkListAccess( library->moduleList, i );
		const size_t funcCount = CkListLength( module->scope->functionList );
		for ( size_t i = 0; i < funcCount; i++ )
			s_ValidateFunc( allocator, CkListAccess( module->scope->functionList, i ) );
	}

	// Global functions
	for ( size_t i = 0; i < glblFuncCount; i++ )
		s_ValidateFunc( allocator, CkListAccess( library->scope->functionList, i ) );
}
