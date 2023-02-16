/*
 * Arena allocator for CK
*/

#ifndef CK_ARENA_H_
#define CK_ARENA_H_

#include <Types.h>

// The alignment required for objects in the arena.
#define ARENA_ALLOC_ALIGN 16

/// <summary>
/// Stores and keep information about allocation blocks.
/// You cannot deallocate objects in arenas manually;
/// all objects are deallocated once the arena frame
/// has been ended.
/// </summary>
typedef struct CkArenaFrame
{
	/// <summary>
	/// The base of the newly allocated memory. This memory
	/// has been allocated via virtual allocation (VirtualAlloc
	/// on Windows, mmap on POSIX.)
	/// </summary>
	void *base;

	/// <summary>
	/// The size of the arena. Changing this parameter may result
	/// in memory leaks and/or undefined behaviour.
	/// </summary>
	size_t size;

	/// <summary>
	/// The offset to the next free address in the arena.
	/// </summary>
	size_t offsetFree;

} CkArenaFrame;

/// <summary>
/// Starts a new arena frame. All of the data that is newly allocated
/// is zeroed out.
/// </summary>
/// <param name="dest">The destination arena frame structure.</param>
/// <param name="maxSize">The maximum size of the arena. If set to 0, will default to 256 megabytes.</param>
void CkArenaStartFrame( CkArenaFrame *dest, size_t maxSize );

/// <summary>
/// Ends and deallocates an arena frame. All subframes are deallocated to prevent memory leaks.
/// </summary>
/// <param name="frame">The frame to deallocate.</param>
void CkArenaEndFrame( CkArenaFrame *frame );

/// <summary>
/// Resets an arena frame, without deallocating it.
/// The data is zeroed out to maintain compatibility with
/// CkArenaStartFrame().
/// </summary>
/// <param name="frame"></param>
void CkArenaResetFrame( CkArenaFrame *frame );

/// <summary>
/// Allocates a new memory block on the current arena frame.
/// This memory cannot be freed.
/// </summary>
/// <param name="frame">The frame to allocate memory from.</param>
/// <param name="bytes">The amount of bytes to allocate. Padding may be applied.</param>
/// <returns>A pointer to the newly allocated memory.</returns>
void *CkArenaAllocate( CkArenaFrame *frame, size_t bytes );

/// <summary>
/// Locks an arena and prevents writing. This uses the MMU.
/// </summary>
/// <param name="frame">The frame of the arena.</param>
void CkArenaWriteLock( CkArenaFrame *frame );

/// <summary>
/// Unlocks an arena and allows writing. This uses the MMU.
/// </summary>
/// <param name="frame">The frame of the arena.</param>
void CkArenaWriteUnlock( CkArenaFrame *frame );

/// <summary>
/// Disables execution permissions for the arena. This uses the MMU.
/// </summary>
/// <param name="frame"></param>
void CkArenaExecLock( CkArenaFrame *frame );

/// <summary>
/// Enables execution permissions for the arena. This uses the MMU.
/// </summary>
/// <param name="frame"></param>
void CkArenaExecUnlock( CkArenaFrame *frame );

#endif
