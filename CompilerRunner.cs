using ck.Diagnostics;
using ck.Lang;
using ck.Lang.Tree.Scopes;
using ck.Lowering;
using ck.Lowering.Lowers;
using ck.Syntax;
using ck.Syntax.Parsing;
using ck.XIL;
using System.Diagnostics;

namespace ck;

/// <summary>
/// Processes the input from the user and performs compilation.
/// </summary>
public sealed class CompilerRunner
{
    /// <summary>
    /// The alignment required for the compiler.
    /// </summary>
    public static readonly int AlignmentRequired = 8;

    /// <summary>
    /// Align an integer to a boundary.
    /// </summary>
    public static int Align(int n, int alignment)
        => (n + (alignment - 1)) & ~(alignment - 1);

    /*
     * <path>       :: The input path
     * -a <path>    :: The output assembly path (by default, input-ext+.s)
     * -o <path>    :: The output binary path (by default, a.out/a.exe)
     * -i <path>    :: Adds an include path
     * -v           :: Displays info about the compiler
     * 
     * --werror     :: Warning as errors
     * --iwarn      :: Infos as warnings
     * --wnone      :: No warnings
     * --inone      :: No infos
     * --no <class> :: Suppresses a warning/info
     */

    /// <summary>
    /// The input path. Mandatory.
    /// </summary>
    public readonly string InputPath = string.Empty;

    /// <summary>
    /// The output assembly path. Defaults to <c>a.s</c>.
    /// </summary>
    public readonly string OutputAssemblyPath = string.Empty;

    /// <summary>
    /// The output binary path. Defaults to <c>a.exe</c> or <c>a.out</c> depending on the system.
    /// </summary>
    public readonly string OutputBinaryPath = string.Empty;

    /// <summary>
    /// The command for the assembler.
    /// </summary>
    public readonly string AssemblerCommand = "nasm";

    /// <summary>
    /// Additional includes
    /// </summary>
    public readonly string[] Includes;

    /// <summary>
    /// The parsing has went correctly.
    /// </summary>
    private readonly bool _suitable_for_compilation = false;

#if DEBUG
    /// <summary>
    /// Whether the compiler should run insteads of compiling.
    /// </summary>
    public readonly bool RunTests = false;
#endif

