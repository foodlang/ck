using ck.Diagnostics;
using ck.Syntax;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang;

/// <summary>
/// An attribute clause is made of multiple attributes. It can be applied
/// to a statement, a declaration or another syntax node.
/// </summary>
public sealed class AttributeClause : IEnumerable<FAttribute>
{
    /// <summary>
    /// The internal storage of the attributes.
    /// </summary>
    private readonly HashSet<FAttribute> _attributes = new();

    /// <summary>
    /// Adds an attribute to this clause.
    /// </summary>
    /// <param name="dbg_token">The debug token (used for throwing diagnostics)</param>
    /// <param name="attr">The attribute to add.</param>
    public void AddAttribute(Token dbg_token, FAttribute attr)
    {
        if (!_attributes.Add(attr))
            Diagnostic.Throw(dbg_token.Source, dbg_token.Position, DiagnosticSeverity.Error, "",
                $"duplicate attribute declaration '{attr.Name}'");
    }

    /// <summary>
    /// Adds a group of attributes to this clause.
    /// </summary>
    /// <param name="dbg_token">The debug token (used for throwing diagnostics)</param>
    /// <param name="attr">The attributes to add.</param>
    public void AddAttributes(Token dbg_token, IEnumerable<FAttribute> attributes)
    {
        foreach (var attr in attributes)
            AddAttribute(dbg_token, attr);
    }

    /// <summary>
    /// Returns true if the attribute is present in this clause.
    /// </summary>
    /// <param name="name">The name of the attribute.</param>
    /// <param name="args">The arguments of the attribute. This is null if the method returns false.</param>
    public bool HasAttribute(string name, out List<object>? args)
    {
        foreach (var attr in _attributes)
            if (attr.Name == name)
            {
                attr.Used = true;
                args = attr.Args;
                return true;
            }
        args = null;
        return false;
    }

    public IEnumerator<FAttribute> GetEnumerator() => _attributes.GetEnumerator();
    IEnumerator IEnumerable.GetEnumerator() => _attributes.GetEnumerator();
}
