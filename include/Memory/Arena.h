/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header provides functions and structures for arena allocation. CK
 * uses an arena allocator instead of malloc()/free() for easier memory
 * management. Arenas also provide faster allocations and a 16-byte
 * alignment.
 *
 ***************************************************************************/

#ifndef CK_ARENA_H_
#define CK_ARENA_H_

#include <Types.h>

// The alignment required for objects in the arena.
#define ARENA_ALLOC_ALIGN 16

// Stores and keep information about allocation blocks.
// You cannot deallocate objects in arenas manually;
// all objects are deallocated once the arena frame
// has been ended.
typedef struct CkArenaFrame
{
	// The base of the newly allocated memory. This memory
	// has been allocated via virtual allocation (VirtualAlloc
	// on Windows, mmap on POSIX.)
	void *base;

	// The size of the arena. Changing this parameter may result
	// in memory leaks and/or undefined behaviour.
	size_t size;

	// The offset to the next free address in the arena.
	size_t offsetFree;

} CkArenaFrame;

// Starts a new arena frame. All of the data that is newly allocated
// is zeroed out.
void CkArenaStartFrame( CkArenaFrame *dest, size_t maxSize );

// Ends and deallocates an arena frame. All subframes are deallocated to prevent memory leaks.
void CkArenaEndFrame( CkArenaFrame *frame );

// Resets an arena frame, without deallocating it.
// The data is zeroed out to maintain compatibility with
// CkArenaStartFrame().
void CkArenaResetFrame( CkArenaFrame *frame );

// Allocates a new memory block on the current arena frame.
// This memory cannot be freed.
void *CkArenaAllocate( CkArenaFrame *frame, size_t bytes );

// Locks an arena and prevents writing. This uses the MMU.
void CkArenaWriteLock( CkArenaFrame *frame );

// Unlocks an arena and allows writing. This uses the MMU.
void CkArenaWriteUnlock( CkArenaFrame *frame );

// Disables execution permissions for the arena. This uses the MMU.
void CkArenaExecLock( CkArenaFrame *frame );

// Enables execution permissions for the arena. This uses the MMU.
void CkArenaExecUnlock( CkArenaFrame *frame );

#endif
