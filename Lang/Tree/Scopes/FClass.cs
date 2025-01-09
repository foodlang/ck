using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// A user type (class)
/// </summary>
public sealed class FClass : NamedObject
{
    /// <summary>
    /// The class ponted to.
    /// </summary>
    public readonly FType Class;

    public FClass(
        // superclass params
        ObjectTable table, string name, Source? internal_to, AttributeClause attributes, Token diag_token,
        // fclass params
        FType @class
        ) : base(table, name, internal_to, attributes, diag_token)
    {
        Class = @class;
    }
}
