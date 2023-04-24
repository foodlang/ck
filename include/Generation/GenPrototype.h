/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the prototype generator, which is a very basic
 * temporary generator written in C. Its purpose is to allow testing of
 * intermediate and frontend concepts and modules without requiring the Lua
 * backend system.
 * 
 * Target architecture/platform: x86-64 assembly
 *
 ***************************************************************************/

#ifndef CK_GEN_PROTOTYPE_H_
#define CK_GEN_PROTOTYPE_H_

#include "../Memory/List.h"
#include "../IL/FFStruct.h"
#include "../Diagnostics.h"

// Generates a program out. Disclaimer: both
// malloc()/free() and arena allocation
// are used by this function.
char* CkGenProgram_Prototype(
	CkDiagnosticHandlerInstance* pDhi, // The diagnostics handler
	CkArenaFrame* allocator,           // Allocator for statically sized objects
	CkList* libraries                  // Element type = CkLibrary*
);

#endif
