using ck.Diagnostics;
using ck.Lang;
using ck.Lowering;
using ck.Lowering.Lowers;
using ck.Syntax;
using ck.Syntax.Parsing;
using System.Diagnostics;

namespace ck;

/// <summary>
/// Runs the tests.
/// </summary>
public static class TestRunner
{
    private static readonly string TestDirectory = "./tests/";

    public static void RunTests()
    {
        var dir = new DirectoryInfo(TestDirectory);

        Debug.Assert(dir.Exists);

        var files = dir.GetFiles();
        for (var i = 0; i < files.Length; i++)
        {
            var file = files[i];
            // don't parse assembly!
            if (Path.GetExtension(file.FullName) == ".s" || Path.GetExtension(file.FullName) == ".x")
                continue;
            var runner = new CompilerRunner([file.FullName]);
            runner.RunCompiler();
            /*var stopwatch = Stopwatch.StartNew();
            var src = new Source(file.FullName);
            var parser = new Parser(src);

            var prog = parser.ParseProgram();
            if (Diagnostic.ErrorsEncountered)
                goto LError;

            var analyzer = new Analyzer(prog);
            analyzer.Analyze();
            var low = new O0Pass();
            low.MutateAst(prog);

            if (Diagnostic.ErrorsEncountered)
                goto LError;

            stopwatch.Stop();
            var line_count = src.Code.Split('\n', StringSplitOptions.RemoveEmptyEntries).Length;
            Console.WriteLine($"test #{0} ({file.FullName}) passed in {stopwatch.Elapsed.TotalMilliseconds} ms ({line_count/Math.Max(stopwatch.Elapsed.TotalSeconds, 1)} loc/s)");
            Console.WriteLine(prog.PrettyPrint());
            Diagnostic.Clear();
            continue;
        LError:
            Diagnostic.Display();
            Console.WriteLine($"\ntest #{0} ({file.FullName}) failed");
            Diagnostic.Clear();*/
        }
    }
}
