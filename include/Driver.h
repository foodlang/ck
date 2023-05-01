/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the driver structs and functions. A compilation driver
 * takes in source code, parses the code and generates IL. Multiple drivers
 * should be runnable at the same time.
 *
 ***************************************************************************/

#ifndef CK_DRIVER_H_
#define CK_DRIVER_H_

#include <Memory/Arena.h>
#include <Syntax/Preprocessor.h>
#include <Diagnostics.h>
#include <Types.h>

#include <IL/FFStruct.h>

// Stores the configuration for a Ck compilation driver.
// This includes the source, the parsing and output settings.
typedef struct CkDriverStartupConfiguration
{
	char     *name;   // The name of the driver.
	CkSource *source; // The source code.
	bool_t    wError; // If true, warnings act as errors (prevent compilation).
	size_t    align;  // The alignment requirement.
	CkList   *defines;// A list to default defines. Elem type = CkMacro

} CkDriverStartupConfiguration;

// Returned by a driver after running. Contains debugging information,
// symbols and other information.
typedef struct CkDriverCompilationResult
{
	bool_t successful;    // If true, the compilation unit compiled successfully.
	double executionTime; // The time elapsed (in milliseconds) for the driver to run.

} CkDriverCompilationResult;

// Runs a compilation driver.
void CkDriverCompile(
	CkDiagnosticHandlerInstance *pDhi,
	CkArenaFrame *threadArena,
	CkArenaFrame *genArena,
	CkLibrary *lib,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig);

#endif
