/*
 * Provides functions to emit Food IL.
*/

#ifndef CK_EMITTER_H_
#define CK_EMITTER_H_

#include "../Syntax/Expression.h"
#include "IL.h"

/// <summary>
/// Emits an expression.
/// </summary>
/// <param name="expression">The expression to emit.</param>
/// <param name="result">A pointer to the reference descriptor. This is where the result of the expression is stored.</param>
void CkEmitExpression(
	CkTACFunctionDecl *func,
	CkExpression *expression,
	CkTACReference *result);

#endif
