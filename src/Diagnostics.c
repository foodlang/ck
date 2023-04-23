#include <Diagnostics.h>
#include <CDebug.h>
#include <FileIO.h>

#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MAXDIAGLENGTH                 512
#define DIAGNOSTICPREFIXFORMAT        "ck: %s issued from (%zu, %zu): "
#define DIAGNOSTICPREFIXFORMAT_1D     "ck: %s issued from (%zu): "

void CkDiagnosticHandlerCreateInstance(
	CkArenaFrame *arena,
	CkDiagnosticHandlerInstance *dhi,
	CkLexInstance *pPassedLexer )
{
	CK_ARG_NON_NULL( dhi );
	CK_ARG_NON_NULL( arena );
	CK_ARG_NON_NULL( pPassedLexer );

	dhi->pPassedSource = pPassedLexer;
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
	uint64_t position,
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
		severityTxt = "message";
		break;
	case CK_DIAG_SEVERITY_WARNING:
		severityTxt = "warning";
		break;
	case CK_DIAG_SEVERITY_ERROR:
		severityTxt = "error";
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
	CkGetRowColString( dhi->pPassedSource->source, position, &pos2DRow, &pos2DCol );

	// Allocating and writing to the sub-buffers
	sprintf( s_prefixBuffer, DIAGNOSTICPREFIXFORMAT, severityTxt, pos2DRow, pos2DCol );
	vsprintf( s_messageBuffer, format, va_args );

	// Getting the length of the sub-buffers and final buffer length
	prefixLength = strlen( s_prefixBuffer );
	messageLength = strlen( s_messageBuffer );
	totalLength = prefixLength + messageLength;

	// Allocating and writing to the final buffer
	diag.message = CkArenaAllocate( dhi->arena, totalLength + 1 );
	diag.severity = severity;
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
	CK_ARG_NON_NULL( dhi );
	thrownDiagnosticsLength = CkListLength( dhi->thrownDiagnosticsVector );
	for ( size_t i = 0; i < thrownDiagnosticsLength; i++ ) {
		const CkThrownDiagnostic *diagnostic = CkListAccess( dhi->thrownDiagnosticsVector, i );
		puts( diagnostic->message );
	}
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
