/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines functions used when simplifying constant expressions.
 * It also simplifies other non-constant expressions to a minimum.
 *
 ***************************************************************************/

#ifndef CK_CONSTEXPR_H_
#define CK_CONSTEXPR_H_

#include "Expression.h"

// Reduces an expression to its simpliest form (best performance.)
// This must be run after the binder. May or may not perform copy or
// create new expressions.
CkExpression *CkConstExprReduce( CkArenaFrame *arena, CkExpression *src );

#endif
