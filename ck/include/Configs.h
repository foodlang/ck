/*
 * Provides functions and structures for compiler configuration files
 * and build configuration files.
*/

#ifndef CK_CONFIGS_H_
#define CK_CONFIGS_H_

#include "List.h"

// The filename of a build config file, relative to the directory specified.
#define CK_BUILD_FILE_RELATIVE "__ckbuild.json"

/// <summary>
/// The type of the output binary.
/// </summary>
typedef enum CkBuildOutput
{
	CK_BUILD_NOT_SET,

	/// <summary>
	/// Indicates that the output binary should be an IL
	/// library or executable that can be JIT'd by the Food
	/// runtime.
	/// </summary>
	CK_BUILD_OUTPUT_UNIVERSAL,

	/// <summary>
	/// Indicates that the output binary is a DLL/shared object.
	/// Use this for custom binaries. Cannot be used with the
	/// universal platform.
	/// </summary>
	CK_BUILD_OUTPUT_DYNAMIC_LIBRARY,

	/// <summary>
	/// Indicates that the output binary is a native static library.
	/// Cannot be used with the universal platform.
	/// </summary>
	CK_BUILD_OUTPUT_STATIC_LIBRARY,

	/// <summary>
	/// Indicates that the output binary is a native executable.
	/// Cannot be used with the universal platform.
	/// </summary>
	CK_BUILD_OUTPUT_EXECUTABLE,

	/// <summary>
	/// Indicates that the output binary is a native object, to be linked
	/// with an external linker. If you plan to use native static libraries,
	/// use this.
	/// </summary>
	CK_BUILD_OUTPUT_NATIVE_OBJECT,

} CkBuildOutput;

/// <summary>
/// The configuration used to build a project.
/// </summary>
typedef struct CkBuildConfig
{
	/// <summary>
	/// The name of the project. If the build config descriptor is actually a
	/// profile, this is the profile name.
	/// </summary>
	char *name;

	/// <summary>
	/// The source directory.
	/// </summary>
	char *sourceDir;

	/// <summary>
	/// The object/intermediate directory (compiler objects.)
	/// </summary>
	char *objDir;

	/// <summary>
	/// The output directory (after linkage.)
	/// </summary>
	char *outDir;

	/// <summary>
	/// The type of the output. Universal binaries require the universal
	/// platform.
	/// </summary>
	CkBuildOutput binaryType;

	/// <summary>
	/// The architecture/platform of the output code. This is
	/// used to find the correct Lua generator. The universal
	/// platform will skip the Lua generator, and serialize
	/// directly the IL.
	/// </summary>
	char *platform;

	/// <summary>
	/// The system of the output code.
	/// </summary>
	char *system;

	/// <summary>
	/// If true, all warnings are considered errors.
	/// </summary>
	bool_t wError;

	/// <summary>
	/// If true, the program will be compiled and linked
	/// with debug symbols.
	/// </summary>
	bool_t debug;

	/// <summary>
	/// The optimization level. 0 is the lowest, however
	/// minor optimizations (such as expression simplification)
	/// will still be performed.
	/// </summary>
	uint8_t optLevel;

	/// <summary>
	/// A list of CkBuildConfig structs that store the
	/// individual build profiles.
	/// </summary>
	CkList profiles;

	/// <summary>
	/// A list of relative source filepaths.
	/// </summary>
	CkList sources;

	/// <summary>
	/// A list of paths to Food libraries.
	/// </summary>
	CkList libraries;

} CkBuildConfig;

/// <summary>
/// Fetches the build config from a directory.
/// </summary>
/// <param name="directoryPath">The directory where the build file is located.</param>
/// <returns></returns>
CkBuildConfig *CkConfigGetBuildConfig(CkArenaFrame *allocator, char *directoryPath);

/// <summary>
/// Applies a profile to a configuration.
/// </summary>
/// <param name="allocator">The allocator used to allocate the new configuration.</param>
/// <param name="base">The base configuration.</param>
/// <param name="profileName">The name of the profile to be applied. NULL attempts to use the configuration without any profiles.</param>
/// <returns>A non-nested configuration that has a profile applied. Returns NULL if the profile is invalid.</returns>
CkBuildConfig *CkConfigApplied(CkArenaFrame *allocator, CkBuildConfig *base, char *profileName);

/// <summary>
/// Gets a pointer to the start of the source filepath string.
/// </summary>
/// <param name="cfg">The configuration to search through (applied.)</param>
/// <param name="index">The index of the string to fetch.</param>
/// <returns></returns>
char *CkConfigGetSource(CkBuildConfig *cfg, size_t index);

#endif
