#include <Memory/List.h>
#include <CDebug.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

CkList *CkListStart(
	CkArenaFrame *allocator,
	size_t elemSize )
{
	CkList *new;

	CK_ARG_NON_NULL(allocator);

	new = CkArenaAllocate( allocator, sizeof( CkList ) + elemSize );
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

	if ( head == desc && !head->used ) {
		head->used = TRUE;
		memcpy( head + 1, source, head->elemSize );
	}
	else {
		head->next = CkListStart( head->allocator, head->elemSize );
		head->next->prev = head;
		memcpy( head->next + 1, source, head->elemSize );
	}
}

void *CkListAccess(CkList *desc, size_t index)
{
	CkListNode *head = desc;

	CK_ARG_NON_NULL( desc );

	for ( size_t i = 0; i < index; i++ ) {
		if ( !head->next ) {
			fprintf( stderr, "ck internal error: Attempting to read out of list bounds (max = %zu, index = %zu\n", i, index );
			abort();
		}
		head = head->next;
	}

	return head + 1;
}

CkList *CkListRemove(CkList *desc, size_t index)
{
	CkListNode *head = desc;

	CK_ARG_NON_NULL( desc );

	for ( size_t i = 0; i < index; i++ ) {
		if ( !head->next ) {
			fprintf( stderr, "ck internal error: Attempting to read out of list bounds (max = %zu, index = %zu\n", i, index );
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

	if ( !desc->next ) {
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
