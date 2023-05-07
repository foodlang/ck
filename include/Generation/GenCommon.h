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
#include "../CDebug.h"

#include <lua.h>
#include <lapi.h>
#include <lualib.h>
#include <lauxlib.h>

// A generator.
typedef struct CkGenerator
{
	CkDiagnosticHandlerInstance *pDhi;   // The diagnostic handler.
	CkArenaFrame                *alloc;  // The allocator for the generator.
	CkStrBuilder                *outsb;  // The output string builder.
	CkList                      *libs;   // The libraries to generate. Elem type = CkList *
	lua_State                   *script; // The lua script.

} CkGenerator;

// Creates a new generator instance.
static inline void CkCreateGenerator(
	CkGenerator                 *pGen,   // The generator.
	CkDiagnosticHandlerInstance *pDhi,   // The diagnostic handler.
	CkArenaFrame                *alloc,  // The allocator for the generator.
	CkList                      *libs,   // The libraries to generate.
	lua_State                   *script) // The lua script.
{
	CK_ARG_NON_NULL( pGen );
	CK_ARG_NON_NULL( pDhi );
	CK_ARG_NON_NULL( alloc );
	CK_ARG_NON_NULL( libs );
	CK_ARG_NON_NULL( script );

	pGen->alloc = alloc;
	pGen->libs = libs;
	pGen->pDhi = pDhi;
	pGen->script = script;
	CkStrBuilderCreate( pGen->outsb, 4096 );
}

// Disposes of a generator instance.
static inline void CkDisposeGenerator( CkGenerator *pGen )
{
	CK_ARG_NON_NULL( pGen );

	CkStrBuilderDispose( pGen->outsb );
}

// Generates the generator's code.
void CkGenerate( CkGenerator *pGen );

#ifdef SKIP

// Generates a program out. Disclaimer: both
// malloc()/free() and arena allocation
// are used by this function.
char *CkGenProgram(
	CkDiagnosticHandlerInstance* pDhi, // The diagnostics handler
	CkArenaFrame* allocator,           // Allocator for statically sized objects
	CkList *libraries,                 // Element type = CkLibrary*
	lua_State *script                  // The Lua state used as a gen script
	);

#endif
#endif
