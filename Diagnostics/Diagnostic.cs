using ck.Syntax;
using ck.Util;
using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Diagnostics;

/// <summary>
/// Tells the compiler how it should handle a type of non-critical diagnostic.
/// </summary>
public enum NonCriticalDiagMode
{
    /// <summary>
    /// Default diag behavior
    /// </summary>
    Default,

    /// <summary>
    /// Diag should be suppressed.
    /// </summary>
    Suppressed,

    /// <summary>
    /// Increased priority of the warning. This turns warnings into errors
    /// and infos into warnings, for example.
    /// </summary>
    IncreasedPriority,
}

/// <summary>
/// A diagnostic object representing a diagnostic.
/// </summary>
public sealed class Diagnostic
{
    /// <summary>
    /// A list of diagnostics.
    /// </summary>
    private static readonly List<Diagnostic> _diagnostics = new();

    /// <summary>
    /// A list of suppressed diagnostics.
    /// </summary>
    private static readonly List<Diagnostic> _suppressed_diagnostics = new();

    /// <summary>
    /// How warnings should be treated.
    /// </summary>
    public static NonCriticalDiagMode WarningMode = NonCriticalDiagMode.Default;

    /// <summary>
    /// How infos should be treated.
    /// </summary>
    public static NonCriticalDiagMode InfoMode = NonCriticalDiagMode.Default;

    /// <summary>
    /// A list of diagnostic classes to automatically suppress.
    /// </summary>
    public static HashSet<string> SuppressClasses = new();

    /// <summary>
    /// Whether any errors have been encountered so far.
    /// </summary>
    public static bool ErrorsEncountered { get; private set; }

    /// <summary>
    /// Whether supressed diagnostics should be displayed.
    /// </summary>
    public static bool DisplaySuppressed = false;

    /// <summary>
    /// Throws a compiler diagnostic.
    /// </summary>
    /// <param name="src">The source of the diagnostic.</param>
    /// <param name="pos">The position of the diagnostic (its origin.)</param>
    /// <param name="severity">The severity of the diagnostic.</param>
    /// <param name="classname">The classname of the diagnostic.</param>
    /// <param name="txt">The message.</param>
    public static void Throw(Source? src, int pos, DiagnosticSeverity severity, string classname, string txt)
    {
        // warnings as error & infos as warnings
        if (severity == DiagnosticSeverity.Info && InfoMode == NonCriticalDiagMode.IncreasedPriority)
            severity = DiagnosticSeverity.Warning;
        if (severity == DiagnosticSeverity.Warning && WarningMode == NonCriticalDiagMode.IncreasedPriority)
            severity = DiagnosticSeverity.Error;

        // Constructs diagnostic and displays if fatal
        var diagnostic = new Diagnostic(src, pos, txt, classname, severity);
        if (severity == DiagnosticSeverity.CompilerError)
            DisplaySingle(diagnostic);

        // Throwing & suppression management
        if (severity == DiagnosticSeverity.Info && InfoMode == NonCriticalDiagMode.Suppressed
         || severity == DiagnosticSeverity.Warning && InfoMode == NonCriticalDiagMode.Suppressed
         || SuppressClasses.Contains(classname) && severity != DiagnosticSeverity.Error)
            _suppressed_diagnostics.Add(diagnostic);
        else
        {
            _diagnostics.Add(diagnostic);
            if (severity == DiagnosticSeverity.Error)
                ErrorsEncountered = true;
        }
    }

    /// <summary>
    /// Displays the diagnostics.
    /// </summary>
    public static void Display()
    {
        _diagnostics.ForEach(DisplaySingle);
        if (DisplaySuppressed && _suppressed_diagnostics.Count != 0)
        {
            Console.WriteLine("\nSuppressed diagnostics:");
            _suppressed_diagnostics.ForEach(DisplaySingle);
        }

        var error_count 
            = _diagnostics.Sum(diag => diag.Severity == DiagnosticSeverity.Error ? 1 : 0)
            + _suppressed_diagnostics.Sum(diag => diag.Severity == DiagnosticSeverity.Error ? 1 : 0);
        var warn_count 
            = _diagnostics.Sum(diag => diag.Severity == DiagnosticSeverity.Warning ? 1 : 0)
            + _suppressed_diagnostics.Sum(diag => diag.Severity == DiagnosticSeverity.Error ? 1 : 0);
        Console.WriteLine($"Build State: {error_count} error(s), {warn_count} warning(s)");
    }

