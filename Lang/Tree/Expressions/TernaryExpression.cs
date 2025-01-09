using ck.Lang.Tree;
using ck.Syntax;

namespace ck.Lang.Tree.Expressions;

/// <summary>
/// A ternary expression.
/// </summary>
public sealed class TernaryExpression : Expression
{
    /// <summary>
    /// First child of this expression.
    /// </summary>
    public Expression First;

    /// <summary>
    /// Second child of this expression.
    /// </summary>
    public Expression Second;

    /// <summary>
    /// Third child of this expression.
    /// </summary>
    public Expression Third;

    public override IEnumerable<SyntaxTree> GetChildren() => new[] { First, Second, Third };

    public TernaryExpression(Token op, ExpressionKind kind, Expression first, Expression second, Expression third) : base(op, kind)
    {
        First = first;
        Second = second;
        Third = third;
        first.Parent = this;
        second.Parent = this;
        third.Parent = this;
        ConstExpr = first.ConstExpr && second.ConstExpr && third.ConstExpr;
    }

    public override void SetChildren(Expression[] expressions)
    {
        First = expressions[0];
        Second = expressions[1];
        Third = expressions[2];
    }
}
