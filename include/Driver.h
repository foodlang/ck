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
#include <Diagnostics.h>
#include <Types.h>

#include <IL/FFStruct.h>

// Stores the configuration for a Ck compilation driver.
// This includes the source, the parsing and output settings.
typedef struct CkDriverStartupConfiguration
{
	// The name of the driver.
	char *name;

	// The source code.
	char *source;

	// If true, warnings act as errors (prevent compilation).
	bool_t wError;

	// The alignment requirement.
	size_t align;

} CkDriverStartupConfiguration;

// Returned by a driver after running. Contains debugging information,
// symbols and other information.
typedef struct CkDriverCompilationResult
{
	// If true, the compilation unit compiled successfully.
	bool_t successful;

	// The time elapsed (in milliseconds) for the driver to run.
	double executionTime;

} CkDriverCompilationResult;

// Runs a compilation driver.
void CkDriverCompile(
	CkDiagnosticHandlerInstance *pDhi,
	CkArenaFrame *threadArena,
	CkArenaFrame *genArena,
	CkModule *temp_dest,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig);

#endif
