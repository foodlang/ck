#include <Syntax/Binder.h>
#include <Syntax/Expression.h>
#include <CDebug.h>
#include <Food.h>

#include <string.h>

#ifndef max
#define max(a, b) ((a) < (b) ? (b) : (a))
#endif

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

// Attempts to get a user type definition.
static const CkUserType *s_GetUserType( char *symname, CkScope *scope )
{
	/*
		TODO:
		Add support for scope reference operator (::)

		Probably using paths or something? I don't know,
		but the system needs to be rethinked entierely.
	*/

	size_t userTypeCount; // User type scope count

	/*
		Checking if scope has function / usertype support.
		This is optional, but it prevents probing around for the 
		length when its zero. Micro-optimization, I guess.
		Eh its four lines and its not that big of a deal.
	*/
	if ( !scope->supportsFunctions ) {
		if (scope->parent )
			return s_GetUserType( symname, scope->parent );
		return NULL;
	}

	// Checking in the scope user type lists
	userTypeCount = CkListLength( scope->usertypeList );
	for ( size_t i = 0; i < userTypeCount; i++ ) {
		const CkUserType *pCurrent = CkListAccess( scope->usertypeList, i );
		if ( !strcmp( symname, pCurrent->name ) )
			return pCurrent;
	}

	// Checking in parent if not found in this scope.
	if ( scope->parent )
		return s_GetUserType( symname, scope->parent );

	return NULL;
}

// Get the size of a Food primitive. Food defines all primitives' sizes by default.
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
	default: abort();
	}
}

// Get the alignment requirement of a Food alignment.
static inline void s_GetAlignFoodNativeT( CkToken *dest, CkFoodType *T )
{
	printf( "alignof() is currently unsupported, alignof(T) = 0\n" );
	dest->value.u64 = 0;
}

// Finds the best float type to store an integer of a given type.
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

// Returns true if the type is usable as a boolean.
static bool_t s_BooleanType( FoodTypeID typeID )
{
	return (CK_TYPE_CLASSED_INT( typeID ) || CK_TYPE_CLASSED_POINTER( typeID ) || typeID == CK_FOOD_BOOL);
}

static bool_t s_CompatibleTypesCheck( CkFoodType *left, CkFoodType *right, CkScope *scope );

// Compares two user types' signatures.
static bool_t s_CompareUserTypesSignature( CkFoodType *left, CkFoodType *right, CkScope *scope )
{
	size_t leftMemberCount = 0;                                // Left member count
	size_t rightMemberCount = 0;                               // Right member count
	const CkUserType *leftT = s_GetUserType( left->extra, scope );   // The left user type.
	const CkUserType *rightT = s_GetUserType( right->extra, scope ); // The right user type.

	// The two types must be of compatible user type kind.
	if ( leftT->kind != rightT->kind )
		return FALSE;

	// Enums are all compatible by default.
	if ( leftT->kind == CK_USERTYPE_ENUM )
		return TRUE;

	// Records and unions use the same struct
	leftMemberCount = CkListLength( leftT->custom.struct_.members );
	rightMemberCount = CkListLength( rightT->custom.struct_.members );

	if ( leftMemberCount != rightMemberCount )
		return FALSE;

	for ( size_t i = 0; i < leftMemberCount; i++ ) {
		const CkVariable *leftM = CkListAccess( leftT->custom.struct_.members, i );
		const CkVariable *rightM = CkListAccess( rightT->custom.struct_.members, i );

		if ( !s_CompatibleTypesCheck( leftM->type, rightM->type, scope ) ) return FALSE;
	}
	return TRUE;
}

