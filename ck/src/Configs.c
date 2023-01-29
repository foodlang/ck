#include <include/Configs.h>
#include <include/CDebug.h>
#include <include/FileIO.h>

#include <cwalk.h>
#include <cJSON.h>

#include <string.h>
#include <stdio.h>

#if _WIN32
#define MAXPATHLENGTH 260
#else
#define MAXPATHLENGTH 4096
#endif

#define CONFIGOBJECT_NAME       "name"
#define CONFIGOBJECT_SOURCEDIR  "sourceDirectory"
#define CONFIGOBJECT_OBJDIR     "objectDirectory"
#define CONFIGOBJECT_OUTPUTDIR  "outputDirectory"
#define CONFIGOBJECT_BINARYTYPE "binaryType"
#define CONFIGOBJECT_PLATFORM   "platform"
#define CONFIGOBJECT_SYSTEM     "system"
#define CONFIGOBJECT_WERROR     "warningsAsErrors"
#define CONFIGOBJECT_DEBUG      "debug"
#define CONFIGOBJECT_OPTLEVEL   "optimizationLevel"
#define CONFIGOBJECT_PROFILES   "profiles"
#define CONFIGOBJECT_SOURCES    "sources"
#define CONFIGOBJECT_LIBRARIES  "libraries"

/// <summary>
/// Stores a string from a JSON.
/// </summary>
/// <param name="dest">A pointer to the destination string.</param>
/// <param name="j"></param>
static inline char *s_StoreStringFromJson(CkArenaFrame *arena, cJSON *j)
{
	const size_t jLength = strlen(j->valuestring) + 1;
	char *yield = CkArenaAllocate(arena, jLength);
	strcpy_s(yield, jLength, cJSON_GetStringValue(j));
	return yield;
}

/// <summary>
/// Duplicates a string.
/// </summary>
/// <param name="arena">The arena to use for allocations.</param>
/// <param name="source">The source string.</param>
/// <returns></returns>
static inline char *s_StringDuplicate(CkArenaFrame *arena, char *source)
{
	const size_t len = strlen(source) + 1;
	char *yield = CkArenaAllocate(arena, len);
	strcpy_s(yield, len, source);
	return yield;
}

/// <summary>
/// Populates a configuration descriptor from a JSON object.
/// </summary>
/// <param name="jConfig">The JSON object.</param>
/// <param name="config">A pointer to the configuration descriptor.</param>
/// <param name="profile">The parent configuration to the profile. If NULL, parsing configuration.</param>
static bool_t s_PopulateConfig(
	CkArenaFrame *arena,
	cJSON *jConfig,
	CkBuildConfig *config,
	CkBuildConfig *parent)
{
	cJSON *jName;          // Target project/profile name. Mandatory.
	cJSON *jSourceDir;     // Target source directory. Mandatory to have a value in all profiles.
	cJSON *jObjDir;        // Target object directory. Mandatory to have a value in all profiles.
	cJSON *jOutDir;        // Target output directory. Mandatory to have a value in all profiles.
	cJSON *jBinaryType;    // Target binary type. Mandatory to have a value in all profiles.
	cJSON *jPlatform;      // Target platform. Mandatory to have a value in all profiles.
	cJSON *jSystem;        // Target system. Mandatory to have a value in all profiles.
	cJSON *jWError;        // wError. Optional, defaults to false.
	cJSON *jDebug;         // Debug. Optional, defaults to false.
	cJSON *jOptLevel;      // Optimization level. Optional, defaults to 1.
	cJSON *jSourcesRoot;   // Root of the sources. Mandatory to have a value in all profiles.
	cJSON *jLibrariesRoot; // Root of the libraries. Optional.
	cJSON *jProfilesRoot;  // Root of the profiles. Optional.

	jName          = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_NAME);
	jSourceDir     = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_SOURCEDIR);
	jObjDir        = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_OBJDIR);
	jOutDir        = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_OUTPUTDIR);
	jBinaryType    = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_BINARYTYPE);
	jPlatform      = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_PLATFORM);
	jSystem        = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_SYSTEM);
	jWError        = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_WERROR);
	jDebug         = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_DEBUG);
	jOptLevel      = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_OPTLEVEL);
	jSourcesRoot   = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_SOURCES);
	jLibrariesRoot = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_LIBRARIES);
	jProfilesRoot  = cJSON_GetObjectItemCaseSensitive(jConfig, CONFIGOBJECT_PROFILES);

