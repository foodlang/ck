#include <Driver.h>
#include <Diagnostics.h>
#include <Configs.h>
#include <FileIO.h>
#include <Util/Time.h>
#include <Generation/GenPrototype.h>

#include <Syntax/Binder.h>

#include <Memory/Arena.h>

#include <IL/FFStruct.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cwalk.h>

int main( int argc, char *argv[], char **envp )
{
	CkArenaFrame globalArena;                 // The arena used for global allocations.
	CkDriverStartupConfiguration driverStart; // Driver startup configuration
	CkDriverCompilationResult driverResult;   // Driver result
	CkBuildConfig *base;                      // The base configuration.
	CkBuildConfig *applied;                   // The configuration with the profile applied.
	char *buildDirectory = NULL;              // The directory where the project is located.
	char *profileName = NULL;                 // The name of the profile to use.
	size_t sourceCount;                       // Used for iterating through all source files
	CkLibrary *result;                        // The resulting library.
	CkModule *temp_driverModule;              // (TEMPORARY) The module for the current driver.
	CkDiagnosticHandlerInstance dhi;          // Handles diagnostics.
	CkTimePoint compilerStart;                // The start of the compilation
	CkTimePoint compilerEnd;                  // The end of the compilation
	bool_t success = TRUE;                    // Compilation & linkage status
	CkList* libs;                             // Library list

	puts( "CK, The Official Food Compiler" );
	puts( "Copyright (C) 2023 The Food Project" );

#if defined(_DEBUG)
	puts( "This is a debug build of CK. The compiler might not perform as fast as release builds." );
	puts( "  -> Please report any bugs you find at https://discord.gg/8SdtguX3P9 (#bug-reports channel.)" );
#endif

	puts( "" );

	if ( argc == 1 ) {
		puts( "Usage: ck <build_dir> <profile>" );
		puts( "\tbuild_dir\tThe directory where the build file is located." );
		puts( "\tprofile\t\tThe name of the profile to use. This parameter is optional." );
		return 0;
	}

	CkTimeGetCurrent( &compilerStart );
	CkArenaStartFrame( &globalArena, 0 );

	if ( argc >= 2 ) buildDirectory = argv[1];
	if ( argc >= 3 ) profileName = argv[2];
	if ( argc > 3 ) puts( "ck: Parameters beyond <profile> are ignored." );

	base = CkConfigGetBuildConfig( &globalArena, buildDirectory );
	if ( !base ) {
		puts( "ck: Failed to parse build configuration, exiting.\n" );
		CkArenaEndFrame( &globalArena );
		return -1;
	}

	applied = CkConfigApplied( &globalArena, base, profileName );
	if ( !applied ) {
		if ( !profileName )
			puts( "ck: A profile must be used for this configuration, as it lacks mandatory settings.\n" );
		else
			puts( "ck: The profile cannot be applied to the configuration, or it doesn't exist.\n" );
		return -1;
	}

	result = CkCreateLibrary( &globalArena, applied->name );

	sourceCount = CkListLength( applied->sources );
	for ( size_t i = 0; i < sourceCount; i++ ) {
		char *source = CkConfigGetSource( applied, i );
		char *sourceTotal;
		size_t nameLen;
		size_t sourceLen;

		if ( !source ) {
			fprintf( stderr, "ck: Attempted to read out of bounds of file list.\n" );
			abort();
		}

		// Init
		nameLen = strlen( applied->name ) + strlen( "::" ) + strlen( source );
		sourceLen = strlen( buildDirectory ) + strlen( applied->sourceDir ) + strlen( "//" ) + strlen( source );
		memset( &driverStart, 0, sizeof( CkDriverStartupConfiguration ) );
		memset( &driverResult, 0, sizeof( CkDriverCompilationResult ) );

		// Name (ProjectName::FileName)
		driverStart.name = CkArenaAllocate( &globalArena, nameLen + 1 );
		strcpy( driverStart.name, applied->name );
		strcat( driverStart.name, "::" );
		strcat( driverStart.name, source );

		// Source filepath (SourceDir/Filepath)
		sourceTotal = CkArenaAllocate( &globalArena, sourceLen + 1 );
		cwk_path_join( buildDirectory, applied->sourceDir, sourceTotal, sourceLen + 1 );
		cwk_path_join( sourceTotal, source, sourceTotal, sourceLen + 1 );

		printf( "ck: compiling file '%s'\n", sourceTotal );

		// Source loading
		driverStart.source = CkReadFileContents( &globalArena, sourceTotal );
		if ( !driverStart.source ) {
			fprintf( stderr, "ck: Project '%s' does not have source file '%s'.\n", applied->name, sourceTotal );
			continue;
		}

		// Loading settings
		driverStart.wError = applied->wError;

		// Driver compilation
		temp_driverModule = CkCreateModule( &globalArena, result, source, TRUE, TRUE );
		CkDriverCompile( &dhi, &globalArena, &globalArena, temp_driverModule, &driverResult, &driverStart );
	}

	// Binding
	CkBinderValidateAndBind( &dhi, &globalArena, result );
	CkDiagnosticDisplay( &dhi );
	if ( dhi.anyErrors || (applied->wError && dhi.anyWarnings) )
		success = FALSE;

	if ( success ) {
		libs = CkListStart( &globalArena, sizeof( CkLibrary* ) );
		CkListAdd( libs, &result );

		CkPrintAST( result );
		puts( CkGenProgram_Prototype( &dhi, &globalArena, libs ) );
	}
	CkArenaEndFrame( &globalArena );
	CkTimeGetCurrent( &compilerEnd );
	printf(
		"Full compilation time: %f ms\n",
		(double)(CkTimeElapsed_mcs(&compilerStart, &compilerEnd)) / 1000.0 );

	(void)envp;
	(void)success;

	return 0;
}
