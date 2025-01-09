using ck.Lang.Tree;
using ck.Syntax;

namespace ck.Lang.Tree.Expressions;

/// <summary>
/// A unary expression.
/// </summary>
public sealed class UnaryExpression : Expression
{
    /// <summary>
    /// The child of this expression.
    /// </summary>
    public Expression Child;

    public override IEnumerable<SyntaxTree> GetChildren() => new[] { Child };
    public override void SetChildren(Expression[] expressions) => Child = expressions[0];

    public UnaryExpression(Token op, ExpressionKind kind, Expression child) : base(op, kind)
    {
        child.Parent = this;
        Child = child;
        ConstExpr = child.ConstExpr;
    }
}
