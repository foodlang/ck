/*
 * Provides functions and types for Ck compilation drivers.
*/

#ifndef CK_DRIVER_H_
#define CK_DRIVER_H_

#include <ckmem/Arena.h>
#include <ckmem/Types.h>

#include <fflib/FFStruct.h>

/// <summary>
/// Stores the configuration for a Ck compilation driver.
/// This includes the source, the parsing and output settings.
/// </summary>
typedef struct CkDriverStartupConfiguration
{
	/// <summary>
	/// The name of the driver.
	/// </summary>
	char *name;

	/// <summary>
	/// The source code.
	/// </summary>
	char *source;

	/// <summary>
	/// If true, warnings act as errors (prevent compilation).
	/// </summary>
	bool_t wError;

} CkDriverStartupConfiguration;

/// <summary>
/// Returned by a driver after running. Contains debugging information,
/// symbols and other information.
/// </summary>
typedef struct CkDriverCompilationResult
{
	/// <summary>
	/// If true, the compilation unit compiled successfully.
	/// </summary>
	bool_t successful;

	/// <summary>
	/// The time elapsed (in milliseconds) for the driver to run.
	/// </summary>
	double executionTime;

} CkDriverCompilationResult;

/// <summary>
/// Runs a compilation driver.
/// </summary>
/// <param name="threadArena">The arena for this thread of execution.</param>
/// <param name="result">The result of the driver.</param>
/// <param name="startupConfig">The startup configuration.</param>
void CkDriverCompile(
	CkArenaFrame *threadArena,
	CkArenaFrame *genArena,
	FFModule *temp_dest,
	CkDriverCompilationResult *result,
	CkDriverStartupConfiguration *startupConfig);

#endif
