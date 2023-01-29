#include <include/Driver.h>
#include <include/Configs.h>
#include <include/Arena.h>
#include <include/FileIO.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cwalk.h>

int main(int argc, char *argv[], char **envp)
{
	CkArenaFrame configArena; // The arena used for config/profile-related allocations.
	CkArenaFrame driverArena; // The arena used for driver allocations. Reset for each driver.

	CkDriverStartupConfiguration driverStart; // Driver startup configuration
	CkDriverCompilationResult driverResult;   // Driver result

	CkBuildConfig *base;    // The base configuration.
	CkBuildConfig *applied; // The configuration with the profile applied.

	char *buildDirectory = NULL; // The directory where the project is located.
	char *profileName = NULL;    // The name of the profile to use.

	puts("CK, The Official Food Compiler");
	puts("Copyright (C) 2023 The Food Project & outside contributors");
#if _DEBUG
	puts("This is a debug build of CK. The compiler might not perform as fast as release builds.");
#endif

	if (argc == 1) {
		puts("Usage: ck <build_dir> <profile>");
		puts("\tbuild_dir\tThe directory where the build file is located.");
		puts("\tprofile\tThe name of the profile to use. This parameter is optional.");
		return 0;
	}

	CkArenaStartFrame(&configArena, 0);
	CkArenaStartFrame(&driverArena, 0);

	if (argc >= 2) buildDirectory = argv[1];
	if (argc >= 3) profileName = argv[2];
	if (argc > 3) puts("ck: Parameters beyond <profile> are ignored.");

	base = CkConfigGetBuildConfig(&configArena, buildDirectory);
	if (!base) {
		puts("ck: Failed to parse build configuration, exiting.\n");
		CkArenaEndFrame(&configArena);
		CkArenaEndFrame(&driverArena);
		return -1;
	}

	applied = CkConfigApplied(&configArena, base, profileName);
	if (!applied) {
		if (!profileName)
			puts("ck: A profile must be used for this configuration, as it lacks mandatory settings.\n");
		else
			puts("ck: The profile cannot be applied to the configuration, or it doesn't exist.\n");
		return -1;
	}

	(void)envp;

	CkArenaStartFrame(&driverArena, 0);
	for (size_t i = 0; i < applied->sourceCount; i++) {
		const char *source = CkConfigGetSource(applied, i);
		char *sourceTotal;
		size_t nameLen;
		size_t sourceLen;

		if (!source) {
			fprintf_s(stderr, "ck: Attempted to read out of bounds of file list.\n");
			abort();
		}

		// Init
		nameLen = strlen(applied->name) + strlen("::") + strlen(source);
		sourceLen = strlen(buildDirectory) + strlen(applied->sourceDir) + strlen("//") + strlen(source);
		memset(&driverStart, 0, sizeof(CkDriverStartupConfiguration));
		memset(&driverResult, 0, sizeof(CkDriverCompilationResult));

		// Name (ProjectName::FileName)
		driverStart.name = CkArenaAllocate(&driverArena, nameLen + 1);
		strcpy_s(driverStart.name, nameLen + 1, applied->name);
		strcat_s(driverStart.name, nameLen + 1, "::");
		strcat_s(driverStart.name, nameLen + 1, source);

		// Source filepath (SourceDir/Filepath)
		sourceTotal = CkArenaAllocate(&driverArena, sourceLen + 1);
		cwk_path_join(buildDirectory, applied->sourceDir, sourceTotal, sourceLen + 1);
		cwk_path_join(sourceTotal, source, sourceTotal, sourceLen + 1);

		// Source loading
		driverStart.source = CkReadFileContents(&driverArena, sourceTotal);
		if (!driverStart.source) {
			fprintf_s(stderr, "ck: Project '%s' does not have source file '%s'.\n", applied->name, sourceTotal);
			CkArenaResetFrame(&driverArena);
			continue;
		}

		// Loading settings
		driverStart.wError = applied->wError;

		// Driver compilation
		CkDriverCompile(&driverArena, &driverResult, &driverStart);

		// Resetting the frame for the next file
		CkArenaResetFrame(&driverArena);
	}

	CkArenaEndFrame(&configArena);
	CkArenaEndFrame(&driverArena);
	return 0;
}
