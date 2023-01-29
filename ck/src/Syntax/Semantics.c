#include <include/Syntax/Semantics.h>
#include <include/CDebug.h>

#include <assert.h>

#define TYPE_CLASSED_INT(x) ((x) == CK_FOOD_I8 || (x) == CK_FOOD_U8 || (x) == CK_FOOD_I16 || (x) == CK_FOOD_U16 || \
							 (x) == CK_FOOD_I32 || (x) == CK_FOOD_U32 || (x) == CK_FOOD_I64 || (x) == CK_FOOD_U64 || \
							 (x) == CK_FOOD_ENUM)

#define TYPE_CLASSED_FLOAT(x) ((x) == CK_FOOD_F16 || (x) == CK_FOOD_F32 || (x) == CK_FOOD_F64)

#define TYPE_CLASSED_INTFLOAT(x) ((x) >= CK_FOOD_I8 && (x) <= CK_FOOD_F64)

#define TYPE_CLASSED_POINTER(x) ((x) == CK_FOOD_POINTER || (x) == CK_FOOD_FUNCPOINTER)

CkExpression *CkSemanticsProcessExpression(
	CkDiagnosticHandlerInstance *dhi,
	CkArenaFrame *outputArena,
	const CkExpression *expression)
{
	CkExpression *newLeft = NULL;
	CkExpression *newRight = NULL;
	CkExpression *newExtra = NULL;
	CkExpression *new;
	uint8_t lType;
	uint8_t rType;
	uint8_t eType;

	CK_ARG_NON_NULL(dhi);
	CK_ARG_NON_NULL(outputArena);
	CK_ARG_NON_NULL(expression);

	if (expression->left) {
		if (expression->left->kind == CK_EXPRESSION_DUMMY) {
			CkDiagnosticThrow(dhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot type an expression that didn't parse properly.");
			return CkExpressionCreateLiteral(outputArena, &expression->token, CK_EXPRESSION_DUMMY, NULL);
		}
		newLeft = CkSemanticsProcessExpression(dhi, outputArena, expression->left);
	}

	if(expression->right) {
		if (expression->right->kind == CK_EXPRESSION_DUMMY) {
			CkDiagnosticThrow(dhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot type an expression that didn't parse properly.");
			return CkExpressionCreateLiteral(outputArena, &expression->token, CK_EXPRESSION_DUMMY, NULL);
		}
		newRight = CkSemanticsProcessExpression(dhi, outputArena, expression->right);
	}

	if (expression->extra) {
		if (expression->extra->kind == CK_EXPRESSION_DUMMY) {
			CkDiagnosticThrow(dhi, expression->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot type an expression that didn't parse properly.");
			return CkExpressionCreateLiteral(outputArena, &expression->token, CK_EXPRESSION_DUMMY, NULL);
		}
		newExtra = CkSemanticsProcessExpression(dhi, outputArena, expression->extra);
	}

	new = CkExpressionCreateTernary(outputArena, &expression->token, expression->kind, NULL, newLeft, newRight, newExtra);
	lType = newLeft && newLeft->type ? newLeft->type->id : 0;
	rType = newRight && newRight->type ? newRight->type->id : 0;
	eType = newExtra && newExtra->type ? newExtra->type->id : 0;

	switch (expression->kind) {
		// Literals
	case CK_EXPRESSION_BOOL_LITERAL:
	case CK_EXPRESSION_INTEGER_LITERAL:
	case CK_EXPRESSION_FLOAT_LITERAL:
	case CK_EXPRESSION_STRING_LITERAL:
	case CK_EXPRESSION_SIZEOF:
	case CK_EXPRESSION_ALIGNOF:
		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, expression->type);
		break;

		// ++ and --, both postfix and prefix
	case CK_EXPRESSION_POSTFIX_INC:
	case CK_EXPRESSION_POSTFIX_DEC:
	case CK_EXPRESSION_PREFIX_INC:
	case CK_EXPRESSION_PREFIX_DEC:
		assert(newLeft);
		if (!newLeft->isLValue) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Increment and decrement operators require an l-value operand.");
		}

		if (!( TYPE_CLASSED_INT(lType) || lType == CK_FOOD_POINTER)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Increment and decrement operators require an operand that is of integer or pointer type.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		break;

		// Unary plus and minus
	case CK_EXPRESSION_UNARY_PLUS:
	case CK_EXPRESSION_UNARY_MINUS:
		assert(newLeft);
		if (!TYPE_CLASSED_INTFLOAT(lType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Unary plus and minus operators require an operand that is of integer or float type.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		break;

		// Bitwise NOT
	case CK_EXPRESSION_BITWISE_NOT:
		assert(newLeft);
		if (!TYPE_CLASSED_INT(lType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Bitwise NOT operator requires an operand of integer type.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		break;

		// Logical NOT
	case CK_EXPRESSION_LOGICAL_NOT:
		assert(newLeft);
		if (!(TYPE_CLASSED_INT(lType) || lType == CK_FOOD_BOOL || TYPE_CLASSED_POINTER(lType))) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Logical NOT operator requires an operand of integer or boolean type.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		break;

		// Dereference
	case CK_EXPRESSION_DEREFERENCE:
		assert(newLeft);
		if (!(lType == CK_FOOD_POINTER || lType == CK_FOOD_REFERENCE)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Dereference operator requires an operand of pointer or reference type.");
		}

		new->isLValue = TRUE;
		if (newLeft->type->child)
			new->type = CkFoodCopyTypeInstance(outputArena, newLeft->type->child);
		else new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_VOID, 0, NULL);
		break;

		// Address-Of
	case CK_EXPRESSION_ADDRESS_OF:
		assert(newLeft);
		if (!newLeft->isLValue) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot get the address of an r-value.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(
			outputArena,
			CK_FOOD_POINTER,
			0,
			CkFoodCopyTypeInstance(outputArena, newLeft->type));
		break;

		// Multiplication and division
	case CK_EXPRESSION_MUL:
	case CK_EXPRESSION_DIV:
	{
		uint8_t oType = lType < rType ? rType : lType;

		assert(newLeft);
		assert(newRight);

		if (!TYPE_CLASSED_INTFLOAT(lType) || !TYPE_CLASSED_INTFLOAT(rType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Multiplication and division operations require integer or float operands.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(
			outputArena,
			oType,
			0,
			NULL);
		break;
	}

		// Modulo
	case CK_EXPRESSION_MOD:
	{
		uint8_t oType = lType < rType ? rType : lType;

		assert(newLeft);
		assert(newRight);

		if (!TYPE_CLASSED_INT(lType) || !TYPE_CLASSED_INT(rType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The modulo operator require integer operands.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(
			outputArena,
			oType,
			0,
			NULL);
		break;
	}

		// Addition and subtraction
	case CK_EXPRESSION_ADD:
	case CK_EXPRESSION_SUB:
	{
		uint8_t oType = lType < rType ? rType : lType;
		CkFoodType *subPtrT = NULL;

		assert(newLeft);
		assert(newRight);

		if (!(TYPE_CLASSED_INTFLOAT(lType) || lType == CK_FOOD_POINTER)
		 || !(TYPE_CLASSED_INTFLOAT(rType) || rType == CK_FOOD_POINTER)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Addition and subtraction operations require integer, float or pointer operands.");
		}

		if (lType == CK_FOOD_POINTER && rType == CK_FOOD_POINTER) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Addition or subtraction of two pointers is forbidden.");
		}

		if (lType == CK_FOOD_POINTER)
			subPtrT = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		else if (rType == CK_FOOD_POINTER)
			subPtrT = CkFoodCopyTypeInstance(outputArena, newRight->type);

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(
			outputArena,
			oType,
			0,
			subPtrT);
		break;
	}

		// Shift operators
	case CK_EXPRESSION_LEFT_SHIFT:
	case CK_EXPRESSION_RIGHT_SHIFT:
	{
		uint8_t oType = lType < rType ? rType : lType;

		assert(newLeft);
		assert(newRight);

		// This is odd because the left operand can be any integer or pointer,
		// but the right operand can only be an integer.
		if (!( TYPE_CLASSED_INT(lType) || lType == CK_FOOD_POINTER )
			|| !TYPE_CLASSED_INT(rType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Bitwise shifts require the first operand to be of integer or pointer type, and the second operand to be of integer type.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(
			outputArena,
			oType,
			0,
			oType == CK_FOOD_POINTER ? CkFoodCopyTypeInstance(outputArena, newLeft->type) : NULL);
		break;
	}

		// <, <=, >, >=
	case CK_EXPRESSION_LOWER:
	case CK_EXPRESSION_LOWER_EQUAL:
	case CK_EXPRESSION_GREATER:
	case CK_EXPRESSION_GREATER_EQUAL:
		assert(newLeft);
		assert(newRight);

		if (!( TYPE_CLASSED_INTFLOAT(lType) || lType == CK_FOOD_POINTER )
			|| !( TYPE_CLASSED_INTFLOAT(rType) || rType == CK_FOOD_POINTER )) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Lower, lower equal, greater and greater-equal require operands of integer, pointer or float types.");
		}

		if (TYPE_CLASSED_FLOAT(lType) || TYPE_CLASSED_FLOAT(rType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_MESSAGE, "float-approximative-comparison",
				"Floats cannot be compared practically. An approximative version of the comparison operator will be used.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_BOOL, 0, NULL);
		break;

		// == and !=
	case CK_EXPRESSION_EQUAL:
	case CK_EXPRESSION_NOT_EQUAL:
		assert(newLeft);
		assert(newRight);

		if (lType != rType
			&& !(TYPE_CLASSED_INTFLOAT(lType) && TYPE_CLASSED_INTFLOAT(rType))
			&& !(TYPE_CLASSED_POINTER(lType) && TYPE_CLASSED_POINTER(rType))) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The equality operator requires the two operands to be compatible or equal.");
		}

		if (TYPE_CLASSED_FLOAT(lType) || TYPE_CLASSED_FLOAT(rType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_MESSAGE, "float-approximative-comparison",
				"Floats cannot be compared practically. An approximative version of the comparison operator will be used.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_BOOL, 0, NULL);
		break;

		// &, |, ^
	case CK_EXPRESSION_BITWISE_AND:
	case CK_EXPRESSION_BITWISE_OR:
	case CK_EXPRESSION_BITWISE_XOR:
	{
		uint8_t oType = lType < rType ? rType : lType;
		CkFoodType *subPtrT = NULL;

		assert(newLeft);
		assert(newRight);

		if (!( TYPE_CLASSED_INT(lType) || lType == CK_FOOD_POINTER )
			|| !( TYPE_CLASSED_INT(rType) || rType == CK_FOOD_POINTER )) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Bitwise operators require that their two operands be of integer or pointer type.");
		}

		if (lType == CK_FOOD_POINTER && rType == CK_FOOD_POINTER) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Bitwise operations on two pointers are forbidden.");
		}

		if (lType == CK_FOOD_POINTER)
			subPtrT = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		else if (rType == CK_FOOD_POINTER)
			subPtrT = CkFoodCopyTypeInstance(outputArena, newRight->type);

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(outputArena, oType, 0, subPtrT);
		break;
	}

		// &&, ||
	case CK_EXPRESSION_LOGICAL_AND:
	case CK_EXPRESSION_LOGICAL_OR:
		assert(newLeft);
		assert(newRight);

		if (!(lType == CK_FOOD_BOOL || TYPE_CLASSED_INT(lType) || TYPE_CLASSED_POINTER(lType))
		 || !(rType == CK_FOOD_BOOL || TYPE_CLASSED_INT(rType) || TYPE_CLASSED_POINTER(rType))) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Logical operations require that their two operands be of integer or boolean type.");
		}

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_BOOL, 0, NULL);
		break;

		// Casts
	case CK_EXPRESSION_C_CAST:
	case CK_EXPRESSION_FOOD_CAST:
		assert(newLeft);
		assert(newRight);

		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, newRight->type);
		break;

		// Conditional expressions
	case CK_EXPRESSION_CONDITIONAL:
	{
		uint8_t oType = lType < rType ? rType : lType;
		CkFoodType *subPtrT = NULL;

		assert(newLeft);
		assert(newRight);
		assert(newExtra);

		if (!( eType == CK_FOOD_BOOL || TYPE_CLASSED_INT(eType) || TYPE_CLASSED_POINTER(eType) )) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The condition of a conditional expression must be of integer or boolean type.");
		}

		if (lType != rType
			&& !( TYPE_CLASSED_INTFLOAT(lType) && TYPE_CLASSED_INTFLOAT(rType) )
			&& !( TYPE_CLASSED_POINTER(lType) && TYPE_CLASSED_POINTER(rType) )) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The left and right branches of a conditional expression must be of compatible or equal type.");
		}

		if (lType == CK_FOOD_POINTER && rType == CK_FOOD_POINTER) {
			if (newLeft->type->child->id != newRight->type->child->id) {
				CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
					"If the left and right branches of a conditional expression are both pointers, then they must of equal subtype.");
			}
		}

		if (lType == CK_FOOD_POINTER)
			subPtrT = CkFoodCopyTypeInstance(outputArena, newLeft->type);
		else if (rType == CK_FOOD_POINTER)
			subPtrT = CkFoodCopyTypeInstance(outputArena, newRight->type);

		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(outputArena, oType, 0, subPtrT);
		break;
	}

		// Assignment
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
		assert(newLeft);
		assert(newRight);

		if (!newLeft->isLValue) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The left operand of an assignment expression must be an l-value.");
		}

		if (newLeft->type->qualifiers & CK_QUALIFIER_CONST_BIT || lType == CK_FOOD_REFERENCE) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot modify a constant value.");
		}

		if (lType != rType
			&& !( TYPE_CLASSED_INTFLOAT(lType) && TYPE_CLASSED_INTFLOAT(rType) )
			&& !( TYPE_CLASSED_POINTER(lType) && TYPE_CLASSED_POINTER(rType) )) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Both sides of the assignment must be of equal type.");
		}
		
		new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_VOID, 0, NULL);
		break;

		// Array subscript
	case CK_EXPRESSION_SUBSCRIPT:
		assert(newLeft);
		assert(newRight);

		if (lType != CK_FOOD_POINTER && lType != CK_FOOD_ARRAY) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Subscripted object must be an array or a pointer.");
		}

		if (!TYPE_CLASSED_INT(rType)) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"The index of the subscript must be of integer type.");
		}

		if (( newLeft->type->child ? newLeft->type->child->id : 0) == CK_FOOD_VOID) {
			CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
				"Cannot index an array or pointer having a void subtype.");
		}

		// To counter null dereference
		new->isLValue = FALSE;
		if (newLeft->type->child)
			new->type = CkFoodCopyTypeInstance(outputArena, newLeft->type->child);
		else new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_VOID, 0, NULL);
		break;

		// Compound expression
	case CK_EXPRESSION_COMPOUND:
		assert(newLeft);
		assert(newRight);

		// Compound expressions always result in void to render them invalid in other
		// expressions.
		new->isLValue = FALSE;
		new->type = CkFoodCreateTypeInstance(outputArena, CK_FOOD_VOID, 0, NULL);
		break;

		// Copy over the type expression
	case CK_EXPRESSION_TYPE:
		new->isLValue = FALSE;
		new->type = CkFoodCopyTypeInstance(outputArena, expression->type);
		break;

		// TODO: Everything down here except dummy
	case CK_EXPRESSION_DUMMY:
	case CK_EXPRESSION_COMPOUND_LITERAL:
	case CK_EXPRESSION_NAMEOF:
	case CK_EXPRESSION_TYPEOF:
	case CK_EXPRESSION_MEMBER_ACCESS:
	case CK_EXPRESSION_POINTER_MEMBER_ACCESS:
	case CK_EXPRESSION_FUNCCALL:
		CkDiagnosticThrow(dhi, new->token.position, CK_DIAG_SEVERITY_ERROR, "",
			"Expression cannot be typed.");
		break;
	}
	
	return new;
}
