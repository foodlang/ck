#include <include/Util/Time.h>

#if _WIN32
static LARGE_INTEGER s_frequency = { .QuadPart = -1 };
#endif

void CkTimeGetCurrent( CkTimePoint *dest )
{
#if _WIN32
	QueryPerformanceCounter( dest );
#else
	clock_gettime( CLOCK_MONOTOMIC, dest );
#endif
}

int64_t CkTimeElapsed_ms( CkTimePoint *start, CkTimePoint *end )
{
	register int64_t result;
#if _WIN32

	// Querying frequency (required for universal time measure)
	if ( s_frequency.QuadPart == -1 )
		QueryPerformanceFrequency( &s_frequency );

	result = end->QuadPart - start->QuadPart;
	result *= 1000;
	result /= s_frequency.QuadPart;
#else

	result = ((end->tv_sec * 1000) + (end->tv_nsec / 1000000))
		- ((start->tv_sec * 1000) + (start->tv_nsec / 1000000);

#endif
	return result;
}

int64_t CkTimeElapsed_mcs( CkTimePoint *start, CkTimePoint *end )
{
	register int64_t result;
#if _WIN32

	// Querying frequency (required for universal time measure)
	if ( s_frequency.QuadPart == -1 )
		QueryPerformanceFrequency( &s_frequency );

	result = end->QuadPart - start->QuadPart;
	result *= 1000000;
	result /= s_frequency.QuadPart;
#else

	result = ((end->tv_sec * 1000000) + (end->tv_nsec / 1000))
		- ((start->tv_sec * 1000000) + (start->tv_nsec / 1000));

#endif
	return result;
}
