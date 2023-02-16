/*
 * Semantics Analyzer for CK
 * Contains:
 *  -> l-value evaluator (Checks for l-value expressions)
 *  -> Type evaluator / checker (Evaluates the types of expressions)
 *  -> Reference validator (Validates the usage of reference types)
*/

#ifndef CK_SEMANTICS_H_
#define CK_SEMANTICS_H_

#include "Expression.h"
#include "../Diagnostics.h"

#define CK_TYPE_CLASSED_INT(x) ((x) == CK_FOOD_I8 || (x) == CK_FOOD_U8 || (x) == CK_FOOD_I16 || (x) == CK_FOOD_U16 || \
							 (x) == CK_FOOD_I32 || (x) == CK_FOOD_U32 || (x) == CK_FOOD_I64 || (x) == CK_FOOD_U64 || \
							 (x) == CK_FOOD_ENUM)

#define CK_TYPE_CLASSED_FLOAT(x) ((x) == CK_FOOD_F16 || (x) == CK_FOOD_F32 || (x) == CK_FOOD_F64)

#define CK_TYPE_CLASSED_INTFLOAT(x) ((x) >= CK_FOOD_I8 && (x) <= CK_FOOD_F64)

#define CK_TYPE_CLASSED_POINTER(x) ((x) == CK_FOOD_POINTER || (x) == CK_FOOD_FUNCPOINTER)

/// <summary>
/// Figures out the semantics of an expression.
/// </summary>
/// <param name="dhi">A pointer to the diagnostic handler to report errors and warnings to.</param>
/// <param name="outputArena">A pointer to the arena descriptor that provides memory for the new expression.</param>
/// <param name="expression">The input expression. This is left untouched and all writes are performed on the returning expression.</param>
/// <returns>The expression that has been populated with various semantic data.</returns>
CkExpression *CkSemanticsProcessExpression(
	CkDiagnosticHandlerInstance *dhi,
	CkArenaFrame *outputArena,
	const CkExpression *expression);

#endif
