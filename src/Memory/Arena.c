#include <Memory/Arena.h>
#include <CDebug.h>
#include <stdio.h>
#include <string.h>
#if _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

// Comment to use VirtualAlloc()/mmap().
#define USE_MALLOC

// Default maximum arena size
#define DEFAULT_MAXSIZE 536870912

void CkArenaStartFrame( CkArenaFrame *dest, size_t maxSize )
{
	CK_ARG_NON_NULL( dest );

	if ( maxSize == 0 ) maxSize = DEFAULT_MAXSIZE;
	dest->size = maxSize;
	dest->offsetFree = 0;
#if defined(USE_MALLOC)
	dest->base = malloc( DEFAULT_MAXSIZE );
#elif defined(_WIN32)
	dest->base = VirtualAlloc( 0, maxSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
#else
	dest->base = mmap( 0, maxSize, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
#endif
}

void CkArenaEndFrame( CkArenaFrame *frame )
{
	CK_ARG_NON_NULL( frame );

#ifdef _DEBUG
	printf("(debug) %zu bytes in use when arena 0x%zX was freed.\n", frame->offsetFree, frame->base);
#endif

#if defined(USE_MALLOC)
	free( frame->base );
#elif defined(_WIN32)
	VirtualFree( frame->base, 0, MEM_RELEASE );
#else
	munmap( frame->base, frame->size );
#endif
}

void *CkArenaAllocate( CkArenaFrame *frame, size_t bytes )
{
	register void *yield = (char *)frame->base + frame->offsetFree;

	CK_ARG_NON_NULL( frame );

	// Allocation & alignment
	bytes += -bytes & (ARENA_ALLOC_ALIGN - 1);
	memset( frame->base + frame->offsetFree, 0, bytes );
	frame->offsetFree += bytes;
	if ( frame->offsetFree > frame->size ) {
		fprintf( stderr, "ck: Allocating beyond arena frame (limit = %zu bytes)\n", frame->size );
		abort();
	}
	return yield;
}

void CkArenaResetFrame( CkArenaFrame *frame )
{
	CK_ARG_NON_NULL( frame );

	memset( frame->base, 0, frame->offsetFree );
	frame->offsetFree = 0;
}

void CkArenaWriteLock( CkArenaFrame *frame )
{
	unsigned long old;
	CK_ARG_NON_NULL( frame );

#if defined(USE_MALLOC)
	(void)old; // do nothing
#elif defined(_WIN32)
	VirtualProtect( frame->base, frame->size, PAGE_READONLY, &old );
#else
	mprotect( frame->base, frame->size, PROT_READ );
#endif
}

void CkArenaWriteUnlock( CkArenaFrame *frame )
{
	unsigned long old;
	CK_ARG_NON_NULL( frame );

#if defined(USE_MALLOC)
	(void)old; // do nothing
#elif defined(_WIN32)
	VirtualProtect( frame->base, frame->size, PAGE_READWRITE, &old );
#else
	mprotect( frame->base, frame->size, PROT_READ | PROT_WRITE );
#endif
}

void CkArenaExecLock( CkArenaFrame *frame )
{
	unsigned long old;
	CK_ARG_NON_NULL( frame );

#if defined(USE_MALLOC)
	(void)old; // do nothing
#elif defined(_WIN32)
	VirtualProtect( frame->base, frame->size, PAGE_READWRITE, &old );
#else
	mprotect( frame->base, frame->size, PROT_READ | PROT_WRITE );
#endif
}

void CkArenaExecUnlock( CkArenaFrame *frame )
{
	unsigned long old;
	CK_ARG_NON_NULL( frame );

#if defined(USE_MALLOC)
	(void)old; // do nothing
#elif defined(_WIN32)
	VirtualProtect( frame->base, frame->size, PAGE_EXECUTE_READWRITE, &old );
#else
	mprotect( frame->base, frame->size, PROT_READ | PROT_WRITE | PROT_EXEC );
#endif
}