#define CHECKJ_STRING(X, Y) if (!cJSON_IsString(X)) { fprintf_s(stderr, "ck: %s must be a string.\n", Y); return FALSE; }
#define CHECKJ_INT(X, Y)    if (!cJSON_IsNumber(X)) { fprintf_s(stderr, "ck: %s must be an integer.\n", Y); return FALSE; }
#define CHECKJ_BOOL(X, Y)   if (!cJSON_IsBool(X)) { fprintf_s(stderr, "ck: %s must be a boolean.\n", Y); return FALSE; }
#define CHECKJ_ARRAY(X, Y)  if (!cJSON_IsArray(X)) { fprintf_s(stderr, "ck: %s must be an array.\n", Y); return FALSE; }

	if (!jName) {
		fprintf_s(stderr, "ck: Build configuration or profile has no name.\n");
		return FALSE;
	}
	CHECKJ_STRING(jName, CONFIGOBJECT_NAME);
	config->name = s_StoreStringFromJson(arena, jName);

	if (parent && ( !jSourceDir && !parent->sourceDir )) {
		fprintf_s(stderr, "ck: A profile has no source directory.\n");
		return FALSE;
	}
	if (jSourceDir) {
		if (!cJSON_IsString(jSourceDir)) {
			fprintf_s(stderr, "ck: %s must be a string.\n", CONFIGOBJECT_SOURCEDIR);
			return FALSE;
		}
		config->sourceDir = s_StoreStringFromJson(arena, jSourceDir);
	}

	if (parent && ( !jObjDir && !parent->objDir )) {
		fprintf_s(stderr, "ck: A profile has no object directory.\n");
		return FALSE;
	}
	if (jObjDir) {
		CHECKJ_STRING(jObjDir, CONFIGOBJECT_OBJDIR);
		config->objDir = s_StoreStringFromJson(arena, jObjDir);
	}

	if (parent && ( !jOutDir && !parent->outDir )) {
		fprintf_s(stderr, "ck: A profile has no output directory.\n");
		return FALSE;
	}
	if (jOutDir) {
		CHECKJ_STRING(jOutDir, CONFIGOBJECT_OUTPUTDIR);
		config->outDir = s_StoreStringFromJson(arena, jOutDir);
	}

	if (parent && ( !jBinaryType && !parent->binaryType )) {
		fprintf_s(stderr, "ck: A profile has no output binary type.\n");
		return FALSE;
	}
	if (jBinaryType) {
		// TODO: Add support for binary type to be a string
		CHECKJ_INT(jBinaryType, CONFIGOBJECT_BINARYTYPE);
		config->binaryType = (int8_t)cJSON_GetNumberValue(jBinaryType);
		if (!config->binaryType) {
			fprintf(stderr, "ck: Binary type cannot be zero.\n");
			return FALSE;
		}
	}

	if (parent && ( !jPlatform && !parent->platform )) {
		fprintf_s(stderr, "ck: A profile has no target platform.\n");
		return FALSE;
	}
	if (jPlatform) {
		CHECKJ_STRING(jPlatform, CONFIGOBJECT_PLATFORM);
		config->platform = s_StoreStringFromJson(arena, jPlatform);
	}

	if (parent && ( !jSystem && !parent->system )) {
		fprintf_s(stderr, "ck: A profile has no target system.\n");
		return FALSE;
	}
	if (jSystem) {
		CHECKJ_STRING(jSystem, CONFIGOBJECT_SYSTEM);
		config->system = s_StoreStringFromJson(arena, jSystem);
	}

	if (jWError) {
		CHECKJ_BOOL(jWError, CONFIGOBJECT_WERROR);
		config->wError = jWError->type == cJSON_True ? TRUE : FALSE;
	} else config->wError = FALSE;

	if (jDebug) {
		CHECKJ_BOOL(jDebug, CONFIGOBJECT_WERROR);
		config->debug = jDebug->type == cJSON_True ? TRUE : FALSE;
	} else config->debug = FALSE;

	if (jOptLevel) {
		CHECKJ_INT(jOptLevel, CONFIGOBJECT_OPTLEVEL);
		config->optLevel = (uint8_t)cJSON_GetNumberValue(jOptLevel);
	} else config->optLevel = 1;

	if (parent && ( !jSourcesRoot && (parent->sourceCount == 0) )) {
		fprintf_s(stderr, "ck: A configuration or profile must have source files.\n");
		return FALSE;
	}
	if (jSourcesRoot) {
		cJSON *sourcePath;

		CHECKJ_ARRAY(jSourcesRoot, CONFIGOBJECT_SOURCES);
		config->sourceCount = (size_t)cJSON_GetArraySize(jSourcesRoot);
		CkArenaStartFrame(&config->sources, 0);
		
		cJSON_ArrayForEach(sourcePath, jSourcesRoot) {
			char *jStr = cJSON_GetStringValue(sourcePath);
			char *dest;
			if (!cJSON_IsString(sourcePath)) {
				fprintf_s(stderr, "ck: A source path must be a string.\n");
				config->sourceCount = 0;
				CkArenaWriteLock(&config->sources);
				return FALSE;
			}
			dest = CkArenaAllocate(&config->sources, strlen(jStr) + 1);
			strcpy_s(dest, strlen(jStr) + 1, jStr);
		}
		CkArenaWriteLock(&config->sources);
	}

	if (jLibrariesRoot) {
		cJSON *libraryPath;

		CHECKJ_ARRAY(jLibrariesRoot, CONFIGOBJECT_LIBRARIES);
		config->libraryCount = (size_t)cJSON_GetArraySize(jLibrariesRoot);
		CkArenaStartFrame(&config->libraries, 0);

		cJSON_ArrayForEach(libraryPath, jLibrariesRoot) {
			char *jStr = cJSON_GetStringValue(libraryPath);
			char *dest;
			if (!cJSON_IsString(libraryPath)) {
				fprintf_s(stderr, "ck: A library path must be a string.\n");
				config->libraryCount = 0;
				CkArenaWriteLock(&config->libraries);
				return FALSE;
			}
			dest = CkArenaAllocate(&config->libraries, strlen(jStr) + 1);
			strcpy_s(dest, strlen(jStr) + 1, jStr);
		}
		CkArenaWriteLock(&config->libraries);
	}

	if (jProfilesRoot) {
		cJSON *profile;

		if (parent) {
			fprintf_s(stderr, "ck: Cannot nest profiles.\n");
			return FALSE;
		}
		CHECKJ_ARRAY(jProfilesRoot, CONFIGOBJECT_PROFILES);
		config->profileCount = (size_t)cJSON_GetArraySize(jProfilesRoot);
		CkArenaStartFrame(&config->profiles, 0);

		cJSON_ArrayForEach(profile, jProfilesRoot) {
			CkBuildConfig *dest;
			dest = CkArenaAllocate(&config->profiles, sizeof(CkBuildConfig));
			if (!s_PopulateConfig(arena, profile, dest, config)) {
				config->profileCount = 0;
				CkArenaWriteLock(&config->profiles);
				return FALSE;
			}
		}
		CkArenaWriteLock(&config->profiles);
	}

	return TRUE;
}

