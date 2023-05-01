/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header provides file/string I/O helper functions.
 *
 ***************************************************************************/

#ifndef CK_FILEIO_H_
#define CK_FILEIO_H_

#include <Types.h>
#include <Memory/Arena.h>

// Reads a file and puts the contents into a buffer.
CkSource *CkReadFileContents(CkArenaFrame *arena, const char *path);

// Gets the colum and row from a 1D position in a string.
void CkGetRowColString(const char *string, size_t pos, size_t *pRow, size_t *pCol);

// Duplicates a string.
char *CkStrDup( CkArenaFrame *allocator, char *src );

// Gets all of the lines in a string. Uses malloc().
char **CkGetLinesFreeable( char *str, size_t *lnum );

// Frees an array of lines.
void CkFreeLines( char **lines, size_t lnum );

#endif
