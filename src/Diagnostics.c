#include <Diagnostics.h>
#include <CDebug.h>
#include <Util/EscapeCodes.h>
#include <FileIO.h>

#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MAXDIAGLENGTH                 4096
#define DIAGNOSTICPREFIXFORMAT        "%s issued from %s at (%zu, %zu): " ESCRESET
#define DIAGNOSTICPREFIXFORMAT_1D     "%s issued from %s at (%zu): " ESCRESET
#define DIAGNOSTICPREFIXFORMATNULL    "%s issued from the compiler: " ESCRESET

void CkDiagnosticHandlerCreateInstance(
	CkArenaFrame *arena,
	CkDiagnosticHandlerInstance *dhi )
{
	CK_ARG_NON_NULL( dhi );
	CK_ARG_NON_NULL( arena );
	
	dhi->arena = arena;
	dhi->anyErrors = FALSE;
	dhi->anyWarnings = FALSE;
	dhi->thrownTryMode = 0;
	dhi->tryMode = FALSE;
	
	dhi->blacklistVector = CkListStart( arena, sizeof( char * ) );
	dhi->thrownDiagnosticsVector = CkListStart( arena, sizeof( CkThrownDiagnostic ) );
}

void CkDiagnosticHandlerDestroyInstance( CkDiagnosticHandlerInstance *dhi )
{
	CK_ARG_NON_NULL( dhi );
}

void CkDiagnosticBlacklist( CkDiagnosticHandlerInstance *dhi, char *name )
{
	size_t nameLength;   // The length of the blacklist name entry.
	char *blacklist;   // The destination blacklist entry.
	size_t blacklistLength = CkListLength( dhi->blacklistVector );
	
	CK_ARG_NON_NULL( dhi );
	CK_ARG_NON_NULL( name );
	
	// 1. Preventing duplicate entries
	for ( size_t i = 0; i < blacklistLength; i++ ) {
		if ( !strcmp( CkListAccess( dhi->blacklistVector, i ), name ) )
			return;
	}
	
	nameLength = strlen( name ) + 1;
	blacklist = CkArenaAllocate( dhi->arena, nameLength );
	strcpy( blacklist, name );
	CkListAdd( dhi->blacklistVector, blacklist );
}

void CkDiagnosticWhitelist( CkDiagnosticHandlerInstance *dhi, char *name )
{
	size_t blacklistLength = CkListLength( dhi->blacklistVector );
	
	CK_ARG_NON_NULL( dhi );
	CK_ARG_NON_NULL( name );
	
	for ( size_t i = 0; i < blacklistLength; i++ ) {
		char *base = CkListAccess( dhi->blacklistVector, i );
		if ( !strcmp( base, name ) ) {
			dhi->blacklistVector = CkListRemove( dhi->blacklistVector, i );
			return;
		}
	}
}

static char s_prefixBuffer[MAXDIAGLENGTH];  // The prefix buffer
static char s_messageBuffer[MAXDIAGLENGTH]; // The message buffer

void CkDiagnosticThrow(
	CkDiagnosticHandlerInstance *dhi,
	CkToken *source,
	uint8_t severity,
	char *name,
	const char *restrict format,
	... )
{
	va_list va_args;
	CkThrownDiagnostic diag; // The diagnostic entry to write to
	size_t blacklistCount = 0;
	
	size_t prefixLength;                 // The length of the prefix buffer
	size_t messageLength;                // The length of the message buffer
	size_t totalLength;                  // The diagnostic message length
	
	// position in 2D text coordinates
	size_t pos2DRow;
	size_t pos2DCol;
	
	const char *severityTxt; // The severity of the diagnostic, as text.
	
	CK_ARG_NON_NULL( dhi );
	CK_ARG_NON_NULL( name );
	CK_ARG_NON_NULL( format );
	
	if ( dhi->tryMode && severity == CK_DIAG_SEVERITY_ERROR ) {
		dhi->thrownTryMode += 1;
		return;
	}
	
	switch ( severity ) {
	case CK_DIAG_SEVERITY_MESSAGE:
		severityTxt = ESCB256F CYAN256 "message";
		break;
	case CK_DIAG_SEVERITY_WARNING:
		severityTxt = ESCB256F YELLOW256 "warning";
		dhi->anyWarnings = TRUE;
		break;
	case CK_DIAG_SEVERITY_ERROR:
		severityTxt = ESCB256F RED256 "error";
		dhi->anyErrors = TRUE;
		break;
		default:
		abort();
	}
	
	// Blacklist check
	if ( severity == CK_DIAG_SEVERITY_WARNING ) {
		blacklistCount = CkListLength( dhi->blacklistVector );
		for ( size_t i = 0; i < blacklistCount; i++ ) {
			// Blacklisted warnings are ignored
			char **base = CkListAccess( dhi->blacklistVector, i );
			if ( !strcmp( name, *base ) )
				return;
		}
	}

	va_start( va_args, format );
	
	if ( !source ) {
		sprintf(
			s_prefixBuffer,
			DIAGNOSTICPREFIXFORMATNULL,
			severityTxt );
		vsprintf( s_messageBuffer, format, va_args );
	} else {
		CkGetRowColString( source->source->code, source->position, &pos2DRow, &pos2DCol );
		diag.line = pos2DRow;
		diag.column = pos2DCol;

		// Allocating and writing to the sub-buffers
		sprintf(
			s_prefixBuffer,
			DIAGNOSTICPREFIXFORMAT,
			severityTxt,
			source->source->filename,
			pos2DRow,
			pos2DCol );
		vsprintf( s_messageBuffer, format, va_args );
	}
	
	// Getting the length of the sub-buffers and final buffer length
	prefixLength = strlen( s_prefixBuffer );
	messageLength = strlen( s_messageBuffer );
	totalLength = prefixLength + messageLength;
	
	// Allocating and writing to the final buffer
	diag.message = CkArenaAllocate( dhi->arena, totalLength + 1 );
	diag.severity = severity;
	diag.source = source->source;
	strcpy( diag.message, s_prefixBuffer );
	strcat( diag.message, s_messageBuffer );
	memset( s_prefixBuffer, 0, MAXDIAGLENGTH );
	memset( s_messageBuffer, 0, MAXDIAGLENGTH );
	
	CkListAdd( dhi->thrownDiagnosticsVector, &diag );
	
	va_end( va_args );
}

