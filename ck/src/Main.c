#include <include/Driver.h>
#include <include/FileIO.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char *argv[])
{
	CkDriverStartupConfiguration config;
	CkDriverCompilationResult result;

	for (int i = 0; i < argc; i++) {
		const size_t len = strnlen_s(argv[i], 256);

		memset(&config, 0, sizeof(CkDriverStartupConfiguration));
		memset(&result, 0, sizeof(CkDriverCompilationResult));

		config.name = _malloca(len + 1);
		strcpy_s(config.name, len + 1, argv[i]);
		config.source = CkReadFileContents(config.name);
		CkDriverCompile(&result, &config);
		
		_freea(config.name);
		free(config.source);
	}

	return 0;
}
