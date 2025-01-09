using ck.Lang.Type;
using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Type;

/// <summary>
/// A structure or union.
/// </summary>
public sealed class StructSubjugateSignature : ITypeSubjugateSignature
{
    /// <summary>
    /// The members of the structure-like type.
    /// </summary>
    public readonly StructMember[] Members;

    /// <summary>
    /// Whether the structure is actually a union.
    /// </summary>
    public readonly bool IsUnion;

    public IEnumerable<FType> GetSubjugateTypes() => Members.Select(m => m.Type!);

    public void WriteSubjugateTypes(IEnumerable<FType> subjugate_types)
    {
        for (var i = 0; i < Members.Length; i++)
            Members[i].Type = subjugate_types.ElementAt(i);
    }

    // lengthof() is not available for structs
    public nint LengthOf() => 0;

    public bool EqualTo(ITypeSubjugateSignature other)
    {
        var other_struct = (StructSubjugateSignature)other;

        if (IsUnion != other_struct.IsUnion
            || Members.Length != other_struct.Members.Length)
            return false;

        for (var i = 0; i < Members.Length; i++)
        {
            var this_mem = Members[i];
            var other_mem = other_struct.Members[i];

            if (this_mem.Type != other_mem.Type)
                return false;
        }

        return true;
    }

    public StructSubjugateSignature(StructMember[] members, bool is_union)
    {
        Members = members;
        IsUnion = is_union;
    }
}
