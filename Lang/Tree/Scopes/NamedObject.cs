using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// A named object is an object that rests in a scope and that bears a name.
/// </summary>
public abstract class NamedObject
{
    /// <summary>
    /// The table containing the object.
    /// </summary>
    public readonly ObjectTable Table;

    /// <summary>
    /// The name of the named object.
    /// </summary>
    public readonly string Name;

    /// <summary>
    /// (If internal) Source file that contains this internal field.<br/>
    /// Leave this field as <c>null</c> if the object is public.
    /// </summary>
    public readonly Source? InternalTo;

    /// <summary>
    /// The attributes of the named object.
    /// </summary>
    public readonly AttributeClause Attributes;

    /// <summary>
    /// Token used for diagnostic purposes.
    /// </summary>
    public readonly Token DiagToken;

    // default ctor
    protected NamedObject(ObjectTable table, string name, Source? internal_to, AttributeClause attributes, Token diag_token)
    {
        Table = table;
        Name = name;
        InternalTo = internal_to;
        Attributes = attributes;
        DiagToken = diag_token;
    }
}
