/*
 * Provides functions and types for Ck compilation drivers.
*/

#ifndef CK_DRIVER_H_
#define CK_DRIVER_H_

#include "Types.h"

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

} CkDriverCompilationResult;

/// <summary>
/// Runs a compilation driver.
/// </summary>
/// <param name="startupConfig"></param>
void CkDriverCompile(CkDriverCompilationResult *result, CkDriverStartupConfiguration *startupConfig);

#endif
