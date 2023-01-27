#include <include/Diagnostics.h>
#include <include/CDebug.h>
#include <include/FileIO.h>

#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MAXDIAGLENGTH                 512
#define DIAGNOSTICPREFIXFORMAT        "ck %s issued from (L%d, C%d): "

void CkDiagnosticHandlerCreateInstance(
	CkArenaFrame *arena,
	CkDiagnosticHandlerInstance *dhi,
	CkLexInstance *pPassedLexer)
{
	CK_ARG_NON_NULL(dhi)
	CK_ARG_NON_NULL(arena)
	CK_ARG_NON_NULL(pPassedLexer)

	dhi->pPassedSource = pPassedLexer;
	dhi->arena = arena;
	dhi->anyErrors = FALSE;
	dhi->anyWarnings = FALSE;
	dhi->blacklistCount = 0;
	dhi->thrownDiagnosticsCount = 0;

	CkArenaStartFrame(&dhi->blacklistVector, 0);
	CkArenaStartFrame(&dhi->thrownDiagnosticsVector, 0);
}

void CkDiagnosticHandlerDestroyInstance(CkDiagnosticHandlerInstance *dhi)
{
	CK_ARG_NON_NULL(dhi)

	CkArenaEndFrame(&dhi->blacklistVector);
	CkArenaEndFrame(&dhi->thrownDiagnosticsVector);
}

void CkDiagnosticBlacklist(CkDiagnosticHandlerInstance *dhi, char *name)
{
	size_t nameLength;   // The length of the blacklist name entry.
	char   **destination;// The destination blacklist entry.
	char **blBase;

	CK_ARG_NON_NULL(dhi)
	CK_ARG_NON_NULL(name)
	
	blBase = dhi->blacklistVector.base;

	// 1. Preventing duplicate entries
	for (size_t i = 0; i < dhi->blacklistCount; i++) {
		if (!strcmp(blBase[i], name))
			return;
	}

	nameLength = strlen(name) + 1;
	destination = CkArenaAllocate(&dhi->blacklistVector, sizeof(char *));
	*destination = CkArenaAllocate(dhi->arena, nameLength);
	strcpy_s(*destination, nameLength, name);
	dhi->blacklistCount++;
}

void CkDiagnosticWhitelist(CkDiagnosticHandlerInstance *dhi, char *name)
{
	CK_ARG_NON_NULL(dhi)
	CK_ARG_NON_NULL(name)

	for (size_t i = 0; i < dhi->blacklistCount; i++) {
		char **base = (char **)dhi->blacklistVector.base + i;
		if (!strcmp(*base, name)) {
			*base = NULL;
			return;
		}
	}
}

void CkDiagnosticThrow(
	CkDiagnosticHandlerInstance *dhi,
	uint64_t position,
	uint8_t severity,
	char *name,
	const char *restrict format,
	...)
{
	va_list va_args;
	CkThrownDiagnostic *diag; // The diagnostic entry to write to

	char *prefixBuffer;   // The prefix buffer
	char *messageBuffer;  // The message buffer
	size_t prefixLength;  // The length of the prefix buffer
	size_t messageLength; // The length of the message buffer
	size_t totalLength;   // The diagnostic message length

	// position in 2D text coordinates
	size_t pos2DRow;
	size_t pos2DCol;

	const char *severityTxt; // The severity of the diagnostic, as text.

	CK_ARG_NON_NULL(dhi)
	CK_ARG_NON_NULL(name)
	CK_ARG_NON_NULL(format)

	switch (severity) {
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
	if (severity == CK_DIAG_SEVERITY_WARNING) {
		for (size_t i = 0; i < dhi->blacklistCount; i++) {
			// Blacklisted warnings are ignored
			char **base = (char **)dhi->blacklistVector.base + i;
			if (base == NULL) continue;
			if (!strcmp(name, *base))
				return;
		}
	}

	va_start(va_args, format);
	CkGetRowColString(dhi->pPassedSource->source, position, &pos2DRow, &pos2DCol);

	diag = CkArenaAllocate(&dhi->thrownDiagnosticsVector, sizeof(CkThrownDiagnostic));
	dhi->thrownDiagnosticsCount++;

	// Allocating and writing to the sub-buffers
	prefixBuffer = _malloca(MAXDIAGLENGTH);
	messageBuffer = _malloca(MAXDIAGLENGTH);
	sprintf_s(prefixBuffer, MAXDIAGLENGTH, DIAGNOSTICPREFIXFORMAT, severityTxt, pos2DRow, pos2DCol);
	vsnprintf_s(messageBuffer, MAXDIAGLENGTH, MAXDIAGLENGTH - 1, format, va_args);

	// Getting the length of the sub-buffers and final buffer length
	prefixLength = strnlen_s(prefixBuffer, MAXDIAGLENGTH);
	messageLength = strnlen_s(messageBuffer, MAXDIAGLENGTH);
	totalLength = prefixLength + messageLength;

	// Allocating and writing to the final buffer
	diag->message = CkArenaAllocate(dhi->arena, totalLength + 1);
	diag->severity = severity;
	strcpy_s(diag->message, totalLength + 1, prefixBuffer);
	strcat_s(diag->message, totalLength + 1, messageBuffer);
	_freea(prefixBuffer);
	_freea(messageBuffer);

	va_end(va_args);
}

void CkDiagnosticDisplay(CkDiagnosticHandlerInstance *dhi)
{
	CK_ARG_NON_NULL(dhi)
	for (size_t i = 0; i < dhi->thrownDiagnosticsCount; i++) {
		const CkThrownDiagnostic *diagnostic = (CkThrownDiagnostic *)dhi->thrownDiagnosticsVector.base + i;
		puts(diagnostic->message);
	}
}
