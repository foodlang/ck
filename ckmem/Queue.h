/*
 * Implements a queue, where objects can be pushed and popped at
 * an irregular rate.
*/

#ifndef CK_QUEUE_H_
#define CK_QUEUE_H_

#include "Arena.h"

/// <summary>
/// A queue.
/// </summary>
typedef struct CkQueue
{
	/// <summary>
	/// A pointer to where the data pushed onto the queue is stored.
	/// </summary>
	void *data;

	/// <summary>
	/// The size of an element on the queue.
	/// </summary>
	size_t elemSizeAligned;

	/// <summary>
	/// The current index of the push thread.
	/// </summary>
	size_t pushIndex;

	/// <summary>
	/// The amount of objects that can be pushed without
	/// a single pop.
	/// </summary>
	size_t limit;

	/// <summary>
	/// The queue's highest index reached.
	/// </summary>
	size_t peakReached;

} CkQueue;

/// <summary>
/// Creates a new queue.
/// </summary>
/// <param name="allocator">The arena to use to allocate the queue.</param>
/// <param name="queueDescriptor">A pointer to the destination queue descriptor.</param>
/// <param name="elemSize">The size of an element in the queue.</param>
/// <param name="limit">The maximal amount of objects that can be pushed with a single pop.</param>
void CkQueueStart(
	CkArenaFrame *allocator,
	CkQueue *queueDescriptor,
	size_t elemSize,
	size_t limit);

/// <summary>
/// Resets a queue. This deletes all pushed objects that weren't popped yet.
/// </summary>
/// <param name="queueDescriptor">A pointer to the destination queue descriptor.</param>
void CkQueueReset(CkQueue *queueDescriptor);

/// <summary>
/// Pushes an object onto the queue.
/// </summary>
/// <param name="queueDescriptor">A pointer to the destination queue descriptor.</param>
/// <param name="pushBase">The base of the object to push. Its size is already defined in the queue descriptor.</param>
/// <returns>Returns false if the object cannot be pushed.</returns>
bool_t CkQueuePush(CkQueue *queueDescriptor, void *pushBase);

/// <summary>
/// Pops an object off the queue.
/// </summary>
/// <param name="queueDescriptor">A pointer to the destination queue descriptor.</param>
/// <param name="popBase">The base of the buffer that will store the popped object.</param>
/// <returns>Returns false if the object cannot be popped.</returns>
bool_t CkQueuePop(CkQueue *queueDescriptor, void *popBase);

#endif
