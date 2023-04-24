#include <Types.h>
#include <CDebug.h>
#include <string.h>

// Creates a new string builder ready to be used.
// blksize = initial capacity, number of bytes to allocate
// every time the max. capacity is reached.
void CkStrBuilderCreate( CkStrBuilder* dest, size_t blksize )
{
	CK_ARG_NON_NULL( dest );
	dest->base = calloc( blksize, 1 );
	dest->length = 0;
	dest->capacity = blksize;
	dest->blksize = blksize;
}

// Appends a single character to the string builder.
void CkStrBuilderAppendChar( CkStrBuilder* sb, char c )
{
	CK_ARG_NON_NULL( sb );
	// If the string reaches max capacity INCLUDING the null terminator
	if ( sb->length + 2 > sb->capacity ) {
		sb->capacity += sb->blksize;
		sb->base = realloc( sb->base, sb->capacity );
	}
	sb->base[sb->length] = c;
	sb->base[sb->length + 1] = '\0'; // null terminator
	sb->length++;
}

// Appends a string to the string builder.
void CkStrBuilderAppendString( CkStrBuilder* sb, char* s )
{
	size_t strlength;

	CK_ARG_NON_NULL( sb );
	CK_ARG_NON_NULL( s );

	strlength = strlen( s );

	// If the string reaches max capacity INCLUDING the null terminator
	if ( sb->length + strlength + 1 > sb->capacity ) {
		// Finding the required amount of bytes and blocks to allocate
		const size_t requiredBytes = sb->length + strlength - sb->capacity;
		const size_t requiredBlocks = requiredBytes / sb->blksize + ( ( requiredBytes % sb->blksize != 0 ) ? 1 : 0 );
		sb->capacity += sb->blksize * requiredBlocks;
		sb->base = realloc( sb->base, sb->capacity );
	}
	strcpy( sb->base + sb->length, s ); // copying. strcpy inserts a null terminator
	sb->length += strlength;
}

// Disposes of a string builder. These objects don't use arenas,
// so it is important to dispose of them.
void CkStrBuilderDispose( CkStrBuilder* sb )
{
	CK_ARG_NON_NULL( sb );

	free( sb->base );
	sb->base = NULL;
	sb->blksize = 0;
	sb->capacity = 0;
	sb->length = 0;
}
