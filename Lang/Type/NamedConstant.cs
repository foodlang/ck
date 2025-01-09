using ck.Lang.Tree.Expressions;

namespace ck.Lang.Type;

/// <summary>
/// A named constant in a type.
/// </summary>
public sealed class NamedConstant
{
    /// <summary>
    /// The name of the constant (or null for default)
    /// </summary>
    public readonly string? Name;

    /// <summary>
    /// The value of the named constant.
    /// </summary>
    public readonly Expression? Value;

    /// <summary>
    /// The const that this const aliases.
    /// </summary>
    public readonly NamedConstant? Aliasing;

    public NamedConstant(string? name, Expression? value, NamedConstant? aliasing)
    {
        Name = name;
        Value = value;
        Aliasing = aliasing;
    }
}
