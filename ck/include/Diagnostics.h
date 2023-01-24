/*
 * Provides structs and functions for diagnostic management.
*/

#ifndef CK_ERROR_H_
#define CK_ERROR_H_

#include "Syntax/Lex.h"

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

	/// <summary>
	/// CK has run into an uncoverable internal error,
	/// and must abort. If this is encountered, it should
	/// be fixed.
	/// </summary>
	CK_DIAG_SEVERITY_ABORT,
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
	char **blacklistVector;

	/// <summary>
	/// The amount of elements in the blacklist vector.
	/// </summary>
	size_t blacklistCount;

	/// <summary>
	/// The current capacity of the blacklist vector.
	/// </summary>
	size_t blacklistCapacity;

	/// <summary>
	/// Stores all of the thrown diagnotics.
	/// </summary>
	CkThrownDiagnostic *thrownDiagnosticsVector;

	/// <summary>
	/// The amount of thrown diagnostics.
	/// </summary>
	size_t thrownDiagnosticsCount;

	/// <summary>
	/// The current capacity of the thrown diagnostics vector.
	/// </summary>
	size_t thrownDiagnosticsCapacity;

	/// <summary>
	/// True if the diagnostics encounters any errors.
	/// </summary>
	bool_t anyErrors;

	/// <summary>
	/// True if the diagnostics encounters any non-blacklisted warnings.
	/// </summary>
	bool_t anyWarnings;

} CkDiagnosticHandlerInstance;

/// <summary>
/// Creates a new diagnostic handler instance.
/// </summary>
/// <param name="dhi">A pointer to the destination struct.</param>
/// <param name="pPassedLexer">A pointer to the passed lexer instance.</param>
void CkDiagnosticHandlerCreateInstance(CkDiagnosticHandlerInstance *dhi, CkLexInstance *pPassedLexer);

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
/// <param name="dhi"></param>
/// <param name="name"></param>
/// <param name="format"></param>
/// <param name=""></param>
void CkDiagnosticThrow(CkDiagnosticHandlerInstance *dhi, char *name, const char *restrict format, ...);

#endif
