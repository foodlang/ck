/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header provides cross-platform implementations of time functions
 * in milliseconds and microseconds.
 *
 ***************************************************************************/

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

// Gets the current time.
void CkTimeGetCurrent( CkTimePoint *dest );

// Gets the time elapsed between two events in milliseconds.
int64_t CkTimeElapsed_ms( CkTimePoint *start, CkTimePoint *end );

// Gets the time elapsed between two events in microseconds.
int64_t CkTimeElapsed_mcs( CkTimePoint *start, CkTimePoint *end );

#endif
