/*
 * Implements a list of objects that all have the same size.
*/

#ifndef CK_LIST_H_
#define CK_LIST_H_

#include "Types.h"
#include "Arena.h"

/// <summary>
/// A linear list of objects that all have the same size.
/// The elements in the list are aligned to a 16-byte boundary.
/// </summary>
typedef struct CkList
{
	/// <summary>
	/// The base of the list.
	/// </summary>
	void *base;

	/// <summary>
	/// The size of an individual element.
	/// </summary>
	size_t elemSize;

	/// <summary>
	/// The amount of elements available.
	/// </summary>
	size_t elemCount;

	/// <summary>
	/// The maximum capacity of the array,
	/// in elements.
	/// </summary>
	size_t capacity;

	/// <summary>
	/// The peak index. This index can only
	/// go up (it is the highest element
	/// that has been written to in the list.)
	/// </summary>
	size_t peakReached;

} CkList;

/// <summary>
/// Allocates a new list inside of an arena.
/// </summary>
/// <param name="allocator">A pointer to the arena descriptor of the list.</param>
/// <param name="elemSize">The size of an element in the list.</param>
/// <param name="capacity">The amount of elements to reserve for the list. Its length cannot go over its capacity.</param>
void CkListStart(
	CkList *desc,
	CkArenaFrame *allocator,
	size_t elemSize,
	size_t capacity);

/// <summary>
/// Clears out the list, up to its peak access index.
/// </summary>
/// <param name="desc">A pointer to the list descriptor.</param>
void CkListClear(CkList *desc);

/// <summary>
/// Adds an element to the list.
/// </summary>
/// <param name="desc">A pointer to the list descriptor.</param>
/// <param name="source">A pointer to the source element to add.</param>
void CkListAdd(CkList *desc, void *source);

/// <summary>
/// Accesses an existing element on the list.
/// </summary>
/// <param name="desc">A pointer to the list descriptor.</param>
/// <param name="index">The index of the element to access.</param>
/// <returns></returns>
void *CkListAccess(CkList *desc, size_t index);

/// <summary>
/// Removes an element from the list.
/// </summary>
/// <param name="desc">A pointer to the list descriptor.</param>
/// <param name="index">The index of the element to access.</param>
void CkListRemove(CkList *desc, size_t index);

#endif
