#include <FileIO.h>
#include <CDebug.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

CkSource *CkReadFileContents( CkArenaFrame *arena, const char *path )
{
	FILE *fileStruct;
	register off_t fileLength;
	register char *cstrBuffer;
	register CkSource *src;

	CK_ARG_NON_NULL( arena );
	CK_ARG_NON_NULL( path );

	// 1. Opening the file and getting the length
	fileStruct = fopen( path, "rb" );
	if ( !fileStruct ) {
		fprintf( stderr, "ck: '%s' does not exist, or cannot be read from.\n", path );
		return NULL;
	}
	universal_fseek( fileStruct, 0, SEEK_END );
	fileLength = (off_t)universal_ftell( fileStruct );
	universal_fseek( fileStruct, 0, SEEK_SET );

	// 2. Creating buffer
	cstrBuffer = CkArenaAllocate( arena, fileLength + 1 );

	// 3. Reading & closing the file
	universal_fread( cstrBuffer, 1, fileLength, fileStruct );
	cstrBuffer[fileLength] = 0;
	universal_fclose( fileStruct );

	src = CkArenaAllocate( arena, sizeof( CkSource ) );
	src->filename = CkStrDup(arena, (char *)path);
	src->len = fileLength;
	src->code = cstrBuffer;
	return src;
}

void CkGetRowColString( const char *string, size_t pos, size_t *pRow, size_t *pCol )
{
	CK_ARG_NON_NULL( string );
	CK_ARG_NON_NULL( pRow );
	CK_ARG_NON_NULL( pCol );

	// Column and row buffers are emptied
	*pCol = 1;
	*pRow = 1;
	for ( size_t i = 0; i < pos; i++ ) {
		if ( string[i] == '\n' ) {
			(*pRow)++;
			*pCol = 1;
		} else if ( string[i] == '\r' ) {
			*pCol = 1;
		} else (*pCol)++;
	}
}

char *CkStrDup( CkArenaFrame *allocator, char *src )
{
	size_t len;
	char *buf;
	CK_ARG_NON_NULL( allocator );
	CK_ARG_NON_NULL( src );

	len = strlen( src );
	buf = CkArenaAllocate( allocator, len + 1 );
	strcpy( buf, src );
	return buf;
}

char **CkGetLinesFreeable( char *str, size_t *lnum )
{
	size_t str_len;
	size_t max_row;
	size_t max_col;
	size_t str_pos;
	char **array;

	CK_ARG_NON_NULL( str );
	CK_ARG_NON_NULL( lnum );

	str_len = strlen( str );
	CkGetRowColString( str, str_len - 1, &max_row, &max_col );
	array = malloc( max_row * sizeof(char *) );
	*lnum = max_row;
	str_pos = 0;
	for ( size_t i = 0; i < max_row; i++ ) {
		char *lbuffer;
		size_t lbuffer_len = 0;
		char *cline_base = str + str_pos;
		char *cline_c = cline_base;

		char c = *cline_c;
		while ( TRUE ) {
			c = *(cline_c++);

			if ( !(c != '\n' && c != 0) )
				break;

			lbuffer_len++;
		}

		lbuffer = malloc( lbuffer_len + 1 );
		memcpy( lbuffer, cline_base, lbuffer_len );
		lbuffer[lbuffer_len] = 0;
		array[i] = lbuffer;
		str_pos += lbuffer_len + 1;
	}
	return array;
}

void CkFreeLines( char **lines, size_t lnum )
{
	for ( size_t i = 0; i < lnum; i++ )
		free( lines[i] );
	free( lines );
}