    public static void Clear()
    {
        _diagnostics.Clear();
        _suppressed_diagnostics.Clear();
        ErrorsEncountered = false;
    }

    /// <summary>
    /// Fetches the two previous lines and the current line of code.
    /// </summary>
    /// <param name="center_line">The center line index.</param>
    private static string GetSourceLinesAt(Source src, int center_line)
        => $"{src.GetLine(center_line - 2)}{src.GetLine(center_line - 1)}{src.GetLine(center_line)}";

    /// <summary>
    /// Displays a single diagnostic.
    /// </summary>
    /// <param name="diagnostic">The diagnostic to display.</param>
    private static void DisplaySingle(Diagnostic diagnostic)
    {
        var text_color = diagnostic.Severity switch
        {
            DiagnosticSeverity.Info => ConsoleColor.Cyan,
            DiagnosticSeverity.Warning => ConsoleColor.Yellow,
            DiagnosticSeverity.Error => ConsoleColor.Red,
            DiagnosticSeverity.CompilerError => ConsoleColor.Red,

            _ => throw new NotImplementedException()
        };

        Console.ForegroundColor = text_color;

        var position = diagnostic.Source != null ? diagnostic.Source.LinearToColumnRowCoordinates(diagnostic.Position) : (0, 0);
        var position_text = position.Item1 != 0 ? $"{position} " : string.Empty;
        Console.WriteLine(position_text + diagnostic.CompiledMesage);

        Console.ForegroundColor = ConsoleColor.DarkMagenta;
        Console.WriteLine(diagnostic.Source != null ? GetSourceLinesAt(diagnostic.Source, position.Item1 - 1) : string.Empty);
        Console.ForegroundColor = ConsoleColor.Magenta;
        if (diagnostic.Source != null)
            Console.WriteLine(new string(Enumerable.Repeat('~', position.Item2 - 1).ToArray()) + '^');
        Console.WriteLine();
        
        Console.ResetColor();
        if (diagnostic.Severity == DiagnosticSeverity.CompilerError)
            Environment.Exit(-1);
    }

    /// <summary>
    /// Creates a restoration point, used to remove a group of diagnostics created after this
    /// restoration point if needed.
    /// </summary>
    public static (int N, int S) CreateRestorationPoint() => (_diagnostics.Count, _suppressed_diagnostics.Count);

    /// <summary>
    /// Loads a restoration point created with <see cref="CreateRestorationPoint"/>.
    /// </summary>
    public static void LoadRestorationPoint((int N, int S) restoration_point)
    {
        _diagnostics.RemoveRange(Math.Max(restoration_point.N - 1, 0), _diagnostics.Count - restoration_point.N);
        _suppressed_diagnostics.RemoveRange(Math.Max(restoration_point.S - 1, 0), _suppressed_diagnostics.Count - restoration_point.S);
    }

    /// <summary>
    /// The source file causing/closest to the diagnostic.
    /// </summary>
    public readonly Source? Source;

    /// <summary>
    /// The position in the source file.
    /// </summary>
    public readonly int Position;

    /// <summary>
    /// The resulting compiled message.
    /// </summary>
    public readonly string CompiledMesage;

    /// <summary>
    /// The name of the diagnostic.
    /// </summary>
    public readonly string Class;

    /// <summary>
    /// The severity of the diagnostic.
    /// </summary>
    public readonly DiagnosticSeverity Severity;

    /// <summary>
    /// Creates a new diagnostic object.
    /// </summary>
    /// <param name="src">The source file related to this diagnostic. Null if none.</param>
    /// <param name="pos">The origin in the source file of the error.</param>
    /// <param name="compiled">The text of the diagnostic.</param>
    /// <param name="class">The class of the diagnostic (name.)</param>
    /// <param name="severity">The severity of the diagnostic.</param>
    private Diagnostic(Source? src, int pos, string compiled, string @class, DiagnosticSeverity severity)
    {
        Source = src;
        Position = pos;
        CompiledMesage = compiled;
        Class = @class;
        Severity = severity;
    }
}