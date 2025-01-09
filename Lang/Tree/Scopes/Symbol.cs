using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// A symbol is a named object that is either a variable, a function param or a function.
/// </summary>
public sealed class Symbol : NamedObject
{
    /// <summary>
    /// Types of references for arguments.
    /// </summary>
    public enum ArgumentRefType
    {
        /// <summary>
        /// Not an argument, or passed by value.
        /// </summary>
        Value = 0,

        /// <summary>
        /// Read-only argument, passed by ref.
        /// </summary>
        In,

        /// <summary>
        /// Write arguemnt, passed by ref (treated as unitialized value.)
        /// </summary>
        Out,

        /// <summary>
        /// Read/write argument, passed by ref.
        /// </summary>
        Varying,
    }

    /*/// <summary>
    /// Types of variable access.
    /// </summary>
    public enum VariableAccessType
    {
        /// <summary>
        /// The variable is mutable and thread-local. Declared using `let`.
        /// </summary>
        Regular = 0,

        /// <summary>
        /// The variable is immutable and thread-local. Declared using `const`.
        /// </summary>
        Constant = 1,

        /// <summary>
        /// The variable is mutable and may be shared across threads or other resources.
        /// Declared using `shared`.
        /// </summary>
        Shared = 2,
    }*/

    /// <summary>
    /// (Param only) The reference type.
    /// </summary>
    public readonly ArgumentRefType RefType;

    /// <summary>
    /// (Local/Internal only) Whether the symbol has been referenced in its lifetime.
    /// </summary>
    public bool Referenced = false;

    /// <summary>
    /// The type of the symbol. May be unresolved at parser-level (denoted by <c>null</c>)
    /// </summary>
    public FType? Type;

    public Symbol(
        // superclass params
        ObjectTable table, string name, Source? internal_to, AttributeClause attributes, Token diag_token,
        // symbol params
        FType? T, ArgumentRefType ref_type = ArgumentRefType.Value
        ) : base(table, name, internal_to, attributes, diag_token)
    {
        Type = T;
        RefType = ref_type;
    }
}
