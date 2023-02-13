/*
 * Provides structs and functions for diagnostic management.
*/

#ifndef CK_ERROR_H_
#define CK_ERROR_H_

#include "Syntax/Lex.h"
#include <ckmem/Arena.h>
#include <ckmem/List.h>

enum
{
	/// <summary>
	/// A suggestion or message given by the compiler.
	/// </summary>
	CK_DIAG_SEVERITY_MESSAGE,

	/// <summary>
	/// A warning about a potential bug to fix. Will not
	/// prevent compilation (unless specified).
	/// </summary>
	CK_DIAG_SEVERITY_WARNING,

	/// <summary>
	/// An error is critical and will prevent compilation
	/// in all cases, as the compiled results might not
	/// reflect intended behaviour.
	/// </summary>
	CK_DIAG_SEVERITY_ERROR,
};

/// <summary>
/// Stores essential data about a thrown diagnostic.
/// </summary>
typedef struct CkThrownDiagnostic
{
	/// <summary>
	/// The message. This is already formatted and allocated on the heap.
	/// </summary>
	char *message;

	/// <summary>
	/// The severity of the diagnostic.
	/// </summary>
	uint8_t severity;

} CkThrownDiagnostic;

/// <summary>
/// An instance of a diagnostic handler. A diagnostic handler
/// takes care of all of the diagnostics thrown by the compiler,
/// and displays them nicely to the user.
/// </summary>
typedef struct CkDiagnosticHandlerInstance
{
	/// <summary>
	/// A pointer to the lexer instance reading the source
	/// code.
	/// </summary>
	CkLexInstance *pPassedSource;

	/// <summary>
	/// Stores all of the blacklisted diagnostics (diagnostics to skip over.)
	/// </summary>
	CkList *blacklistVector;

	/// <summary>
	/// Stores all of the thrown diagnotics.
	/// </summary>
	CkList *thrownDiagnosticsVector;

	/// <summary>
	/// True if the diagnostics encounters any errors.
	/// </summary>
	bool_t anyErrors;

	/// <summary>
	/// True if the diagnostics encounters any non-blacklisted warnings.
	/// </summary>
	bool_t anyWarnings;

	/// <summary>
	/// The arena used for allocations. Does not include the thrown diagnostics and the blacklist.
	/// </summary>
	CkArenaFrame *arena;

} CkDiagnosticHandlerInstance;

/// <summary>
/// Creates a new diagnostic handler instance.
/// </summary>
/// <param name="dhi">A pointer to the destination struct.</param>
/// <param name="pPassedLexer">A pointer to the passed lexer instance.</param>
void CkDiagnosticHandlerCreateInstance(CkArenaFrame *arena, CkDiagnosticHandlerInstance *dhi, CkLexInstance *pPassedLexer);

/// <summary>
/// Destroys a diagnostic handler and frees all used resources (except
/// for the passed lexer.)
/// </summary>
/// <param name="dhi">A pointer to the diagnostic handler struct.</param>
void CkDiagnosticHandlerDestroyInstance(CkDiagnosticHandlerInstance *dhi);

/// <summary>
/// Adds a diagnostic to the blacklist. The blacklist prevents warnings
/// from being thrown. If the diagnostic is already added to the list,
/// nothing happens.
/// </summary>
/// <param name="dhi">A pointer to the diagnostic handler struct.</param>
/// <param name="name">The name of the diagnostic to blacklist. Case-sensitive.</param>
void CkDiagnosticBlacklist(CkDiagnosticHandlerInstance *dhi, char *name);

/// <summary>
/// Attempts to remove a diagnostic from the blacklist. Does simply nothing
/// if the diagnostic isn't already blacklisted.
/// </summary>
/// <param name="dhi">A pointer to the diagnostic handler struct.</param>
/// <param name="name">The name of the diagnostic to whitelist. Case-sensitive.</param>
void CkDiagnosticWhitelist(CkDiagnosticHandlerInstance *dhi, char *name);

/// <summary>
/// Throws a diagnostic.
/// </summary>
/// <param name="dhi">A pointer to the diagnostic handler struct.</param>
/// <param name="position">The linear position that causes the diagnostic.</param>
/// <param name="severity">The severity of the diagnostic.</param>
/// <param name="name">The name of the diagnostic to throw. Case-sensitive.</param>
/// <param name="format">The format of the message.</param>
/// <param name="va_args">Params for the format.</param>
void CkDiagnosticThrow(
	CkDiagnosticHandlerInstance *dhi,
	uint64_t position,
	uint8_t severity,
	char *name,
	const char *restrict format,
	...);

/// <summary>
/// Displays all of the reported diagnostics.
/// </summary>
/// <param name="dhi">A pointer to the diagnostic handler struct.</param>
void CkDiagnosticDisplay(CkDiagnosticHandlerInstance *dhi);

#endif
