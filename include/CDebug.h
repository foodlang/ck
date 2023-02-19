/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares debugging macros and functions (for the debug version
 * of CK, not compiling Food projects for debugging.)
 *
 ***************************************************************************/

#ifndef CK_CDEBUG_H_
#define CK_CDEBUG_H_

#include <stdio.h>
#include <stdlib.h>

// CK_ARG_NON_NULL: Check argument for nullity
// CK_ASSERT: Assert

#ifdef _DEBUG
#define CK_ARG_NON_NULL(x)   \
	if (!(x)) {              \
		fprintf(             \
			stderr,          \
			"ck (debug): func %s does not accept null arguments, and %s is null\n", \
			__func__, #x);   \
		abort();             \
	}
#define CK_ASSERT(x) \
	if (!(x)) { fprintf(stderr, "ck (debug): assertion '%s' failed in function %s\n", #x, __func__); abort(); }
#else
#define CK_ARG_NON_NULL(x) (void)x
#define CK_ASSERT(x) (void)x
#endif

#endif
