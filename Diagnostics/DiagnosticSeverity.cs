namespace ck.Diagnostics;

/// <summary>
/// The severity of a diagnostic helps the compiler display accurate error
/// messages and handle its behaviour accordingly.
/// </summary>
public enum DiagnosticSeverity
{
    /// <summary>
    /// An info message is simply to inform the user of a
    /// special case or such that isn't of grave importance,
    /// unlike undefined behaviour threats or such.
    /// </summary>
    Info,

    /// <summary>
    /// A warning is a cause for concern to the user, but doesn't
    /// prevent compilation (unless -Werror.)
    /// </summary>
    Warning,

    /// <summary>
    /// An error prevents compilation in all cases and should be
    /// only thrown when the compiler fails to understand the program,
    /// cannot create an output or knows definitely of a undefined
    /// behaviour path (like reading to an unitialized variable.)
    /// </summary>
    Error,

    /// <summary>
    /// An error thrown by the compiler itself, indicative of an internal issue.
    /// </summary>
    CompilerError,
}
