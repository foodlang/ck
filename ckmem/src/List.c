#include "../List.h"
#include "../CDebug.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

CkList *CkListStart(
	CkArenaFrame *allocator,
	size_t elemSize )
{
	CK_ARG_NON_NULL(allocator);

	elemSize += -elemSize & (ARENA_ALLOC_ALIGN - 1);
	CkList *new = CkArenaAllocate( allocator, sizeof( CkList ) + elemSize );
	new->allocator = allocator;
	new->elemSize = elemSize;
	new->next = NULL;
	new->prev = NULL;
	new->used = FALSE;
	return new;
}

void CkListAdd(CkList *desc, void *source)
{
	CkListNode *head = desc;

	CK_ARG_NON_NULL(desc);
	CK_ARG_NON_NULL(source);
	
	while ( head->next )
		head = head->next;

	if ( head == desc ) {
		head->used = TRUE;
		memcpy_s( head + sizeof( CkListNode ), head->elemSize, source, head->elemSize );
	}
	else {
		head->next = CkListStart( head->allocator, head->elemSize );
		head->next->prev = head;
		memcpy_s( head->next + sizeof( CkListNode ), head->elemSize, source, head->elemSize );
	}
}

void *CkListAccess(CkList *desc, size_t index)
{
	CkListNode *head = desc;

	CK_ARG_NON_NULL( desc );

	for ( size_t i = 0; i < index; i++ ) {
		if ( !head->next ) {
			fprintf_s( stderr, "ck internal error: Attempting to read out of list bounds (max = %llu, index = %llu\n", i, index );
			abort();
		}
		head = head->next;
	}

	return head + sizeof( CkListNode );
}

CkList *CkListRemove(CkList *desc, size_t index)
{
	/*size_t subbufferSize = (desc->elemCount - index) * desc->elemSize;
	char *subbuffer;
	CK_ARG_NON_NULL(desc);

	if (index == desc->elemCount) {
		fprintf_s(stderr, "ck: Attempting to remove a list element that does not exist.\n");
		abort();
	}

	// If the index is at the end of list
	if (index + 1 == desc->elemCount) {
		void *m = CkListAccess(desc, index);
		memset(m, 0, desc->elemSize);
		desc->elemCount--;
		return;
	}

	subbuffer = _malloca(subbufferSize);
	memcpy_s(subbuffer, subbufferSize, CkListAccess(desc, index + 1), subbufferSize);
	memcpy_s(CkListAccess(desc, index), subbufferSize, subbuffer, subbufferSize);
	memset(CkListAccess(desc, desc->elemCount - 1), 0, desc->elemSize);
	desc->elemCount--;
	_freea(subbuffer);*/
	CkListNode *head = desc;

	CK_ARG_NON_NULL( desc );

	for ( size_t i = 0; i < index; i++ ) {
		if ( !head->next ) {
			fprintf_s( stderr, "ck internal error: Attempting to read out of list bounds (max = %llu, index = %llu\n", i, index );
			abort();
		}
		head = head->next;
	}

	// Detaching the object
	if ( !head->prev ) {
		if ( head->next ) {
			head->next->prev = NULL;
			return head->next;
		} else;
	}  else {
		if ( !head->next )
			head->prev->next = NULL;
		else head->prev->next = head->next;
	}
	return head;
}

size_t CkListLength( CkList *desc )
{
	size_t index = 1;

	if ( !desc ) return 0;

	if ( !desc->prev ) {
		if ( desc->used )
			return 1;
		return 0;
	}

	while ( desc->next ) {
		index++;
		desc = desc->next;
	}
	return index;
}
