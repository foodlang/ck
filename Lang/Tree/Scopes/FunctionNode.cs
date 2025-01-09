using ck.Diagnostics;
using ck.Lang;
using ck.Lang.Tree.Statements;
using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// This node represents a function in the program tree.
/// </summary>
public sealed class FunctionNode : ScopeTree
{
    /// <summary>
    /// A function argument.
    /// </summary>
    public readonly struct Argument
    {
        /// <summary>
        /// The name of the argument.
        /// </summary>
        public readonly Token Identifier;

        /// <summary>
        /// The type of the argument.
        /// </summary>
        public readonly FType Type;

        /// <summary>
        /// The type of reference of this argument.
        /// </summary>
        public readonly Symbol.ArgumentRefType RefType;

        /// <summary>
        /// The attributes of the argument.
        /// </summary>
        public readonly AttributeClause Attributes;

        public Argument(Token name, FType T, Symbol.ArgumentRefType ref_type, AttributeClause attributes)
        {
            Identifier = name;
            Type = T;
            RefType = ref_type;
            Attributes = attributes;
        }

        
    }

    private Statement? _body;

    /// <summary>
    /// The body of the function.
    /// </summary>
    public ref Statement? Body
    {
        get => ref _body;
        /*set
        {
            Debug.Assert(_body == null);
            _body = value;
            if (value != null && Count == 0)
                Add(value);
        }*/
    }

    /// <summary>
    /// The name of the function.
    /// </summary>
    public readonly string Name;
    
    /// <summary>
    /// The name of the function.
    /// </summary>
    public readonly Symbol FuncSym;

    /// <summary>
    /// The labels of the function.
    /// </summary>
    public readonly List<Label> Labels = new();

    /// <summary>
    /// Whether a finally block is present.
    /// </summary>
    public bool HasFinally = false;

    /// <summary>
    /// Creates a new node representing a function.
    /// </summary>
    /// <param name="parent">The parent tree. Must not be null, and must be a scope tree.</param>
    /// <param name="name">The name of the function.</param>
    public FunctionNode(ScopeTree? parent, string name, Symbol sym, Statement? body) : base(parent)
    {
        Debug.Assert(parent != null);
        FuncSym = sym;
        Name = name;
        Body = body;
        if (Body != null)
            Body.Parent = this;
    }

    /// <summary>
    /// Whether a label has been declared.
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    public bool IsLabelDeclared(string name)
    {
        foreach (var label in Labels)
            if (label.Name == name)
                return true;
        return false;
    }

    public override string PrintElement(int indent_level = 0) => $"{Name} :: {FuncSym.Type!.Name}\n{Body!.PrettyPrint(indent_level + 1)}";
}
