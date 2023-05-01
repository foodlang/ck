/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header provides a linked list implementation.
 *
 ***************************************************************************/

#ifndef CK_LIST_H_
#define CK_LIST_H_

#include <Types.h>
#include "Arena.h"

// A linked list.
typedef struct CkListNode
{
	// The arena used for the allocations.
	CkArenaFrame *allocator;

	// The next node in the list. NULL means this is the end of the list.
	struct CkListNode *next;

	// The previous node in the list. NULL means this is the start of the list.
	struct CkListNode *prev;

	// The size of the element stored at this node.
	size_t elemSize;

	// This is only used for the first element of a list. If this is true, then the first
	// node is used.
	bool_t used;

} CkList, CkListNode;

// Allocates a new list inside of an arena.
CkList *CkListStart(
	CkArenaFrame *allocator,
	size_t elemSize);

// Adds an element to the list.
void CkListAdd(CkList *desc, void *source);

// Adds a list to another list.
void CkListAddRange( CkList *desc, CkList *src );

// Accesses an existing element on the list.
void *CkListAccess(CkList *desc, size_t index);

// Removes an element from the list.
CkList *CkListRemove(CkList *desc, size_t index);

// Returns the length of a list.
size_t CkListLength( CkList *desc );

// Clears a whole list.
void CkListClear( CkList *base );

#endif