void CkDiagnosticDisplay( CkDiagnosticHandlerInstance *dhi )
{
	size_t thrownDiagnosticsLength = 0;
	size_t lines_array_len = 0;
	char **lines_array = NULL;
	CkSource *prev_source = NULL;
	CK_ARG_NON_NULL( dhi );
	thrownDiagnosticsLength = CkListLength( dhi->thrownDiagnosticsVector );
	for ( size_t i = 0; i < thrownDiagnosticsLength; i++ ) {
		char *curline;
		char *prevline;

		const CkThrownDiagnostic *diagnostic = CkListAccess( dhi->thrownDiagnosticsVector, i );
		puts( diagnostic->message );

		// ----- Loading lines -----
		/*
		 * TODO: Optimize this
		 * Currently, we can only keep the same line array if
		 * the currently thrown diagnostic has the same source
		 * as the previous one.
		*/

		if ( diagnostic->source ) {
			puts( "" );
			if ( prev_source != diagnostic->source ) {
				prev_source = diagnostic->source;
				if ( lines_array ) CkFreeLines( lines_array, lines_array_len );
				lines_array = CkGetLinesFreeable( diagnostic->source->code, &lines_array_len );
			}
			curline = lines_array[diagnostic->line - 1];
			prevline = diagnostic->line >= 2 ? lines_array[diagnostic->line - 2] : NULL;
			if ( prevline )
				printf( ESCB256F CYAN256 "%zu %s\n" ESCRESET, diagnostic->line - 1, prevline );
			printf( ESCB256F CYAN256 "%zu %s\n" ESCRESET, diagnostic->line, curline );
			printf( ESCB256F MAGENTA256 );
			for ( size_t i = 0; i < diagnostic->column - 1; i++ ) {
				if ( curline[i - 1] == '\t' )
					printf( "~~~~~~~~" );
				else putc( '~', stdout );
			}
			putc( '^', stdout );
			printf( ESCRESET );
			putc( '\n', stdout );
		}
	}

	if ( lines_array ) CkFreeLines( lines_array, lines_array_len );
}

void CkDiagnosticBeginTryMode( CkDiagnosticHandlerInstance *dhi )
{
	if ( !dhi->tryMode ) {
		dhi->tryMode = TRUE;
		dhi->thrownTryMode = 0;
	} else {
		fprintf(stderr,  "ck internal error: Attempting to set try mode on diagnostic handler while already on try mode.\n" );
		abort();
	}
}

bool_t CkDiagnosticEndTryMode( CkDiagnosticHandlerInstance *dhi )
{
	if ( dhi->tryMode ) {
		dhi->tryMode = FALSE;
		if ( dhi->thrownTryMode != 0 ) {
			dhi->thrownTryMode = 0;
			return FALSE;
		}
		return TRUE;
	}
	// No try mode enabled
	return FALSE;
}

void CkDiagnosticClear( CkDiagnosticHandlerInstance* dhi )
{
	const size_t len = CkListLength( dhi->thrownDiagnosticsVector );
	for ( size_t i = 0; i < len; i++ )
		CkListRemove( dhi->thrownDiagnosticsVector, i );
}
