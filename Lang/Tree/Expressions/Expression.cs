using ck.Lang.Type;
using ck.Lang.Tree;
using ck.Syntax;

namespace ck.Lang.Tree.Expressions;

/// <summary>
/// An expression.
/// </summary>
public abstract class Expression : SyntaxTree
{
    /// <summary>
    /// The type of the expression. If this is null,
    /// it means that a type must be bound to it.
    /// </summary>
    public FType? Type;

    /// <summary>
    /// Whether the expression is an lvalue.
    /// </summary>
    public bool LValue = false;

    /// <summary>
    /// Whether the expression is an expression that
    /// can be evaluated at compile-time.
    /// </summary>
    public bool ConstExpr = false;

    /// <summary>
    /// Whether the expression is a label reference.
    /// </summary>
    public bool IsLabelRef = false;

    /// <summary>
    /// The kind of the expression.
    /// </summary>
    public ExpressionKind Kind;

    /// <summary>
    /// The operator or literal value's token.
    /// </summary>
    public readonly Token Token;

    /// <summary>
    /// The decorations of the expressions.
    /// </summary>
    public readonly List<string> Decorations = new();

    /// <summary>
    /// Sets the children of this expression.
    /// </summary>
    public abstract void SetChildren(Expression[] expressions);

    public IEnumerable<Expression> GetChildrenExpressions() => GetChildren().Cast<Expression>();

    public IEnumerable<Expression> GetRecursiveChildrenExpressions() => GetChildrenExpressions().Concat(GetChildrenExpressions().SelectMany(e => e.GetRecursiveChildrenExpressions()));

    /// <summary>
    /// Creates a new expression.
    /// </summary>
    public Expression(Token op_literal, ExpressionKind kind)
    {
        Token = op_literal;
        Kind = kind;
    }

    public override string PrintElement(int indent_level = 0) => $"{Kind}";
}
