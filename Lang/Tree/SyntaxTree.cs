using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree;

/// <summary>
/// An expression tree.
/// </summary>
public abstract class SyntaxTree
{
    /// <summary>
    /// Gets an enumerable representing all of the children
    /// of this node.
    /// </summary>
    public abstract IEnumerable<SyntaxTree> GetChildren();

    /// <summary>
    /// The parent node.
    /// </summary>
    public SyntaxTree? Parent;

    /// <summary>
    /// The attributes of the syntax tree.
    /// </summary>
    public AttributeClause AttributeClause = new();

    /// <summary>
    /// The id of the expression.
    /// </summary>
    public readonly uint Id;

    /// <summary>
    /// Finds the nearest (parent?) function.
    /// </summary>
    public FunctionNode? FindNearestFunction()
    {
        if (this is FunctionNode f)
            return f;
        return Parent != null ? Parent.FindNearestFunction() : null;
    }

    /// <summary>
    /// Finds the nearest scope tree, including the present node.
    /// </summary>
    /// <returns></returns>
    public ScopeTree? FindScopeTree()
    {
        if (this is ScopeTree this_casted)
            return this_casted;
        return FindNearestParentScope();
    }

    /// <summary>
    /// Tries to find the nearest parent scope tree node.
    /// </summary>
    /// <returns><c>null</c> if no parent node is a scope.</returns>
    public ScopeTree? FindNearestParentScope()
    {
        if (Parent == null)
            return null;

        if (Parent is ScopeTree scope_tree_parent)
            return scope_tree_parent;

        return Parent.FindNearestParentScope();
    }

    /// <summary>
    /// Returns true if this node has <paramref name="potential_parent"/>
    /// as a parent.
    /// </summary>
    public bool IsPartOf(SyntaxTree potential_parent)
    {
        if (this == potential_parent)
            return true;
        if (Parent == null)
            return false;

        if (Parent == potential_parent)
            return true;

        return Parent.IsPartOf(potential_parent);
    }

    /// <summary>
    /// Random used to generate GUIDs.
    /// </summary>
    private readonly static Random _random = new Random();

    public SyntaxTree()
    {
        Id = BitConverter.ToUInt32(Guid.NewGuid().ToByteArray(), _random.Next(0, 12));
    }

    public IEnumerable<Statement> AllStatements()
    {
        var children = GetChildren().Where(n => n is Statement).Cast<Statement>();
        return children.Concat(children.SelectMany(c => c.AllStatements()));
    }

    public IEnumerable<Statement> AllStatementsExcept(StatementKind kind)
    {
        var children = GetChildren().Where(n => n is Statement s && s.Kind != kind).Cast<Statement>();
        return children.Concat(children.SelectMany(c => c.AllStatements()));
    }

    /// <summary>
    /// Prints an element.
    /// </summary>
    public virtual string PrintElement(int indent_level = 0) => GetType().Name;

    public string PrettyPrint(int indent = 0)
        => PrintElement(indent) + "\n" + GetChildren().Aggregate(string.Empty, (a, child) => $"{a}{new string(' ', 2 * indent + 2)}{child.PrettyPrint(indent + 1)}\n");
}
