#include "Intern.h"
#include <Food.h>
#include <CDebug.h>
#include <Syntax/ConstExpr.h>

#include <stdlib.h>

static inline FoodTypeID s_GetMostInformationInt(FoodTypeID a, FoodTypeID b) { return max(a, b); }

static size_t s_AnalyzeExpression(CkExpression *expr, CkAnalyzeConfig *cfg)
{
#define CHECK_LVALUE(X, Y) if (!X->isLValue) { \
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "", \
		Y ); goto LEnd; }
	static const void *expr_dispatch_table[] = {
		&&LDummy, &&LIdentifier, &&LScopedReference, &&LCompoundLiteral, &&LIntegerLiteral,
		&&LFloatLiteral, &&LStringLiteral, &&LBoolLiteral, &&LType, &&LSizeOf, &&LAlignOf, &&LNameOf,
		&&LTypeOf, &&LPostfixInc, &&LPostfixDec, &&LFuncCall, &&LSubscript, &&LMemberAccess,
		&&LPrefixInc, &&LPrefixDec, &&LUnaryPlus, &&LUnaryMinus, &&LLogicalNot, &&LBitwiseNot,
		&&LCCast, &&LDereference, &&LOpaqueAddrOf, &&LAddrOf, &&LRef, &&LMul, &&LDiv, &&LMod,
		&&LAdd, &&LSub, &&LLeftShift, &&LRightShift, &&LLower, &&LLowerEqual, &&LGreater,
		&&LGreaterEqual, &&LEqual, &&LNotEqual, &&LBitwiseAnd, &&LBitwiseXor, &&LBitwiseOr,
		&&LLogicalAnd, &&LLogicalOr, &&LFoodCast, &&LConditional, &&LAssign, &&LAssignSum,
		&&LAssignDiff, &&LAssignProduct, &&LAssignQuotient, &&LAssignRemainder, &&LAssignLeftShift,
		&&LAssignRightShift, &&LAssignAnd, &&LAssignXor, &&LAssignOr, &&LCompound,
	};

	size_t bindings = 0;

	// ----- Check for binding -----
	if (expr->type) return 0;

	// ----- Perform binding -----
	CK_ASSERT(expr->kind <= CK_EXPRESSION_MAX);
	goto *expr_dispatch_table[expr->kind];

LDummy:
	fprintf(stderr, "Unsupported expression %d\n", expr->kind);
	abort(); // No fallthrough

LIdentifier:
	// TODO
	goto LEnd;

LScopedReference:
	// TODO
	goto LEnd;

LCompoundLiteral:
	// TODO
	goto LEnd;

LIntegerLiteral:
LFloatLiteral:
LStringLiteral:
LBoolLiteral:
	goto LEnd;

LType: goto LEnd;

LSizeOf:
LAlignOf:
	bindings++;
	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_U64, 0, NULL);
	goto LEnd;

LNameOf:
	bindings++;
	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_STRING, 0, NULL);
	goto LEnd;

LTypeOf:
	bindings++;
	// TODO
	goto LEnd;

LPostfixInc:
LPostfixDec:
LPrefixInc:
LPrefixDec:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	// L-value check
	CHECK_LVALUE(expr->left, "Operand of postfix/prefix ++/-- must be an l-value.");

	// Integer/pointer check
	if (!CK_TYPE_CLASSED_INT(expr->left->type->id) && !CK_TYPE_CLASSED_POINTER_ARITHM(expr->left->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"Operand of postfix/prefix ++/-- must be an integer or a pointer that supports arithmetic operations." );
		goto LEnd;
	}

	if (expr->left->type->id == CK_FOOD_ARRAY
		&& (expr->type->id == CK_EXPRESSION_POSTFIX_DEC || expr->type->id == CK_EXPRESSION_PREFIX_DEC)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"Arrays don't support postfix/prefix decrement (--) operators.");
		goto LEnd;
	}
	// TODO: Arrays

	expr->type = CkFoodCopyTypeInstance(cfg->allocator, expr->left->type);
	goto LEnd;

LFuncCall:
	goto LEnd;

