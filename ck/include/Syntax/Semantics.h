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
