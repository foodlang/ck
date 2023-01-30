#include <include/Queue.h>

#include <string.h>

void CkQueueStart(
	CkArenaFrame *allocator,
	CkQueue *queueDescriptor,
	size_t elemSize,
	size_t limit)
{
	queueDescriptor->elemSizeAligned = ( elemSize + ( -elemSize & ( ARENA_ALLOC_ALIGN - 1 ) ) );
	queueDescriptor->pushIndex = 0;
	queueDescriptor->limit = limit;
	queueDescriptor->data = CkArenaAllocate(allocator, queueDescriptor->limit * queueDescriptor->elemSizeAligned);
}

void CkQueueReset(CkQueue *queueDescriptor)
{
	queueDescriptor->pushIndex = 0;
	memset(queueDescriptor->data, 0, queueDescriptor->peakReached * queueDescriptor->elemSizeAligned);
}

bool_t CkQueuePush(CkQueue *queueDescriptor, void *pushBase)
{
	const size_t index = queueDescriptor->pushIndex++;
	char *writebase;

	if (index == queueDescriptor->limit) return FALSE;
	writebase = (char *)queueDescriptor->data + queueDescriptor->elemSizeAligned * index;
	memcpy_s(writebase, queueDescriptor->elemSizeAligned, pushBase, queueDescriptor->elemSizeAligned);
	return TRUE;
}

bool_t CkQueuePop(CkQueue *queueDescriptor, void *popBase)
{
	// Return false if no objects can be popped
	if (!queueDescriptor->pushIndex) return FALSE;

	// Reading popped value
	memcpy_s(
		popBase,
		queueDescriptor->elemSizeAligned,
		queueDescriptor->data,
		queueDescriptor->elemSizeAligned);

	// Popping
	memcpy_s(
		queueDescriptor->data,
		queueDescriptor->limit * queueDescriptor->elemSizeAligned,
		(char *)queueDescriptor->data + queueDescriptor->elemSizeAligned,
		queueDescriptor->pushIndex * queueDescriptor->elemSizeAligned);
	queueDescriptor->pushIndex--;
	return TRUE;
}
