/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines the configuration handler. The configuration handler
 * parses input build scripts and setups the drivers accordingly. It also
 * applies profiles if required.
 *
 ***************************************************************************/

#ifndef CK_CONFIGS_H_
#define CK_CONFIGS_H_

#include <Memory/List.h>

// The filename of a build config file, relative to the directory specified.
#define CK_BUILD_FILE_RELATIVE "__ckbuild.json"

// The type of the output binary.
typedef enum CkBuildOutput
{
	CK_BUILD_NOT_SET,

	// Indicates that the output binary should be an IL
	// library or executable that can be JIT'd by the Food
	// runtime.
	CK_BUILD_OUTPUT_UNIVERSAL,

	// Indicates that the output binary is a DLL/shared object.
	// Use this for custom binaries. Cannot be used with the
	// universal platform.
	CK_BUILD_OUTPUT_DYNAMIC_LIBRARY,

	// Indicates that the output binary is a native static library.
	// Cannot be used with the universal platform.
	CK_BUILD_OUTPUT_STATIC_LIBRARY,

	// Indicates that the output binary is a native executable.
	// Cannot be used with the universal platform.
	CK_BUILD_OUTPUT_EXECUTABLE,

	// Indicates that the output binary is a native object, to be linked
	// with an external linker. If you plan to use native static libraries,
	// use this.
	CK_BUILD_OUTPUT_NATIVE_OBJECT,

} CkBuildOutput;

// The configuration used to build a project.
typedef struct CkBuildConfig
{
	// The name of the project. If the build config descriptor is actually a
	// profile, this is the profile name.
	char *name;

	// The source directory.
	char *sourceDir;

	// The object/intermediate directory (compiler objects.)
	char *objDir;

	// The output directory (after linkage.)
	char *outDir;

	// The type of the output. Universal binaries require the universal
	// platform.
	CkBuildOutput binaryType;

	// The architecture/platform of the output code. This is
	// used to find the correct Lua generator. The universal
	// platform will skip the Lua generator, and serialize
	// directly the IL.
	char *platform;

	// The system of the output code.
	char *system;

	// If true, all warnings are considered errors.
	bool_t wError;

	// If true, the program will be compiled and linked
	// with debug symbols.
	bool_t debug;

	// The optimization level. 0 is the lowest, however
	// minor optimizations (such as expression simplification)
	// will still be performed.
	uint8_t optLevel;

	// A list of CkBuildConfig structs that store the
	// individual build profiles.
	CkList *profiles;

	// A list of relative source filepaths.
	CkList *sources;

	// A list of paths to Food libraries.
	CkList *libraries;

} CkBuildConfig;

// Fetches the build config from a directory.
CkBuildConfig *CkConfigGetBuildConfig(CkArenaFrame *allocator, char *directoryPath);

// Applies a profile to a configuration.
CkBuildConfig *CkConfigApplied(CkArenaFrame *allocator, CkBuildConfig *base, char *profileName);

// Gets a pointer to the start of the source filepath string.
char *CkConfigGetSource(CkBuildConfig *cfg, size_t index);

#endif
