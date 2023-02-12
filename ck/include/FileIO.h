/*
 * Provides functions for file/string operations.
*/

#ifndef CK_FILEIO_H_
#define CK_FILEIO_H_

#include <ckmem/Types.h>
#include <ckmem/Arena.h>

/// <summary>
/// Reads a file and puts the contents into a buffer.
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
char *CkReadFileContents(CkArenaFrame *arena, const char *path);

/// <summary>
/// Gets the colum and row from a 1D position in a string.
/// </summary>
/// <param name="string"></param>
/// <param name="pCol"></param>
/// <param name="pRow"></param>
void CkGetRowColString(const char *string, size_t pos, size_t *pRow, size_t *pCol);

#endif