    private void DisplayError(string message)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine("<ck error> " + message);
        Console.ResetColor();
    }

    /// <summary>
    /// Runs the compiler normally.
    /// </summary>
    public void RunCompiler()
    {
        if (RunTests)
        {
            TestRunner.RunTests();
            return;
        }

        var uptime_parsing = 0d;
        var uptime_semantic = 0d;
        var uptime_gen = 0d;
        var uptime_asm = 0d;

        if (!_suitable_for_compilation)
            return;

        var stopwatch = Stopwatch.StartNew();
        var main_source = new Source(InputPath);
        var parser = new Parser(main_source);
        parser.AppendIncludeDirectories(Includes);
        uptime_parsing = stopwatch.Elapsed.TotalMilliseconds;
        stopwatch.Restart();
        var program = parser.ParseProgram();
        var analyzer = new Analyzer(program);
        var error_status = analyzer.Analyze() || Diagnostic.ErrorsEncountered;

        // lowering
        var low_expander = new LowPass();
        low_expander.MutateAst(program);
        var o0 = new O0Pass();
        o0.MutateAst(program);

        uptime_semantic = stopwatch.Elapsed.TotalMilliseconds;
        stopwatch.Restart();
        Diagnostic.Display();
        if (error_status)
        {
            DisplayError("cannot proceed with compilation; errors encountered before or at semantic analysis");
            Console.WriteLine($"uptime parser: {uptime_parsing}ms");
            Console.WriteLine($"uptime semantic analyzer: {uptime_semantic}ms");
            return;
        }

        var xcode = new XModule(Path.GetFileNameWithoutExtension(InputPath));
        foreach (var symbol in program.GetChildren())
        {
            if (symbol is not FunctionNode func)
                continue;

            xcode.AddMethod(XCompiler.GenerateFunction(func, xcode));
        }

        uptime_asm = stopwatch.Elapsed.TotalMilliseconds;
        stopwatch.Stop();

        var output_path = Path.ChangeExtension(InputPath, "x");
        File.WriteAllText(output_path, xcode.ToString());

        // debug pannel
        Console.WriteLine();
        Console.WriteLine($"X written to {output_path}");
        Console.WriteLine($"uptime parser: {(float)uptime_parsing}ms");
        Console.WriteLine($"uptime analyzer: {(float)uptime_semantic}ms");
        Console.WriteLine($"uptime generator: {(float)uptime_gen}ms");
        Console.WriteLine($"uptime assembler: {(float)uptime_asm}ms");
    }

    public CompilerRunner(string[] args)
    {
        var version_requested = false;
        var includes = new List<string>();
        Includes = Array.Empty<string>();

        if (args.Length == 0)
        {
            Console.WriteLine("ck usage:");
            Console.WriteLine("  <path>:             \tspecify input file (max=1)");
            Console.WriteLine("  -v/--version:       \tdisplays version info");
            Console.WriteLine();
            Console.WriteLine("  -a/-assembly <path>:\tspecify assembly path (max=1)");
            Console.WriteLine("  -o/--output <path>: \tspecify output path (max=1)");
            Console.WriteLine("  -i/--include <path>:\tspecify an include path");
            Console.WriteLine("  --asmcmd <cmd>:     \tspecify a custom assembler (may not be supported, max=1)");
            Console.WriteLine();
            Console.WriteLine("  --no <diagclass>:   \tsilence a diagnostic");
            Console.WriteLine("  --werror:           \tpromote warnings to errors");
            Console.WriteLine("  --wnone:            \tsilence all warnings");
            Console.WriteLine("  --iwarn:            \tpromote infos to warns (can be combined with --werror)");
            Console.WriteLine("  --inone:            \tsilence all infos");
            Console.WriteLine("  --isuppress:        \tignore supress");
#if DEBUG
            Console.WriteLine();
            Console.WriteLine("  --test:             \trun tests for the compiler");
#endif
        }

        for (var i = 0; i < args.Length; i++)
        {
            var arg = args[i];
            var arg_lowercase = arg.ToLower();
            if (arg_lowercase == "-a" || arg_lowercase == "--assembly")
            {
                if (i + 1 > args.Length)
                {
                    DisplayError($"missing path after '{arg}'");
                    return;
                }

                var assembly_path = args[++i];
                OutputAssemblyPath = assembly_path;
                continue;
            }
            else if (arg_lowercase == "-o" || arg_lowercase == "--output")
            {
                if (i + 1 > args.Length)
                {
                    DisplayError($"missing path after '{arg}'");
                    return;
                }

                var binary_path = args[++i];
                OutputBinaryPath = binary_path;
                continue;
            }
            else if (arg_lowercase == "-i" || arg_lowercase == "--include")
            {
                if (i + 1 > args.Length)
                {
                    DisplayError($"missing path after '{arg_lowercase}'");
                    return;
                }

                var include_path = args[++i];
                includes.Add(include_path);
                continue;
            }
            else if (arg_lowercase == "--asmcmd")
            {
                if (i + 1 > args.Length)
                {
                    DisplayError($"missing command after '{arg_lowercase}'");
                    return;
                }

                if (AssemblerCommand != "nasm")
                {
                    DisplayError($"duplicate assignment of asmcmd\ncurrent:'{AssemblerCommand}'\nattempt:'{arg[++i]}'");
                    return;
                }
                AssemblerCommand = args[++i];
                continue;
            }
            else if (arg_lowercase == "--no")
            {
                if (i + 1 > args.Length)
                {
                    DisplayError($"missing diagnostic class after '{arg_lowercase}'");
                    return;
                }

                Diagnostic.SuppressClasses.Add(args[++i]);
                continue;
            }
            else if (arg_lowercase == "--test")
            {
                RunTests = true;
                continue;
            }
            else if (arg_lowercase == "--werror")
                Diagnostic.WarningMode = NonCriticalDiagMode.IncreasedPriority;
            else if (arg_lowercase == "--wnone")
                Diagnostic.WarningMode = NonCriticalDiagMode.Suppressed;
            else if (arg_lowercase == "--iwarn")
                Diagnostic.InfoMode = NonCriticalDiagMode.IncreasedPriority;
            else if (arg_lowercase == "--inone")
                Diagnostic.InfoMode = NonCriticalDiagMode.Suppressed;
            else if (arg_lowercase == "--isuppress")
                Diagnostic.DisplaySuppressed = true;
            else if (arg_lowercase == "-v" || arg_lowercase == "--version")
                version_requested = true;

            if (InputPath != string.Empty)
            {
                DisplayError($"duplicate input file. ck requires a single input file\ncurrent:'{InputPath}'\nattempt:'{arg}'");
                return;
            }

            InputPath = arg;
        }

        Includes = includes.ToArray();

        if (OutputAssemblyPath == string.Empty)
            OutputAssemblyPath = "a.s";

        if (OutputBinaryPath == string.Empty)
            OutputBinaryPath = OperatingSystem.IsWindows() ? "a.exe" : "a.out";

        if (version_requested)
        {
            Console.WriteLine("ck private beta, only for the cool kids!!!");
            Console.WriteLine("feature set: no");
        }

        if (InputPath != string.Empty)
            _suitable_for_compilation = true;
    }
}
