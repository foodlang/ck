#include <FileIO.h>
#include <CDebug.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#ifdef _WIN32
#define universal_fseek(f, c, o) _fseeki64_nolock(f, c, o)
#define universal_ftell(f) _ftelli64_nolock(f)
#define universal_fread(b, z, n, f) _fread_nolock(b, z, n, f)
#define universal_fclose(f) _fclose_nolock(f)
typedef size_t off_t;
#else
#define universal_fseek(f, c, o) fseek(f, c, o)
#define universal_ftell(f) ftell(f)
#define universal_fread(b, z, n, f) fread(b, z, n, f)
#define universal_fclose(f) fclose(f)
#endif

char *CkReadFileContents( CkArenaFrame *arena, const char *path )
{
	FILE *fileStruct;
	register off_t fileLength;
	register char *cstrBuffer;

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

	return cstrBuffer;
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
