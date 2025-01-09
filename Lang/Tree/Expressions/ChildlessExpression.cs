using ck.Lang.Tree;
using ck.Syntax;
using LLVMSharp;
using LLVMSharp.Interop;

namespace ck.Lang.Tree.Expressions;

/// <summary>
/// An expression with no children.
/// </summary>
public sealed class ChildlessExpression : Expression
{
    public override IEnumerable<SyntaxTree> GetChildren() => Enumerable.Empty<SyntaxTree>();

    public ChildlessExpression(Token op_literal, ExpressionKind kind) : base(op_literal, kind) { }

    public override void SetChildren(Expression[] expressions) { } // do nothing
}
