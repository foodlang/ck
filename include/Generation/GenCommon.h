/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the common generator, which allows the use of scripts
 * to generate code from a library tree.
 *
 ***************************************************************************/

#ifndef CK_GEN_COMMON_H_
#define CK_GEN_COMMON_H_

#include "../Memory/List.h"
#include "../IL/FFStruct.h"
#include "../Diagnostics.h"

#include <lua.h>
#include <lapi.h>
#include <lualib.h>
#include <lauxlib.h>

// Generates a program out. WARNING: This does
// not use arena allocation, but rather uses
// standard malloc()/free().
char *CkGenProgram(
	CkDiagnosticHandlerInstance* pDhi, // The diagnostics handler
	CkArenaFrame* allocator, // Allocator for statically sized objects
	CkList *libraries, // Element type = CkLibrary*
	lua_State *script  // The Lua state used as a gen script
	);

#endif
