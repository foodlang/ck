using ck.Diagnostics;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang;

/// <summary>
/// An attribute that can be applied to multiple constructs in Food.
/// </summary>
public sealed class FAttribute
{
    /// <summary>
    /// The name of the attribute.
    /// </summary>
    public readonly string Name;

    /// <summary>
    /// The debug token of the attribute (used for attributes.)
    /// </summary>
    public readonly Token DbgToken;

    /// <summary>
    /// The arguments of the attribute.
    /// </summary>
    public readonly List<object> Args;

    /// <summary>
    /// Whether this attribute has been used.
    /// </summary>
    public bool Used = false;

    public FAttribute(string name, List<object> args, Token dbg_token)
    {
        Name = name;
        Args = args;
        DbgToken = dbg_token;
        _attributes.Add(this);
    }

    /// <summary>
    /// All of the attributes ever declared.
    /// </summary>
    private readonly static HashSet<FAttribute> _attributes = new();

    /// <summary>
    /// Reports the unused attributes in the program.
    /// </summary>
    public static void ReportUnusedAttributes()
    {
        foreach (var attr in _attributes)
        {
            if (!attr.Used)
                Diagnostic.Throw(attr.DbgToken.Source, attr.DbgToken.Position, DiagnosticSeverity.Warning,
                    "unused-attribute",
                    $"attribute '{attr.Name}' was ignored in this context");
        }
    }
}