LSubscript:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	// Incomplete binding
	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_POINTER_ARITHM(expr->left->type->id) || !CK_TYPE_CLASSED_INT(expr->right->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The expression to be subscripted must be an expression or an array and its operand must be an integer.");
		goto LEnd;
	}

	expr->type = CkFoodCopyTypeInstance(cfg->allocator, expr->left->type->child);
	goto LEnd;

LMemberAccess:
	// TODO
	goto LEnd;

LUnaryPlus:
LUnaryMinus:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INTFLOAT(expr->left->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operand of the unary minus (-) or plus (+) must be an integer or a floating-point number.");
		goto LEnd;
	}

	expr->type = CkFoodCopyTypeInstance(cfg->allocator, expr->left->type);
	goto LEnd;

LLogicalNot:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	if (!(expr->left->type->id == CK_FOOD_BOOL
		|| CK_TYPE_CLASSED_INT(expr->left->type->id)
		|| CK_TYPE_CLASSED_POINTER(expr->left->type->id))) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operand of the logical not operator (!) must be a boolean, integer or a pointer.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_BOOL, 0, NULL);
	goto LEnd;

LBitwiseNot:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INT(expr->left->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operand of the bitwise not operator (~) must be an integer.");
		goto LEnd;
	}

	expr->type = CkFoodCopyTypeInstance(cfg->allocator, expr->left->type);
	goto LEnd;

LCCast:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	goto LEnd;

LDereference:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_POINTER(expr->left->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operand of the dereference operator (*) must be a pointer.");
		goto LEnd;
	}

	// Check for incomplete type
	if (expr->left->type->child->id == CK_FOOD_VOID) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"Cannot dereference a pointer to an incomplete type.");
		goto LEnd;
	}

	expr->type = CkFoodCopyTypeInstance(cfg->allocator, expr->left->type->child);
	expr->isLValue = true;
	goto LEnd;


LAddrOf:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	CHECK_LVALUE(expr->left, "The operand of the address-of (&) operator must be an l-value.");

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator, CK_FOOD_POINTER, 0,
		CkFoodCopyTypeInstance(cfg->allocator, expr->left->type));
	goto LEnd;

LOpaqueAddrOf:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	CHECK_LVALUE(expr->left, "The operand of the opaque address-of (&&) operator must be an l-value.");

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator, CK_FOOD_POINTER, 0,
		CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_VOID, 0, NULL));
	goto LEnd;

LRef:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	CHECK_LVALUE(expr->left, "The operand of the reference (ref) operator must be an l-value.");

	if (expr->left->type->id != CK_FOOD_REFERENCE) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"It is impossible to get the reference of a reference.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator, CK_FOOD_REFERENCE, 0, CkFoodCopyTypeInstance(cfg->allocator, expr->left->type));
	goto LEnd;

LMul:
LDiv:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INTFLOAT(expr->left->type->id) || !CK_TYPE_CLASSED_INTFLOAT(expr->right->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands of the multiplication/division operator must be integers or floating-point numbers.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator,
		s_GetMostInformationInt(expr->left->type->id, expr->right->type->id), 0, NULL);
	goto LEnd;

LMod:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INT(expr->left->type->id) || !CK_TYPE_CLASSED_INT(expr->right->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands of the modulo operator must be integers.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator,
		s_GetMostInformationInt(expr->left->type->id, expr->right->type->id), 0, NULL);
	goto LEnd;

