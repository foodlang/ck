/*
 * Debugging macros for the CK compiler (internal)
*/

#ifndef CK_CDEBUG_H_
#define CK_CDEBUG_H_

#include <stdio.h>
#include <stdlib.h>

// CK_ARG_NON_NULL: Check argument for nullity

#if _DEBUG
#define CK_ARG_NON_NULL(x)   \
	if (!(x)) {              \
		fprintf(             \
			stderr,          \
			"ck (debug): func %s does not accept null arguments, and %s is null\n", \
			__func__, #x);   \
		abort();             \
	}
#else
#define CK_ARG_NON_NULL(x)
#endif

#endif