// Compares two types. This will return true if they are practically the same, but some attributes or qualifiers are different.
static bool_t s_CompatibleTypesCheck( CkFoodType *left, CkFoodType *right, CkScope *scope )
{
	// References to pointers is allowed
	if ( (left->id == CK_FOOD_REFERENCE && right->id == CK_FOOD_POINTER)
		|| (left->id == CK_FOOD_POINTER && right->id == CK_FOOD_REFERENCE) )
		return s_CompatibleTypesCheck( left->child, right->child, scope );

	// Only user types can be compared with other user types...
	if ( (left->id == CK_FOOD_STRUCT && right->id == CK_FOOD_STRUCT)
		|| (left->id == CK_FOOD_UNION && right->id == CK_FOOD_UNION) )
		return s_CompareUserTypesSignature( left, right, scope );

	if ( CK_TYPE_CLASSED_INTFLOAT( left->id ) && CK_TYPE_CLASSED_INTFLOAT( right->id ) ) return TRUE;
	if ( CK_TYPE_CLASSED_INT( left->id ) && CK_TYPE_CLASSED_POINTER_ARITHM( right->id ) ) return TRUE;
	if ( CK_TYPE_CLASSED_POINTER_ARITHM( left->id ) && CK_TYPE_CLASSED_INT( right->id ) ) return TRUE;

	return FALSE;
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
	case CK_EXPRESSION_TYPE:
	case CK_EXPRESSION_COMPOUND_LITERAL:
		return CkExpressionDuplicate(allocator, expression);

		// TODO: Scoped references
	case CK_EXPRESSION_SCOPED_REFERENCE:
		abort();

		// An identifier
	case CK_EXPRESSION_IDENTIFIER:
	{
		CkExpression *result;
		const CkFoodType *symType = s_TryGetSymbolType( expression->token.value.cstr, scope );
		if ( symType )
			result = CkExpressionCreateLiteral(
				allocator,
				&expression->token,
				CK_EXPRESSION_IDENTIFIER,
				CkFoodCopyTypeInstance( allocator, (CkFoodType *)symType )
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
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, subtype != NULL ? CkFoodCopyTypeInstance( allocator, subtype ) : NULL ),
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
		if ( !s_BooleanType( extra->type->id ) ) {
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
			left->type->id != CK_FOOD_POINTER
			&& left->type->id != CK_FOOD_REFERENCE
			&& left->type->id != CK_FOOD_ARRAY ) {
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

		CK_ASSERT( left );
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

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left,
			right
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

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left,
			right
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

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, newTypeID, 0, subtype != NULL ? CkFoodCopyTypeInstance( allocator, subtype ) : NULL ),
			left,
			right
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

		if ( !s_BooleanType( left->type->id ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The left operand of a logical operator (&&, ||) must be either a boolean, an integer or a pointer." );
		if ( !s_BooleanType( right->type->id ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The right operand of a logical operator (&&, ||) must be either a boolean, an integer or a pointer." );

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCreateTypeInstance( allocator, CK_FOOD_BOOL, 0, NULL ),
			left,
			right
		);

		// x => T, (T)x
	case CK_EXPRESSION_C_CAST:
	case CK_EXPRESSION_FOOD_CAST:
		CK_ASSERT( left );
		CK_ASSERT( right );

		// (T&)expr is not allowed
		if ( expression->type->id == CK_FOOD_REFERENCE )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Casting to a reference is not allowed." );

		// C-style discard
		if ( expression->type->id == CK_FOOD_VOID )
			return CkExpressionDuplicate(allocator, expression);

		if ( expression->type->id == CK_FOOD_STRUCT
			|| expression->type->id == CK_FOOD_UNION)
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The result of a cast must be of scalar type." );

		if ( left->type->id == CK_FOOD_STRUCT
			|| left->type->id == CK_FOOD_UNION)
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The input of a cast must be of scalar type." );

		return CkExpressionDuplicate(allocator, expression);

		// Classic assignment
	case CK_EXPRESSION_ASSIGN:
	case CK_EXPRESSION_ASSIGN_SUM:
	case CK_EXPRESSION_ASSIGN_DIFF:
	case CK_EXPRESSION_ASSIGN_PRODUCT:
	case CK_EXPRESSION_ASSIGN_QUOTIENT:
	case CK_EXPRESSION_ASSIGN_REMAINDER:
	case CK_EXPRESSION_ASSIGN_LEFT_SHIFT:
	case CK_EXPRESSION_ASSIGN_RIGHT_SHIFT:
	case CK_EXPRESSION_ASSIGN_AND:
	case CK_EXPRESSION_ASSIGN_XOR:
	case CK_EXPRESSION_ASSIGN_OR:
		CK_ASSERT( left );
		CK_ASSERT( right );

		if ( !s_CompatibleTypesCheck(left->type, right->type, scope) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"An assignment requires compatible value types." );

		if ( (expression->kind == CK_EXPRESSION_ASSIGN_SUM || expression->kind == CK_EXPRESSION_ASSIGN_DIFF)
			&& ( !CK_TYPE_CLASSED_INTFLOAT( left->type->id ) && !CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id ) ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"You can only increment or decrement a value of arithmetic type." );

		if ( (expression->kind == CK_EXPRESSION_ASSIGN_PRODUCT || expression->kind == CK_EXPRESSION_ASSIGN_QUOTIENT)
			&& ( !CK_TYPE_CLASSED_INTFLOAT( left->type->id ) ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"You can only multiply or divide a value of arithmetic type." );

		if ( ( expression->kind == CK_EXPRESSION_MOD )
			&& (!CK_TYPE_CLASSED_INT( left->type->id ) || !CK_TYPE_CLASSED_INT( right->type->id ) ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The modulo assignment operator requires both of its operands to be integers." );

		if ( (expression->kind == CK_EXPRESSION_LEFT_SHIFT
			|| expression->kind == CK_EXPRESSION_RIGHT_SHIFT
			|| expression->kind == CK_EXPRESSION_ASSIGN_AND
			|| expression->kind == CK_EXPRESSION_ASSIGN_XOR
			|| expression->kind == CK_EXPRESSION_ASSIGN_OR)
			&& (
				( !CK_TYPE_CLASSED_INT( left->type->id )
				&& !CK_TYPE_CLASSED_POINTER_ARITHM( left->type->id ) )
				|| !CK_TYPE_CLASSED_INT( right->type->id )) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Incorrect usage of bitwise operator." );

		if ( !left->isLValue )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Only an lvalue can be assigned a value." );

		return CkExpressionDuplicate(allocator, expression);

		// x[y]
	case CK_EXPRESSION_SUBSCRIPT:
		CK_ASSERT( left );
		CK_ASSERT( right );

		if ( !CK_TYPE_CLASSED_POINTER_ARITHM( left->kind ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"You can only index via subscript an arithmetic pointer." );

		if ( !CK_TYPE_CLASSED_INT( right->kind ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The index of the subscript operator must be an integer." );

		if ( left->type->child->id == CK_FOOD_VOID )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot index a void pointer via subscript." );

		return CkExpressionCreateBinary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type->child ),
			left,
			right
		);

		// x, y
	case CK_EXPRESSION_COMPOUND:
		CK_ASSERT( left );
		CK_ASSERT( right );
		return CkExpressionDuplicate(allocator, expression);

		// ?:
	case CK_EXPRESSION_CONDITIONAL:
		CK_ASSERT( left );
		CK_ASSERT( right );
		CK_ASSERT( extra );

		if ( !s_BooleanType( extra->type->id ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The condition of a conditional statement (a in a ? b : c) must be an integer, a pointer or a boolean." );

		if ( !s_CompatibleTypesCheck( left->type, right->type, scope ) )
			CkDiagnosticThrow( pDhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The two operands of the conditional statement must be of practical equality/be compatible." );

		return CkExpressionCreateTernary(
			allocator,
			&expression->token,
			expression->kind,
			CkFoodCopyTypeInstance( allocator, left->type ), // TODO: Lowest common denominator??
			left,
			right,
			extra
		);

	case CK_EXPRESSION_DUMMY:
	case CK_EXPRESSION_NAMEOF:
	case CK_EXPRESSION_TYPEOF:
	case CK_EXPRESSION_MEMBER_ACCESS:
	case CK_EXPRESSION_POINTER_MEMBER_ACCESS:
	case CK_EXPRESSION_FUNCCALL:
		printf( "ck internal error: Missing type binder for operator %d.\n", expression->kind );
		abort();
	}
}

static void s_ValidateBlock( CkDiagnosticHandlerInstance *pDhi, CkArenaFrame *allocator, CkStatement *blk );

static void s_ValidateStmt( CkDiagnosticHandlerInstance *pDhi, CkArenaFrame *allocator, CkStatement *stmt, CkScope *scope )
{
	switch ( stmt->stmt ) {
	case CK_STMT_EMPTY: break;
	case CK_STMT_BLOCK: s_ValidateBlock( pDhi, allocator, stmt ); break;
	case CK_STMT_BREAK: break; // Handled at generation
	case CK_STMT_CONTINUE: break; // Handled at generation
	case CK_STMT_EXPRESSION: stmt->data.expression = s_ValidateExpression( scope, pDhi, allocator, stmt->data.expression ); break;
	case CK_STMT_SPONGE: s_ValidateStmt( pDhi, allocator, stmt->data.sponge.statement, scope );
	case CK_STMT_ASSERT:
		stmt->data.assert.expression = s_ValidateExpression( scope, pDhi, allocator, stmt->data.expression );
		if ( !s_BooleanType(stmt->data.assert.expression->type->id) )
			CkDiagnosticThrow( pDhi, stmt->data.assert.expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Assert requires a boolean condition." );
		break;

	case CK_STMT_IF:
		stmt->data.if_.condition = s_ValidateExpression( scope, pDhi, allocator, stmt->data.expression );
		if ( !s_BooleanType( stmt->data.if_.condition->type->id ) )
			CkDiagnosticThrow( pDhi, stmt->data.if_.condition->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"If requires a boolean condition." );
		s_ValidateStmt( pDhi, allocator, stmt->data.if_.cThen, scope );
		if (stmt->data.if_.cElse) s_ValidateStmt( pDhi, allocator, stmt->data.if_.cElse, scope );
		break;

	case CK_STMT_WHILE:
	case CK_STMT_DO_WHILE:
		stmt->data.while_.condition = s_ValidateExpression( scope, pDhi, allocator, stmt->data.expression );
		if ( !s_BooleanType( stmt->data.while_.condition->type->id ) )
			CkDiagnosticThrow( pDhi, stmt->data.while_.condition->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"If requires a boolean condition." );
		s_ValidateStmt( pDhi, allocator, stmt->data.while_.cWhile, scope );
		break;

	case CK_STMT_FOR:
		s_ValidateStmt( pDhi, allocator, stmt->data.for_.cInit, stmt->data.for_.scope );
		stmt->data.for_.condition = s_ValidateExpression( stmt->data.for_.scope, pDhi, allocator, stmt->data.for_.condition );
		stmt->data.for_.lead = s_ValidateExpression( stmt->data.for_.scope, pDhi, allocator, stmt->data.for_.lead );

		if ( !s_BooleanType(stmt->data.for_.condition->type->id) )
			CkDiagnosticThrow( pDhi, stmt->data.while_.condition->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"For's condition must be a boolean." );

		s_ValidateStmt( pDhi, allocator, stmt->data.for_.body, stmt->data.for_.scope );
		break;

	case CK_STMT_GOTO:
		if ( stmt->data.goto_.computed ) {
			stmt->data.goto_.computedExpression = s_ValidateExpression( scope, pDhi, allocator, stmt->data.goto_.computedExpression );
			if ( stmt->data.goto_.computedExpression->type->id == CK_FOOD_POINTER && stmt->data.goto_.computedExpression->type->child->id == CK_FOOD_VOID )
				CkDiagnosticThrow( pDhi, stmt->data.while_.condition->token.position, CK_DIAG_SEVERITY_ERROR, "",
					"Computed goto requires a void pointer operand." );
		}
		break;

	case CK_STMT_SWITCH:
		break; // not yet supported
	}
}

static void s_ValidateBlock( CkDiagnosticHandlerInstance *pDhi, CkArenaFrame *allocator, CkStatement *blk )
{
	const size_t elemCount = CkListLength( blk->data.block.stmts );
	for ( size_t i = 0; i < elemCount; i++ ) {
		const CkStatement *stmt = *(CkStatement **)CkListAccess( blk->data.block.stmts, i );
		s_ValidateStmt( pDhi, allocator, (CkStatement *)stmt, blk->data.block.scope );
	}
}

static void s_ValidateFunc( CkScope *scope, CkDiagnosticHandlerInstance *pDhi, CkArenaFrame *allocator, CkFunction *func )
{
	CK_ARG_NON_NULL( func );
	CK_ARG_NON_NULL( func->body );
	if ( func->body->stmt == CK_STMT_EXPRESSION )
		func->body->data.expression = s_ValidateExpression( scope, pDhi, allocator, func->body->data.expression );
	else s_ValidateBlock( pDhi, allocator, func->body );
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
