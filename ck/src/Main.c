#include <include/Driver.h>
#include <include/Configs.h>
#include <include/Arena.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char *argv[], char **envp)
{
	/*CkDriverStartupConfiguration config;
	CkDriverCompilationResult result;
	CkArenaFrame arena;

	if (argc == 1 || (argc == 2 &&
		(
			!strcmp(argv[1], "--help")
			|| !strcmp(argv[1], "-h")
			|| !strcmp(argv[1], "--usage")
			))) {
		puts("Usage: ck [params] <source-file-1> <source-file-2> ...");
		puts("Parameters list:");
		puts("");
		puts("  --help, -h\tBrings up this message.");
		puts("  --usage\tSame as --help.");
		return 0;
	}

	(void)envp;

	for (int i = 1; i < argc; i++) {
		const size_t len = strnlen_s(argv[i], 256);
		CkArenaStartFrame(&arena, 0);

		memset(&config, 0, sizeof(CkDriverStartupConfiguration));
		memset(&result, 0, sizeof(CkDriverCompilationResult));

		config.name = CkArenaAllocate(&arena, len + 1);
		strcpy_s(config.name, len + 1, argv[i]);
		config.source = CkReadFileContents(&arena, config.name);
		config.wError = FALSE;
		CkDriverCompile(&arena, &result, &config);
		
		CkArenaResetFrame(&arena);
	}

	CkArenaEndFrame(&arena);*/

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
	(void)driverStart;
	(void)driverResult;

	CkArenaEndFrame(&configArena);
	CkArenaEndFrame(&driverArena);
	return 0;
}
