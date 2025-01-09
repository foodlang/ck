using ck.Lang.Type;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Type;

/// <summary>
/// A structure member.
/// </summary>
public sealed class StructMember
{
    /// <summary>
    /// The name of the structure member.
    /// </summary>
    public readonly string Name;

    /// <summary>
    /// The type of the structure member.
    /// </summary>
    public FType Type;

    public StructMember(string name, FType t)
    {
        Name = name;
        Type = t;
    }
}
