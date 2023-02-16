/*
 * Crossplatform time functions
*/

#ifndef CK_TIME_H_
#define CK_TIME_H_

#include <Types.h>

#ifdef _WIN32
#include <Windows.h>

typedef LARGE_INTEGER CkTimePoint;
#else
#include <time.h>

typedef struct timespec CkTimePoint;
#endif

/// <summary>
/// Gets the current time.
/// </summary>
/// <param name="dest">The destination buffer for the current time.</param>
void CkTimeGetCurrent( CkTimePoint *dest );

/// <summary>
/// Gets the time elapsed between two events in milliseconds.
/// </summary>
/// <param name="start">A pointer to the descriptor of the first event.</param>
/// <param name="end">A pointer to the descriptor of the last event.</param>
/// <returns>The time elapsed between these two events in milliseconds.</returns>
int64_t CkTimeElapsed_ms( CkTimePoint *start, CkTimePoint *end );

/// <summary>
/// Gets the time elapsed between two events in microseconds.
/// </summary>
/// <param name="start">A pointer to the descriptor of the first event.</param>
/// <param name="end">A pointer to the descriptor of the last event.</param>
/// <returns>The time elapsed between these two events in microseconds.</returns>
int64_t CkTimeElapsed_mcs( CkTimePoint *start, CkTimePoint *end );

#endif
