using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Type;

/// <summary>
/// Holds the identifier for the user type.
/// </summary>
public sealed class UserPlaceholderSubjugateSignature : ITypeSubjugateSignature
{
    /// <summary>
    /// The name of the type.
    /// </summary>
    public readonly string Name;

    /// <summary>
    /// Where the type was created.
    /// </summary>
    public readonly Source? Where;

    public UserPlaceholderSubjugateSignature(string name, Source? where)
    {
        Name = name;
        Where = where;
    }

    public IEnumerable<FType> GetSubjugateTypes() => Enumerable.Empty<FType>();

    public void WriteSubjugateTypes(IEnumerable<FType> subjugates) {} // no subjugate types available

    public nint LengthOf() => 0;

    // We don't know, so we return false to prevent catastrophic errors.
    // User placeholders should be replaced by their correct type
    // implementations later.
    public bool EqualTo(ITypeSubjugateSignature other)
        => false;
    /*{
        var other_user_placeholder = (UserPlaceholderSubjugateSignature)other;

        // There cannot be two types with the same name in the scope,
        // therefore we can assume they are equal.
        if (Where == other_user_placeholder.Where
            && Name == other_user_placeholder.Name)
            return true;

        
        return false;
    }*/
}
