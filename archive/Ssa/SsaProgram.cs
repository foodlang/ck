using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Ssa;

public sealed class SsaProgram
{
    public readonly Dictionary<string, SsaFunction> Functions = new();

    public string Print()
    {
        var sb = new StringBuilder(4096);
        foreach (var f in Functions)
            f.Value.Print(sb, f.Key);
        return sb.ToString();
    }
}