LAdd:
LSub:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	// left operand can be int, float, pointer; right can be int, float.
	if (!(CK_TYPE_CLASSED_INTFLOAT(expr->left->type->id) || CK_TYPE_CLASSED_POINTER_ARITHM(expr->left->type->id))
		|| !CK_TYPE_CLASSED_INTFLOAT(expr->left->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands of the addition and subtraction operators must be integers or floats. The left operand"
		    " can also be a pointer.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator,
		s_GetMostInformationInt(expr->left->type->id, expr->right->type->id), 0, NULL);
	goto LEnd;

LLeftShift:
LRightShift:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INT(expr->left->type->id) || !CK_TYPE_CLASSED_INT(expr->right->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands of the left and right bitwise shift operators need to be integers.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator,
		s_GetMostInformationInt(expr->left->type->id, expr->right->type->id), 0, NULL);
	goto LEnd;

LLower:
LLowerEqual:
LGreater:
LGreaterEqual:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INTFLOAT(expr->left->type->id) || !CK_TYPE_CLASSED_INTFLOAT(expr->right->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands of the inequal comparisons lower, lower equal, greater and greater equal must be integers or floats.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_BOOL, 0, NULL);
	goto LEnd;

LEqual:
LNotEqual:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	// References cannot be compared
	if (expr->left->type->id == expr->right->type->id && expr->left->type->id == CK_FOOD_REFERENCE) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"References cannot be compared.");
		goto LEnd;
	}

	if (CK_TYPE_CLASSED_INTFLOAT(expr->left->type->id) && CK_TYPE_CLASSED_INTFLOAT(expr->right->type->id)) goto LEnd;
	if (CK_TYPE_CLASSED_POINTER(expr->left->type->id) && CK_TYPE_CLASSED_POINTER(expr->right->type->id)) goto LEnd;
	// TODO: User-type compare

	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_BOOL, 0, NULL);
	goto LEnd;

LBitwiseAnd:
LBitwiseXor:
LBitwiseOr:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!CK_TYPE_CLASSED_INT(expr->left->type->id) || !CK_TYPE_CLASSED_INT(expr->right->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands must be integers.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(
		cfg->allocator,
		s_GetMostInformationInt(expr->left->type->id, expr->right->type->id), 0, NULL);
	goto LEnd;

LLogicalAnd:
LLogicalOr:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	if (!(expr->left->type->id == CK_FOOD_BOOL || CK_TYPE_CLASSED_INT(expr->left->type->id) || CK_TYPE_CLASSED_POINTER(expr->left->type->id))
		|| !(expr->right->type->id == CK_FOOD_BOOL || CK_TYPE_CLASSED_INT(expr->right->type->id) || CK_TYPE_CLASSED_POINTER(expr->right->type->id))) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The operands must be integers or booleans.");
		goto LEnd;
	}

	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_BOOL, 0, NULL);
	goto LEnd;

LFoodCast:
	CK_ASSERT(expr->left);
	bindings += s_AnalyzeExpression(expr->left, cfg);

	// Incomplete binding
	if (expr->left->type == NULL)
		return 0;

	goto LEnd;

LConditional:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	CK_ASSERT(expr->extra);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);
	bindings += s_AnalyzeExpression(expr->extra, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL || expr->extra->type == NULL)
		return 0;

	if (expr->extra->type->id != CK_FOOD_BOOL
		&& !CK_TYPE_CLASSED_INT(expr->extra->type->id)
		&& !CK_TYPE_CLASSED_POINTER(expr->extra->type->id)) {
		CkDiagnosticThrow(cfg->p_dhi, &expr->token, CK_DIAG_SEVERITY_ERROR, "",
			"The condition of a conditional statement must be a boolean, an integer or a pointer.");
		goto LEnd;
	}

	// TODO: check operands

	goto LEnd;

LAssign:
LAssignSum:
LAssignDiff:
LAssignProduct:
LAssignQuotient:
LAssignRemainder:
LAssignLeftShift:
LAssignRightShift:
LAssignAnd:
LAssignXor:
LAssignOr:
	// TODO
	goto LEnd;

LCompound:
	CK_ASSERT(expr->left);
	CK_ASSERT(expr->right);
	bindings += s_AnalyzeExpression(expr->left, cfg);
	bindings += s_AnalyzeExpression(expr->right, cfg);

	if (expr->left->type == NULL || expr->right->type == NULL)
		return 0;

	expr->type = CkFoodCreateTypeInstance(cfg->allocator, CK_FOOD_VOID, 0, NULL);

	// fallthrough
LEnd:
	return bindings;
}

size_t AnalyzeExpression(CkExpression *expr, CkAnalyzeConfig *cfg)
{
	size_t ret = s_AnalyzeExpression(expr, cfg);
	expr = CkConstExprReduce(cfg->allocator, expr);
	return ret;
}
