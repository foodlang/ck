using ck.Diagnostics;
using ck.Syntax;

namespace ck;

internal class Program
{
    static void Main(string[] args)
    {
#if !DEBUG
        Console.WriteLine("ck - the official food compiler");
#else
        Console.WriteLine("ck - the official food compiler (debug mode)");
#endif
        Console.WriteLine("copyright (c) The Food Project 2023-2024");
        Console.WriteLine();
#if DEBUG
        Console.WriteLine($"working directory: {Directory.GetCurrentDirectory()}");
#endif

        var runner = new CompilerRunner(args);
        runner.RunCompiler();
    }
}
