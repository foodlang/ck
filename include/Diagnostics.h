/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the diagnostic handler module. The diagnostic handler
 * is used to report CK errors (related to the user code & linking). These
 * errors can then be all displayed at once.
 *
 ***************************************************************************/

#ifndef CK_ERROR_H_
#define CK_ERROR_H_

#include "Syntax/Lex.h"
#include <Memory/Arena.h>
#include <Memory/List.h>

 // The severity of the diagnostic.
typedef enum CkDiagnosticSeverity
{
	// A suggestion or message given by the compiler.
	CK_DIAG_SEVERITY_MESSAGE,

	// A warning about a potential bug to fix. Will not
	// prevent compilation (unless specified).
	CK_DIAG_SEVERITY_WARNING,

	// An error is critical and will prevent compilation
	// in all cases, as the compiled results might not
	// reflect intended behaviour.
	CK_DIAG_SEVERITY_ERROR,

} CkDiagnosticSeverity;

// Stores essential data about a thrown diagnostic.
typedef struct CkThrownDiagnostic
{
	char *message;    // The message. This is already formatted and allocated on the heap.
	uint8_t severity; // The severity of the diagnostic.
	size_t line;      // The line number. Used to display context lines.
	size_t column;    // The column. Used to display context lines.
	CkSource *source; // The source file. Used to display context lines.

} CkThrownDiagnostic;

// An instance of a diagnostic handler. A diagnostic handler
// takes care of all of the diagnostics thrown by the compiler,
// and displays them nicely to the user.
typedef struct CkDiagnosticHandlerInstance
{
	// Stores all of the blacklisted diagnostics (diagnostics to skip over.)
	CkList *blacklistVector;

	// Stores all of the thrown diagnotics.
	CkList *thrownDiagnosticsVector;

	// True if the diagnostics encounters any errors.
	bool anyErrors;

	// True if the diagnostics encounters any non-blacklisted warnings.
	bool anyWarnings;

	// The arena used for allocations. Does not include the thrown diagnostics
	// and the blacklist.
	CkArenaFrame *arena;

	// If true, the diagnostic handler is currently in try mode.
	bool tryMode;

	// The number of diagnostics thrown in try mode. If no try mode, set to 0.
	size_t thrownTryMode;

} CkDiagnosticHandlerInstance;

// Creates a new diagnostic handler instance.
void CkDiagnosticHandlerCreateInstance( CkArenaFrame *arena, CkDiagnosticHandlerInstance *dhi );

// Destroys a diagnostic handler and frees all used resources (except
// for the passed lexer.)
void CkDiagnosticHandlerDestroyInstance( CkDiagnosticHandlerInstance *dhi );

// Adds a diagnostic to the blacklist. The blacklist prevents warnings
// from being thrown. If the diagnostic is already added to the list,
// nothing happens.
void CkDiagnosticBlacklist( CkDiagnosticHandlerInstance *dhi, char *name );

// Attempts to remove a diagnostic from the blacklist. Does simply nothing
// if the diagnostic isn't already blacklisted.
void CkDiagnosticWhitelist( CkDiagnosticHandlerInstance *dhi, char *name );

// Throws a diagnostic.
void CkDiagnosticThrow(
	CkDiagnosticHandlerInstance *dhi,
	CkToken *p_token,
	uint8_t severity,
	char *name,
	const char *restrict format,
	... );

// Displays all of the reported diagnostics.
void CkDiagnosticDisplay( CkDiagnosticHandlerInstance *dhi );

// Begins a try mode. Used by the statement/declaration parser.
void CkDiagnosticBeginTryMode( CkDiagnosticHandlerInstance *dhi );

// Begins an end try mode. The old diagnostics are restored after a
// call to this function. Returns true if ok.
bool CkDiagnosticEndTryMode( CkDiagnosticHandlerInstance *dhi );

// Clears the thrown diagnostics.
void CkDiagnosticClear( CkDiagnosticHandlerInstance* dhi );

#endif
