using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Ssa;

/// <summary>
/// A block that has a memory offset and and a size.
/// </summary>
public readonly struct AutoBlock
{
    /// <summary>
    /// The offset of the block.
    /// </summary>
    public readonly nint Offset;

    /// <summary>
    /// The size of the block.
    /// </summary>
    public readonly nint Size;

    public AutoBlock(nint offset, nint size)
    {
        Offset = offset;
        Size = size;
    }

    /// <summary>
    /// Computes the top of the block.
    /// </summary>
    public nint Top() => Offset + Size;
}
