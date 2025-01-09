using ck.Lang.Tree;
using ck.Syntax;

namespace ck.Lang.Tree.Expressions;

/// <summary>
/// A binary expression.
/// </summary>
public sealed class BinaryExpression : Expression
{
    /// <summary>
    /// Left child of this expression.
    /// </summary>
    public Expression Left;

    /// <summary>
    /// Right child of this expression.
    /// </summary>
    public Expression Right;

    public override IEnumerable<SyntaxTree> GetChildren() => new[] { Left, Right };

    public BinaryExpression(Token op, ExpressionKind kind, Expression left, Expression right) : base(op, kind)
    {
        Left = left;
        Right = right;
        left.Parent = this;
        right.Parent = this;
        ConstExpr = left.ConstExpr && right.ConstExpr;
    }

    public override void SetChildren(Expression[] expressions)
    {
        Left = expressions[0];
        Right = expressions[1];
    }
}
