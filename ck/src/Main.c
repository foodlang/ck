#include <include/Driver.h>
#include <include/FileIO.h>
#include <include/Food.h>
#include <include/Syntax/ParserTypes.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char *argv[], char **envp)
{
	CkDriverStartupConfiguration config;
	CkDriverCompilationResult result;

	puts("CK (Official Food Compiler)");
	puts("Copyright (C) 2023 The Food Project");
#if _DEBUG
	puts("This is a debug build of CK. The compiler might not perform as fast as release builds.");
#endif

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

		memset(&config, 0, sizeof(CkDriverStartupConfiguration));
		memset(&result, 0, sizeof(CkDriverCompilationResult));

		config.name = _malloca(len + 1);
		strcpy_s(config.name, len + 1, argv[i]);
		config.source = CkReadFileContents(config.name);
		config.wError = FALSE;
		CkDriverCompile(&result, &config);
		
		_freea(config.name);
		free(config.source);
	}

	return 0;
}
