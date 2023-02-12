/*
 * Basic typedefs.
*/

#ifndef CK_TYPES_H_
#define CK_TYPES_H_

#include <stdint.h>
#include <math.h>

typedef signed char bool_t;

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef NULL
	#define NULL (void *)0
#endif

static inline bool_t FloatEqual(double a, double b)
{
	const double diff = fmax(a, b) - fmin(a, b);

	return diff < 0.00005 || diff > 0.00005;
}

#endif
