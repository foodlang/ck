/*
 * Implements a list of objects that all have the same size.
*/

#ifndef CK_LIST_H_
#define CK_LIST_H_

#include <Types.h>
#include "Arena.h"

/// <summary>
/// A linked list.
/// </summary>
typedef struct CkListNode
{
	/// <summary>
	/// The arena used for the allocations.
	/// </summary>
	CkArenaFrame *allocator;

	/// <summary>
	/// The next node in the list. NULL means this is the end of the list.
	/// </summary>
	struct CkListNode *next;

	/// <summary>
	/// The previous node in the list. NULL means this is the start of the list.
	/// </summary>
	struct CkListNode *prev;

	/// <summary>
	/// The size of the element stored at this node.
	/// </summary>
	size_t elemSize;

	/// <summary>
	/// This is only used for the first element of a list. If this is true, then the first
	/// node is used.
	/// </summary>
	bool_t used;

} CkList, CkListNode;

/// <summary>
/// Allocates a new list inside of an arena.
/// </summary>
/// <param name="allocator">A pointer to the arena descriptor of the list.</param>
/// <param name="elemSize">The size of an element in the list.</param>
CkList *CkListStart(
	CkArenaFrame *allocator,
	size_t elemSize);

/// <summary>
/// Adds an element to the list.
/// </summary>
/// <param name="desc">The list.</param>
/// <param name="source">A pointer to the source element to add.</param>
void CkListAdd(CkList *desc, void *source);

/// <summary>
/// Accesses an existing element on the list.
/// </summary>
/// <param name="desc">The list.</param>
/// <param name="index">The index of the element to access.</param>
/// <returns></returns>
void *CkListAccess(CkList *desc, size_t index);

/// <summary>
/// Removes an element from the list.
/// </summary>
/// <param name="desc">The list.</param>
/// <param name="index">The index of the element to remove.</param>
/// <returns>The new list.</returns>
CkList *CkListRemove(CkList *desc, size_t index);

/// <summary>
/// Returns the length of a list.
/// </summary>
/// <param name="desc"></param>
size_t CkListLength( CkList *desc );

#endif
