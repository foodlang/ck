/*
 * Wrapper for a linked list that uses an arena as an allocator
*/

#ifndef CK_LIST_H_
#define CK_LIST_H_

#include "Arena.h"

typedef struct CkList
{
	/// <summary>
	/// An isolated arena used to store list items.
	/// </summary>
	CkArenaFrame *listArena;

	/// <summary>
	/// The length of the list.
	/// </summary>
	size_t length;

	/// <summary>
	/// The size of an element.
	/// </summary>
	size_t elemSize;

} CkList;

#endif
