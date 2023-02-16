/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the semantics analyzer. The semantics analyzer
 * verifies expressions and AST objects for semantic validity. It also
 * performs type propagation. The semantics analyzer works closely with the
 * parser. It also checks and adds lvalue statuses to expressions and
 * validates the usage of references.
 *
 ***************************************************************************/

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

// Figures out the semantics of an expression.
CkExpression *CkSemanticsProcessExpression(
	CkDiagnosticHandlerInstance *dhi,
	CkArenaFrame *outputArena,
	const CkExpression *expression);

#endif
