#include <include/Diagnostics.h>
#include <include/CDebug.h>
#include <include/FileIO.h>

#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define BLACKLISTVECTOR_ALLOCBLOCK    16
#define THROWNDIAGNOSTICS_ALLOCKBLOCK 16
#define MAXDIAGLENGTH                 512
#define DIAGNOSTICPREFIXFORMAT        "ck %s issued from (L%d, C%d): "

void CkDiagnosticHandlerCreateInstance(
	CkDiagnosticHandlerInstance *dhi,
	CkLexInstance *pPassedLexer)
{
	memset(dhi, 0, sizeof(CkDiagnosticHandlerInstance));
	dhi->pPassedSource = pPassedLexer;

	// 1. Allocating blacklist vector
	dhi->blacklistCapacity = BLACKLISTVECTOR_ALLOCBLOCK;
	dhi->blacklistVector = malloc(dhi->blacklistCapacity * sizeof(char *));
	
	// 2. Allocating diagnostic vector
	dhi->thrownDiagnosticsCapacity = THROWNDIAGNOSTICS_ALLOCKBLOCK;
	dhi->thrownDiagnosticsVector = malloc(dhi->thrownDiagnosticsCapacity * sizeof(CkThrownDiagnostic));
}

void CkDiagnosticHandlerDestroyInstance(CkDiagnosticHandlerInstance *dhi)
{
	// 1. Destroying the blacklist vector
	for (size_t i = 0; i < dhi->blacklistCount; i++)
		free(dhi->blacklistVector[i]);
	free(dhi->blacklistVector);

	// 3. Destroying the thrown diagnostics vector
	for (size_t i = 0; i < dhi->thrownDiagnosticsCount; i++)
		free(dhi->thrownDiagnosticsVector[i].message);
	free(dhi->thrownDiagnosticsVector);
	
	memset(dhi, 0, sizeof(CkDiagnosticHandlerInstance));
}

void CkDiagnosticBlacklist(CkDiagnosticHandlerInstance *dhi, char *name)
{
	
	size_t nameLength;   // The length of the blacklist name entry.
	char   **destination;// The destination blacklist entry.

	// 1. Preventing duplicate entries
	for (size_t i = 0; i < dhi->blacklistCount; i++) {
		if (!strcmp(dhi->blacklistVector[i], name))
			return;
	}

	// 2. Reallocating space if required
	if (dhi->blacklistCount + 1 == dhi->blacklistCapacity) {
		dhi->blacklistCapacity += BLACKLISTVECTOR_ALLOCBLOCK;
		dhi->blacklistVector = realloc(dhi->blacklistVector, dhi->blacklistCapacity * sizeof(char *));
	}

	nameLength = strlen(name) + 1;
	destination = dhi->blacklistVector + dhi->blacklistCount++;

	*destination = malloc(nameLength);
	strcpy_s(*destination, nameLength, name);
}

void CkDiagnosticWhitelist(CkDiagnosticHandlerInstance *dhi, char *name)
{
	CK_ARG_NON_NULL(dhi)
	CK_ARG_NON_NULL(name)

	for (size_t i = 0; i < dhi->blacklistCount; i++) {
		if (!strcmp(dhi->blacklistVector[i], name)) {
			/*
			 * Unless the diagnostic is at the end of the buffer,
			 * we can't simply remove an entry from the middle of the
			 * list. Therefore, we must take all of the entries below
			 * and after the removed entry and paste them together.
			*/
			const size_t upperStart = i + 1 < dhi->blacklistCount ? i + 1 : i;
			char **upperSubvector;
			size_t upperSubvectorSize;

			// Check for end-of-list
			if (i == upperStart) {
				dhi->blacklistCount--;
				return;
			}

			free(dhi->blacklistVector[i]);

			upperSubvectorSize = ( dhi->blacklistCount - upperStart ) * sizeof(char *);
			upperSubvector = _malloca(upperSubvectorSize);
			// Upper elems -> subvector
			memcpy_s(
				upperSubvector, upperSubvectorSize,
				dhi->blacklistVector, upperSubvectorSize);
			// Subvector -> Elems
			memcpy_s(
				dhi->blacklistVector + --dhi->blacklistCount, upperSubvectorSize,
				upperSubvector, upperSubvectorSize);

			_freea(upperSubvector);
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
			if (!strcmp(name, dhi->blacklistVector[i]))
				return;
		}
	}

	va_start(va_args, format);
	CkGetRowColString(dhi->pPassedSource->source, position, &pos2DRow, &pos2DCol);

	// Reallocating necessary space
	if (dhi->thrownDiagnosticsCount + 1 == dhi->thrownDiagnosticsCapacity) {
		dhi->thrownDiagnosticsCapacity += THROWNDIAGNOSTICS_ALLOCKBLOCK;
		dhi->thrownDiagnosticsVector = realloc(dhi->thrownDiagnosticsVector, sizeof(CkThrownDiagnostic) * dhi->thrownDiagnosticsCapacity);
	}

	diag = dhi->thrownDiagnosticsVector + dhi->thrownDiagnosticsCount++;

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
	diag->message = malloc(totalLength + 1);
	diag->severity = severity;
	strcpy_s(diag->message, totalLength, prefixBuffer);
	strcat_s(diag->message, totalLength, messageBuffer);
	_freea(prefixBuffer);
	_freea(messageBuffer);

	va_end(va_args);
}

void CkDiagnosticDisplay(CkDiagnosticHandlerInstance *dhi)
{
	for (size_t i = 0; i < dhi->thrownDiagnosticsCount; i++) {
		const CkThrownDiagnostic *diagnostic = dhi->thrownDiagnosticsVector + i;
		puts(diagnostic->message);
	}
}
