#include <include/Diagnostics.h>
#include <malloc.h>
#include <string.h>

#define BLACKLISTVECTOR_ALLOCBLOCK    16
#define THROWNDIAGNOSTICS_ALLOCKBLOCK 16

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
	// 1. Destroying the blacklist
	for (size_t i = 0; i < dhi->blacklistCount; i++)
		free(dhi->blacklistVector[i]);
	free(dhi->blacklistVector);

	// 3. Destroying the thrown diagnostics vector
	free(dhi->thrownDiagnosticsVector);
	
	memset(dhi, 0, sizeof(CkDiagnosticHandlerInstance));
}

void CkDiagnosticBlacklist(CkDiagnosticHandlerInstance *dhi, char *name)
{
	
	size_t nameLength;   // The length of the blacklist name entry.
	char   *destination; // The destination blacklist entry.

	// 1. Preventing duplicate entries
	for (size_t i = 0; i < dhi->blacklistCount; i++) {
		if (!strcmp(dhi->blacklistVector[i], name))
			return;
	}

	nameLength = strlen(name) + 1;
	destination = dhi->blacklistVector[dhi->blacklistCount];

	// 2. Reallocating space if required
	if (dhi->blacklistCount + 1 == dhi->blacklistCapacity) {
		dhi->blacklistCapacity += BLACKLISTVECTOR_ALLOCBLOCK;
		dhi->blacklistVector = realloc(dhi->blacklistVector, dhi->blacklistCapacity * sizeof(char *));
	}

	destination = malloc(nameLength);
	strcpy_s(destination, nameLength, name);
}

void CkDiagnosticWhitelist(CkDiagnosticHandlerInstance *dhi, char *name)
{
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