CkBuildConfig *CkConfigGetBuildConfig(
	CkArenaFrame *allocator,
	char *directoryPath)
{
	char *filepathBuffer;
	char *fileConfigBuffer;
	CkBuildConfig *config;
	cJSON *jsonConfig;

	CK_ARG_NON_NULL(allocator);
	CK_ARG_NON_NULL(directoryPath);

	// Building the path
	filepathBuffer = CkArenaAllocate(allocator, MAXPATHLENGTH);
	cwk_path_normalize(directoryPath, filepathBuffer, MAXPATHLENGTH);
	cwk_path_join(filepathBuffer, CK_BUILD_FILE_RELATIVE, filepathBuffer, MAXPATHLENGTH);

	// Reading file
	fileConfigBuffer = CkReadFileContents(allocator, filepathBuffer);
	if (!fileConfigBuffer) // CkReadFileContents() already generates an error message
		return NULL;

	// Parsing JSON
	jsonConfig = cJSON_Parse(fileConfigBuffer);
	if (!jsonConfig) {
		fprintf_s(stderr, "ck: cJSON failed to read JSON file '%s':\n%s\n", filepathBuffer, cJSON_GetErrorPtr());
		return NULL;
	}

	// Creating configuration descriptor
	config = CkArenaAllocate(allocator, sizeof(CkBuildConfig));
	if (!s_PopulateConfig(allocator, jsonConfig, config, NULL)) {
		// s_PopulateConfig() already generates an error message
		cJSON_Delete(jsonConfig);
		return NULL;
	}
	
	cJSON_Delete(jsonConfig); // cJSON uses manual memory management
	return config;
}

#define BUILDCONFIG_SIZE_ALIGNED ( sizeof(CkBuildConfig) + (-sizeof(CkBuildConfig) & (ARENA_ALLOC_ALIGN - 1)) )

