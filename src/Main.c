#include <Driver.h>
#include <Diagnostics.h>
#include <Configs.h>
#include <FileIO.h>
#include <Util/Time.h>
#include <Generation/GenPrototype.h>
#include <Defines.h>
#include <Analyzer.h>
#include <Memory/Arena.h>
#include <IL/FFStruct.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if _WIN32
#include <direct.h>

#define _OFF_T_DEFINED // for stat.h (stdlib bug)
typedef off_t _off_t;  // for stat.h (stdlib bug)
#endif

#include <sys/stat.h>

#if _WIN32
#define ported_mkdir(x) _mkdir(x)
#else
#define ported_mkdir(x) mkdir(x, 0x1FF)
#endif

#include <cwalk.h>

int main(int argc, char *argv[], char **envp)
{
	CkArenaFrame globalArena;                 // The arena used for global allocations.
	CkDriverStartupConfiguration driverStart; // Driver startup configuration
	CkDriverCompilationResult driverResult = {};// Driver result
	CkBuildConfig *base;                      // The base configuration.
	CkBuildConfig *applied;                   // The configuration with the profile applied.
	char *buildDirectory = NULL;              // The directory where the project is located.
	char *profileName = NULL;                 // The name of the profile to use.
	size_t sourceCount;                       // Used for iterating through all source files
	CkLibrary *result;                        // The resulting library.
	CkDiagnosticHandlerInstance dhi;          // Handles diagnostics.
	CkTimePoint tcompilerStart;               // The start of the compilation
	CkTimePoint tcompilerEnd;                 // The end of the compilation
	CkTimePoint tdriverStart;                 // The start of the driver parsing
	CkTimePoint tdriverEnd;                   // The end of the driver parsing
	bool success = true;                      // Compilation & linkage status
	CkList* libs;                             // Library list
	CkAnalyzeConfig analysis_cfg;             // Analysis config

	puts("CK, The Official Food Compiler");
	puts("Copyright (C) 2023 The Food Project");

#if defined(_DEBUG)
	puts("Compiler is running in debug mode!");
#endif

	puts("");

	if (argc == 1) {
		puts("Usage: ck <build_dir> <profile>");
		puts("\tbuild_dir\tThe directory where the build file is located.");
		puts("\tprofile\t\tThe name of the profile to use. This parameter is optional.");
		return 0;
	}

	CkArenaStartFrame(&globalArena, 0);
	CkTimeGetCurrent(&tcompilerStart);

	driverStart.defines = CkListStart(&globalArena, sizeof(CkMacro));
	CkDefineConstants(&globalArena, driverStart.defines);

	if (argc >= 2) buildDirectory = argv[1];
	if (argc >= 3) profileName = argv[2];
	if (argc > 3) puts("ck: Parameters beyond <profile> are ignored.");

	base = CkConfigGetBuildConfig(&globalArena, buildDirectory);
	if (!base) {
		puts("ck: Failed to parse build configuration, exiting.\n");
		CkArenaEndFrame(&globalArena);
		return -1;
	}

	applied = CkConfigApplied(&globalArena, base, profileName);
	if (!applied) {
		if (!profileName)
			puts("ck: A profile must be used for this configuration, as it lacks mandatory settings.\n");
		else
			puts("ck: The profile cannot be applied to the configuration, or it doesn't exist.\n");
		return -1;
	}

	result = CkCreateLibrary(&globalArena, applied->name);
	CkDiagnosticHandlerCreateInstance(&globalArena, &dhi);

	// __CC__

	sourceCount = CkListLength(applied->sources);
	for (size_t i = 0; i < sourceCount; i++) {
		char *source = CkConfigGetSource(applied, i);
		char *sourceTotal;
		size_t nameLen;
		size_t sourceLen;

		CkTimeGetCurrent(&tdriverStart);

		if (!source) {
			fprintf(stderr, "ck: Attempted to read out of bounds of file list.\n");
			abort();
		}

		// Init
		nameLen = strlen(applied->name) + strlen("::") + strlen(source);
		sourceLen = strlen(buildDirectory) + strlen(applied->sourceDir) + strlen("//") + strlen(source);
		memset(&driverResult, 0, sizeof(CkDriverCompilationResult));

		// Name (ProjectName::FileName)
		driverStart.name = CkArenaAllocate(&globalArena, nameLen + 1);
		strcpy(driverStart.name, applied->name);
		strcat(driverStart.name, "::");
		strcat(driverStart.name, source);

		// Source filepath (SourceDir/Filepath)
		sourceTotal = CkArenaAllocate(&globalArena, sourceLen + 1);
		cwk_path_join(buildDirectory, applied->sourceDir, sourceTotal, sourceLen + 1);
		cwk_path_join(sourceTotal, source, sourceTotal, sourceLen + 1);

		// Source loading
		driverStart.source = CkReadFileContents(&globalArena, sourceTotal);

		if (!driverStart.source) {
			fprintf(stderr, "ck: Project '%s' does not have source file '%s'.\n", applied->name, sourceTotal);
			continue;
		}

		// Loading settings
		driverStart.wError = applied->wError;

		// Driver compilation
		CkDriverCompile(&dhi, &globalArena, &globalArena, result, &driverResult, &driverStart);
		CkTimeGetCurrent(&tdriverEnd);
		if (dhi.anyErrors || (applied->wError && dhi.anyWarnings) || !driverResult.successful) {
			CkDiagnosticThrow(&dhi, NULL, CK_DIAG_SEVERITY_MESSAGE, "",
				"Semantic analysis/type binding will not be performed if parsing failed.");
			CkDiagnosticDisplay(&dhi);
			CkTimeGetCurrent(&tcompilerEnd);
			CkArenaEndFrame(&globalArena);
			printf(
				"Full compilation time: %f ms\n",
				(double)(CkTimeElapsed_mcs(&tcompilerStart, &tcompilerEnd)) / 1000.0);
			return 0;
		}
		printf("  - '%s' (%f ms)\n",
			sourceTotal,
			(double)(CkTimeElapsed_mcs(&tdriverStart, &tdriverEnd)) / 1000.0);
	}

	puts("");

	// Binding
	//CkBinderValidateAndBind( &dhi, &globalArena, result );
	analysis_cfg.allocator = &globalArena;
	analysis_cfg.p_dhi = &dhi;
	libs = CkListStart(&globalArena, sizeof(CkLibrary *));
	CkListAdd(libs, &result);
	CkAnalyzeFull(libs, &analysis_cfg);
	CkDiagnosticDisplay(&dhi);
	if (dhi.anyErrors || (applied->wError && dhi.anyWarnings))
		success = false;

	if (success) {
		FILE  *output;
		char  *output_path;
		size_t output_path_len;
		struct stat st;

		// ----- Path -----
		output_path_len =
			strlen(buildDirectory)
			+ strlen(applied->outDir)
			+ strlen("//")
			+ strlen(applied->name) + strlen(".asm");
		output_path = CkArenaAllocate(&globalArena, output_path_len + 1);
		cwk_path_join(buildDirectory, applied->outDir, output_path, output_path_len);
		if (stat(output_path, &st) == -1)
			ported_mkdir(output_path);
		cwk_path_join(output_path, applied->name, output_path, output_path_len);
		cwk_path_change_extension(output_path, ".asm", output_path, output_path_len);

		// ----- Opening and generating file -----
		output = fopen(output_path, "wb");
		fputs(CkGenProgram_Prototype(&dhi, &globalArena, libs), output);
		universal_fclose(output);
		printf("Output file: '%s'\n", output_path);
	}
	CkTimeGetCurrent(&tcompilerEnd);
	CkArenaEndFrame(&globalArena);
	printf(
		"Full compilation time: %f ms\n",
		(double)(CkTimeElapsed_mcs(&tcompilerStart, &tcompilerEnd)) / 1000.0);

	(void)envp;
	(void)success;

	return 0;
}
