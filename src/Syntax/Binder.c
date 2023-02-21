#include <Syntax/Binder.h>
#include <Syntax/Expression.h>
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
	CkExpression *left = expression->left ? s_ValidateExpression( scope, pDhi, allocator, expression->left ) : NULL;
	CkExpression *right = expression->right ? s_ValidateExpression( scope, pDhi, allocator, expression->right ) : NULL;
	CkExpression *extra = expression->extra ? s_ValidateExpression( scope, pDhi, allocator, expression->extra ) : NULL;

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
				CkFoodCopyTypeInstance( allocator, symType )
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
		CkFoodType *subtype = NULL;
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
		else if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_POINTER_ARITHM( right->type->id ) ) { // ptr + int => ptr
			newTypeID = right->type->id;
			subtype = right->type->child;
		} else if ( CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) ) {// int + ptr => ptr
			newTypeID = left->type->id;
			subtype = left->type->child;
		} else {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Addition and subtraction require arithmetic types on both operands. A pointer type is allowed on one of the two operands." );
		}

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, CkFoodCopyTypeInstance( subtype ) ),
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

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, NULL ),
			left,
			right
		);
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

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, NULL ),
			left,
			right
		);
	}

	// x++, x--, ++x, --x
	case CK_EXPRESSION_POSTFIX_INC:
	case CK_EXPRESSION_POSTFIX_DEC:
	case CK_EXPRESSION_PREFIX_INC:
	case CK_EXPRESSION_PREFIX_DEC:
		/*
			Allowed cases:
			int++ => int :: Return same type
			ptr++ => ptr :: Return same type
		*/
		CK_ASSERT( left );
		if ( !(CK_TYPE_CLASSED_INT( left->type->id ) || CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id )) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Postfix/prefix increment or decrement operators require an integer or pointer operand." );
		}

		if ( !left->isLValue ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Postfix/prefix increment or decrement operators require an lvalue operand." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type ),
			left
		);

		// +x, -x
	case CK_EXPRESSION_UNARY_PLUS:
	case CK_EXPRESSION_UNARY_MINUS:
		/*
			Allowed cases:
			+int => int :: Return same type
			+float => float :: Return same type

			Food doesn't allow pointers for the +x and -x operators,
			as opposed to other languages (e.g. C)
		*/

		CK_ASSERT( left );
		if ( !CK_TYPE_CLASSED_INTFLOAT( left->type->id ) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The unary plus and minus operators require an operand of integer or float type." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type ),
			left
		);

		// ~x
	case CK_EXPRESSION_BITWISE_NOT:
		/*
			Allowed cases:
			~int => int :: Return same type
		*/

		CK_ASSERT( left );
		if ( !CK_TYPE_CLASSED_INT( left->type->id ) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The bitwise not operator (~x) requires an integer operand." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type ),
			left
		);

		// !x
	case CK_EXPRESSION_LOGICAL_NOT:
		/*
			Allowed cases:
			!int =>  bool
			!bool => bool
			!ptr =>  bool
		*/

		CK_ASSERT( left );
		if ( !(CK_TYPE_CLASSED_INT( left->type->id )
			|| CK_TYPE_CLASSED_POINTER( left->type->id )
			|| left->type->id == CK_FOOD_BOOL) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The logical not operator (!x) requires an integer, pointer or boolean operand." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left
		);

		// *x
	case CK_EXPRESSION_DEREFERENCE:
	{
		/*
			Allowed cases:
			*(T *) => T
			*(T &) => T
			*(T[]) => T
		*/

		CkExpression *result;

		CK_ASSERT( left );
		if (
			left->type->id == CK_FOOD_POINTER
			&& left->type->id == CK_FOOD_REFERENCE
			&& left->type->id == CK_FOOD_ARRAY ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Only a pointer, reference or array may be dereferenced." );
			return CkExpressionCreateUnary( // Avoiding NULL dereference + attempt to maintain order in error messages
				allocator,
				&expression->token,
				expression->kind,
				CkFoodCopyTypeInstance( allocator, left->type ),
				left
			);
		}

		result = CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type->child ),
			left
		);

		result->isLValue = TRUE;
		return result;
	}

	// Address-Of
	case CK_EXPRESSION_ADDRESS_OF:
		/*
			The address-of operator has a few constraints that need
			to be noted:
			  1) The operand must be an lvalue (must be addressable)
			  2) The operand must not be a reference
			  3) Its result is **always** a reference, and will be
				 casted to a pointer if required. This trick is
				 because the binder is uni-directional.

			&T => T& :: Always reference type.
		*/

		CK_ASSERT( left );
		if ( !left->isLValue ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"It is impossible to take the address of a non-lvalue object." );
		}

		if ( left->type->id == CK_FOOD_REFERENCE ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"It is impossible to take the address of a reference." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance(
				allocator,
				CK_FOOD_REFERENCE,
				0,
				CkFoodCopyTypeInstance( allocator, left->type )
			),
			left
		);

		// &&x
	case CK_EXPRESSION_OPAQUE_ADDRESS_OF:
		/*
			The opaque address-of operator is very similar to the classic
			address-of operator, however it will allows return an opaque
			reference. It also requires its operand to be an lvalue and
			not a reference.
		*/

		if ( !left->isLValue ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"It is impossible to take the opaque address of a non-lvalue object." );
		}

		if ( left->type->id == CK_FOOD_REFERENCE ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"It is impossible to take the opaque address of a reference." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance(
				allocator,
				CK_FOOD_POINTER,
				0,
				CkFoodCreateTypeInstance( allocator, CK_FOOD_VOID, 0, NULL )
			),
			left
		);

		// Bitwise shifts
	case CK_EXPRESSION_LEFT_SHIFT:
	case CK_EXPRESSION_RIGHT_SHIFT:
		/*
			Allowed cases:
			int << int :: Left type
			ptr << int :: Left type
			Note: The pointer can only be on the left side of the shift
			expression to prevent bugs and ridiculous shifting.
		*/

		CK_ASSERT( left );
		CK_ASSERT( right );

		if ( !CK_TYPE_CLASSED_INT( right->type->id ) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The right operand of a bitwise shift must be of integer type." );
		}

		if ( !CK_TYPE_CLASSED_INT( left->type->id ) && !CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id ) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The left operand of a bitwise shift must be an integer or an arithmetic-capable pointer." );
		}

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type ),
			left,
			right
		);

		// <, <=, >, >=
	case CK_EXPRESSION_LOWER:
	case CK_EXPRESSION_LOWER_EQUAL:
	case CK_EXPRESSION_GREATER:
	case CK_EXPRESSION_GREATER_EQUAL:
		/*
			Allowed cases:
			int < int     => bool
			int < float   => bool
			float < float => bool
			T* < T*       => bool
		*/

		CK_ASSERT( left );
		CK_ASSERT( right );
		if ( CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id ) && CK_TYPE_CLASSED_POINTER( right->type->id ) ) {
			if ( left->type->child->id != right->type->child->id ) {
				CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Two pointers of non-equal subtypes cannot be compared, even if one of them is an opaque pointer." );
			}
		} else if ( !CK_TYPE_CLASSED_INTFLOAT( left->type->id ) && !CK_TYPE_CLASSED_INTFLOAT( right->type->id ) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The operands of a comparison must either be two pointers with the same subtype, integers or floats." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left
		);

		// == and !=
	case CK_EXPRESSION_EQUAL:
	case CK_EXPRESSION_NOT_EQUAL:
		/*
			Valid cases of equality comparison:
			  1) Both types are arithmetic types or pointers.
			  2) Both types are references (same-reference comparison)
			  3) Both types are equal (applies to user types.)

			bool is always the returned type.
		*/

		CK_ASSERT( left );
		CK_ASSERT( right );

		if ( (left->type->id == right->type->id)
			&& (
				left->type->id == CK_FOOD_STRUCT
				|| left->type->id == CK_FOOD_UNION) ) {
			// TODO: Support user types
			printf( "user types equality validity unsupported; defaults to valid\n" );
		} else if ( (left->type->id == right->type->id) && left->type->id == CK_FOOD_REFERENCE ) {
			if ( left->type->child->id != right->type->child->id ) {
				CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Two references cannot be compared if they don't have the same subtype. "
					"If you wish to compare two references that don't have the same subtype, use pointers." );
			}
		} else if ( (CK_TYPE_CLASSED_INTFLOAT( left->type->id ) || CK_TYPE_CLASSED_POINTER( left->type->id ))
			&& (CK_TYPE_CLASSED_INTFLOAT( right->type->id ) || CK_TYPE_CLASSED_POINTER( left->type->id )) ) {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Equality comparison requires two identical types for user-types, or two arithmetic types or pointers." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left
		);

		// Classical bitwise operators (not shifts)
	case CK_EXPRESSION_BITWISE_AND:
	case CK_EXPRESSION_BITWISE_OR:
	case CK_EXPRESSION_BITWISE_XOR:
	{
		FoodTypeID newTypeID = CK_FOOD_VOID;
		CkFoodType *subtype = NULL;
		/*
			Allowed cases:
			int & int => int :: Biggest integer
			ptr & int => ptr :: Same pointer type (ptr and int can be interchanged)
		*/

		CK_ASSERT( left );
		CK_ASSERT( right );

		if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) )
			newTypeID = max( left->type->id, right->type->id );
		else if ( CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id ) && CK_TYPE_CLASSED_INT( right->type->id ) ) {
			newTypeID = left->type->id;
			subtype = left->type->child;
		} else if ( CK_TYPE_CLASSED_INT( left->type->id ) && CK_TYPE_CLASSED_POINTER( right->type->id ) ) {
			newTypeID = right->type->id;
			subtype = right->type->child;
		} else {
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The classic bitwise operators (&, |, ^) require their operands to be integer, with one operand allowed to be a pointer." );
		}

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, CkFoodCopyTypeInstance(subtype) ),
			left
		);
	}

	// && and ||
	case CK_EXPRESSION_LOGICAL_AND:
	case CK_EXPRESSION_LOGICAL_OR:
		/*
			Both operands must be of boolean, integer or pointer type.
			Always returns bool.
		*/

		CK_ASSERT( left );
		CK_ASSERT( right );

		if ( !(CK_TYPE_CLASSED_INT( left->type->id )
			|| CK_TYPE_CLASSED_POINTER( left->type->id )
			|| left->type->id == CK_FOOD_BOOL) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The left operand of a logical operator (&&, ||) must be either a boolean, an integer or a pointer." );
		if ( !(CK_TYPE_CLASSED_INT( right->type->id )
			|| CK_TYPE_CLASSED_POINTER( right->type->id )
			|| right->type->id == CK_FOOD_BOOL) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The right operand of a logical operator (&&, ||) must be either a boolean, an integer or a pointer." );

		return CkExpressionCreateUnary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left
		);

		// ?:
	case CK_EXPRESSION_CONDITIONAL:
	{
		FoodTypeID newTypeID = CK_FOOD_VOID;
		CkFoodType *subtype = NULL;

		CK_ASSERT( left );
		CK_ASSERT( right );
		CK_ASSERT( extra );

		if ( !(CK_TYPE_CLASSED_INT( extra->type->id ) || CK_TYPE_CLASSED_POINTER( extra->type->id ) || extra->type->id == CK_FOOD_POINTER) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The condition of a conditional statement (a in a ? b : c) must be an integer, a pointer or a boolean." );
	}
	}
}

static void s_ValidateFunc( CkScope *scope, CkDiagnosticHandlerInstance *pDhi, CkArenaFrame *allocator, CkFunction *func )
{
	if ( func->body->stmt == CK_STMT_EXPRESSION )
		func->body->data.expression = s_ValidateExpression( scope, pDhi, allocator, func->body->data.expression );
	else { // Block

	}
}

void CkBinderValidateAndBind( CkDiagnosticHandlerInstance *pDhi, CkArenaFrame *allocator, CkLibrary *library )
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
			s_ValidateFunc( module->scope, pDhi, allocator, CkListAccess( module->scope->functionList, i ) );
	}

	// Global functions
	for ( size_t i = 0; i < glblFuncCount; i++ )
		s_ValidateFunc( library->scope, pDhi, allocator, CkListAccess( library->scope->functionList, i ) );
}