CkBuildConfig *CkConfigApplied(
	CkArenaFrame *allocator,
	CkBuildConfig *base,
	char *profileName)
{
	CkBuildConfig *total;
	CkBuildConfig *currentProfile;
	CkBuildConfig *selected = NULL;

	char *baseSourcesToWrite;
	char *selectedSourcesToWrite;
	char *baseLibrariesToWrite;
	char *selectedLibrariesToWrite;

	if (!profileName) goto LSkipLookup;
	for (size_t i = 0; i < base->profileCount; i++) {
		currentProfile = (CkBuildConfig *)((char *)(base->profiles.base) + BUILDCONFIG_SIZE_ALIGNED * i);
		if (!strcmp(currentProfile->name, profileName)) {
			selected = currentProfile;
			break;
		}
	}

LSkipLookup:
	if (!selected) // Everything's set to 0..
		selected = CkArenaAllocate(allocator, sizeof(CkBuildConfig));
	total = CkArenaAllocate(allocator, sizeof(CkBuildConfig));
	total->name = s_StringDuplicate(allocator, base->name);

	if (selected->sourceDir)
		total->sourceDir = s_StringDuplicate(allocator, selected->sourceDir);
	else total->sourceDir = s_StringDuplicate(allocator, base->sourceDir);

	if (selected->objDir)
		total->objDir = s_StringDuplicate(allocator, selected->objDir);
	else total->objDir = s_StringDuplicate(allocator, base->objDir);

	if (selected->outDir)
		total->outDir = s_StringDuplicate(allocator, selected->outDir);
	else total->outDir = s_StringDuplicate(allocator, base->outDir);

	total->binaryType = selected->binaryType ? selected->binaryType : base->binaryType;

	if (selected->platform)
		total->platform = s_StringDuplicate(allocator, selected->platform);
	else total->platform = s_StringDuplicate(allocator, base->platform);

	if (selected->system)
		total->system = s_StringDuplicate(allocator, selected->system);
	else total->system = s_StringDuplicate(allocator, base->system);

	total->wError = selected->wError ? selected->wError : base->wError;
	total->debug = selected->debug ? selected->debug : base->debug;
	total->optLevel = selected->optLevel ? selected->optLevel : base->optLevel;

	// Sources
	CkArenaStartFrame(&total->sources, base->sources.offsetFree + selected->sources.offsetFree);
	if (base->sourceCount) {
		baseSourcesToWrite = CkArenaAllocate(&total->sources, base->sources.offsetFree);
		memcpy_s(baseSourcesToWrite, base->sources.offsetFree, base->sources.base, base->sources.offsetFree);
	}
	if (selected->sourceCount) {
		selectedSourcesToWrite = CkArenaAllocate(&total->sources, selected->sources.offsetFree);
		memcpy_s(selectedSourcesToWrite, selected->sources.offsetFree, selected->sources.base, selected->sources.offsetFree);
	}
	CkArenaWriteLock(&total->sources);
	total->sourceCount = base->sourceCount + selected->sourceCount;

	// Libraries
	CkArenaStartFrame(&total->libraries, base->libraries.offsetFree + selected->libraries.offsetFree);
	if (base->libraryCount) {
		baseLibrariesToWrite = CkArenaAllocate(&total->libraries, base->libraries.offsetFree);
		memcpy_s(baseLibrariesToWrite, base->libraries.offsetFree, base->libraries.base, base->libraries.offsetFree);
	}
	if (selected->libraryCount) {
		selectedLibrariesToWrite = CkArenaAllocate(&total->libraries, selected->libraries.offsetFree);
		memcpy_s(selectedLibrariesToWrite, selected->libraries.offsetFree, selected->libraries.base, selected->libraries.offsetFree);
	}
	CkArenaWriteLock(&total->libraries);
	total->libraryCount = base->libraryCount + selected->libraryCount;

	return total;
}

char *CkConfigGetSource(CkBuildConfig *cfg, size_t index)
{
	char *cPtr = cfg->sources.base;
	size_t strCounter = 0;
	bool_t walkingThroughAlignment = FALSE;

	if (cfg->sourceCount <= index) {
		fprintf_s(stderr, "ck: Project '%s' doesn't have %llu source files.\n", cfg->name, index);
		return NULL;
	}

	if (index == 0) return cPtr;
	
	while (strCounter < index) {
		if (!*cPtr && !walkingThroughAlignment) {
			cPtr++;
			walkingThroughAlignment = TRUE;
		}

		if (!*cPtr && walkingThroughAlignment) {
			cPtr++;
			continue;
		}

		if (*cPtr && walkingThroughAlignment) {
			strCounter++;
			walkingThroughAlignment = FALSE;
			continue;
		}

		cPtr++;
		walkingThroughAlignment = FALSE;
	}

	return cPtr;
}
