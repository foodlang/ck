using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Ssa;

/// <summary>
/// The live interval of a variable.
/// </summary>
public readonly struct LiveInterval
{
    public readonly SsaInstruction? Start;
    public readonly SsaInstruction? End;

    public LiveInterval(SsaInstruction? start, SsaInstruction? end)
    {
        Start = start;
        End = end;
    }

    /// <summary>
    /// Whether two intervals overlap.
    /// </summary>
    public bool Overlap(ref readonly LiveInterval other)
        => Start! <= other.End! && other.Start! <= End!;
}
