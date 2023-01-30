#include <include/List.h>
#include <include/CDebug.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

void CkListStart(
	CkList *desc,
	CkArenaFrame *allocator,
	size_t elemSize,
	size_t capacity)
{
	CK_ARG_NON_NULL(desc);
	CK_ARG_NON_NULL(allocator);

	desc->capacity = capacity;
	desc->elemCount = 0;
	desc->elemSize = ( elemSize + ( -elemSize & ( ARENA_ALLOC_ALIGN - 1 ) ) );
	desc->peakReached = 0;
	desc->base = CkArenaAllocate(allocator, desc->elemSize * capacity);
}

void CkListClear(CkList *desc)
{
	CK_ARG_NON_NULL(desc);
	memset(desc->base, 0, desc->elemSize * desc->peakReached);
}

void CkListAdd(CkList *desc, void *source)
{
	const size_t index = desc->elemCount++;
	void *m = CkListAccess(desc, index);

	CK_ARG_NON_NULL(desc);
	CK_ARG_NON_NULL(source);

	if (desc->peakReached == desc->elemCount)
		desc->peakReached++;

	if (desc->elemCount > desc->capacity) {
		fprintf_s(stderr, "ck: Attempting to allocate beyond a list's capacity, which causes arena corruption.\n");
		abort();
	}

	memcpy_s(m, desc->elemSize, source, desc->elemSize);
}

void *CkListAccess(CkList *desc, size_t index)
{
	CK_ARG_NON_NULL(desc);
	if (index >= desc->elemCount) {
		fprintf_s(stderr, "ck: Attempting to read outside of allocated element bounds of a list.\n");
		abort();
	}

	return (char *)desc->base + index * desc->elemSize;
}

void CkListRemove(CkList *desc, size_t index)
{
	size_t subbufferSize = ( desc->elemCount - index ) * desc->elemSize;
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
	_freea(subbuffer);
}
