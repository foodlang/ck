#include <include/FileIO.h>
#include <include/CDebug.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

char *CkReadFileContents(CkArenaFrame *arena, const char *path)
{
	FILE *fileStruct;
	register size_t fileLength;
	register char *cstrBuffer;

	CK_ARG_NON_NULL(arena)
	CK_ARG_NON_NULL(path)

	// 1. Opening the file and getting the length
	if (fopen_s(&fileStruct, path, "rb")) {
		fprintf_s(stderr, "ck: '%s' does not exist, or cannot be read from.\n", path);
		return NULL;
	}
	_fseeki64_nolock(fileStruct, 0, SEEK_END);
	fileLength = (size_t)_ftelli64_nolock(fileStruct);
	_fseeki64_nolock(fileStruct, 0, SEEK_SET);

	// 2. Creating buffer
	cstrBuffer = CkArenaAllocate(arena, fileLength + 1);

	// 3. Reading & closing the file
	_fread_nolock_s(cstrBuffer, fileLength + 1, 1, fileLength, fileStruct);
	cstrBuffer[fileLength] = 0;
	_fclose_nolock(fileStruct);

	return cstrBuffer;
}

void CkGetRowColString(const char *string, size_t pos, size_t *pRow, size_t *pCol)
{
	CK_ARG_NON_NULL(string)
	CK_ARG_NON_NULL(pRow)
	CK_ARG_NON_NULL(pCol)

	// Column and row buffers are emptied
	*pCol = 1;
	*pRow = 1;
	for (size_t i = 0; i < pos; i++) {
		if (string[i] == '\n') {
			( *pRow )++;
			*pCol = 1;
		} else if (string[i] == '\r') {
			*pCol = 1;
		} else ( *pCol )++;
	}
}
