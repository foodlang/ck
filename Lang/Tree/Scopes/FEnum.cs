using ck.Syntax;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// An enum
/// </summary>
public sealed class FEnum : NamedObject
{
    /// <summary>
    /// The named constants of the enum.
    /// </summary>
    public readonly (string Name, nint Const)[] NamedConstants;

    /// <summary>
    /// Attempts to look up a named constant in the enum.
    /// </summary>
    /// <param name="name">The name to look up.</param>
    /// <param name="k">(Out) The constant with the given name <paramref name="name"/>. <c>null</c> if not found.</param>
    /// <returns>True if the constant was found.</returns>
    public bool LookupNamedConstant(string name, out nint? k)
    {
        foreach (var nc in NamedConstants)
        {
            if (nc.Name == name)
            {
                k = nc.Const;
                return true;
            }
        }

        k = null;
        return false;
    }

    public FEnum(
        // superclass params
        ObjectTable table, string name, Source? internal_to, AttributeClause attributes, Token diag_token,
        // fenum params
        (string Name, nint Const)[] named_constants
        ) : base(table, name, internal_to, attributes, diag_token)
    {
        NamedConstants = named_constants;
    }
}
