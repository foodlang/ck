using ck.Lang.Tree;
using ck.Lang.Tree.Scopes;
using ck.Syntax;

namespace ck.Lang.Tree.Expressions;

/// <summary>
/// An expression holding multiple children expressions.
/// </summary>
public sealed class AggregateExpression : Expression
{
    /// <summary>
    /// The children.
    /// </summary>
    public IEnumerable<Expression> Aggregate;

    /// <summary>
    /// The names of the aggregate members. Used for initializer lists.
    /// </summary>
    public IEnumerable<string> AggregateNames;

    /// <summary>
    /// The reftypes for the arguments. Used for function calls.
    /// </summary>
    public IEnumerable<Symbol.ArgumentRefType> RefTypes;

    public override IEnumerable<SyntaxTree> GetChildren() => Aggregate;

    public AggregateExpression(
        Token op, ExpressionKind kind,
        IEnumerable<Expression> aggregate,
        IEnumerable<string>? aggregate_names = null,
        IEnumerable<Symbol.ArgumentRefType>? ref_types = null) : base(op, kind)
    {
        Aggregate = aggregate;
        ConstExpr = aggregate.Any();
        foreach (var elem in aggregate)
        {
            elem.Parent = this;
            ConstExpr = ConstExpr && elem.ConstExpr;
        }
        AggregateNames = aggregate_names ?? Enumerable.Empty<string>();
        RefTypes = ref_types ?? Enumerable.Empty<Symbol.ArgumentRefType>();
    }

    public override void SetChildren(Expression[] expressions) => Aggregate = expressions;
}
