using ck.Lang.Type;
using ck.Lang.Tree.Expressions;
using ck.Syntax;
using ck.Util;

namespace ck.Lowering;

public sealed partial class Analyzer
{
    // Const expr reducer

    /// <summary>
    /// Reduces a const expression into a literal value. TODO: add support for
    /// conditional optimization & logical or optimization
    /// </summary>
    /// <param name="source">The source expression.</param>
    /// <returns>The result.</returns>
    public static Expression ReduceConstExpr(Expression source)
    {
        var results = source.GetChildrenExpressions().Select(x =>
        {
            var v = ReduceConstExpr(x).Token.Value!;
            return (decimal)v;
        });
        var st = source.Token;

        // creates a new constexpr result. If override_T != null, set the type to that
        ChildlessExpression Create(decimal constv, FType? override_T = null)
         => new ChildlessExpression(
             new(st.Source!, st.Position, TokenKind.NumberLiteral, override_T == null ? source.Type! : override_T, constv),
             ExpressionKind.Literal)
         { Type = override_T is null ? source.Type! : override_T };

        return source.Kind switch
        {
            ExpressionKind.Literal => source,
            // TODO: nameof, etc
            ExpressionKind.Cast => Create(results.ElementAt(0), source.Type!),
            ExpressionKind.UnaryPlus => source,
            ExpressionKind.UnaryMinus => Create(-results.ElementAt(0)),
            ExpressionKind.LogicalNot => Create((results.ElementAt(0) == 0).Int(), FType.BOOL),
            ExpressionKind.BitwiseNot => Create(~(long)results.ElementAt(0)),
            ExpressionKind.Multiply => Create(results.ElementAt(0) * results.ElementAt(1)),
            ExpressionKind.Divide => Create(results.ElementAt(0) / results.ElementAt(1)),
            ExpressionKind.Modulo => Create((long)results.ElementAt(0) % (long)results.ElementAt(1)),
            ExpressionKind.Add => Create(results.ElementAt(0) + results.ElementAt(1)),
            ExpressionKind.Subtract => Create(results.ElementAt(0) - results.ElementAt(1)),
            ExpressionKind.LeftShift => Create((long)results.ElementAt(0) << (int)results.ElementAt(1)),
            ExpressionKind.RightShift => Create((long)results.ElementAt(0) >> (int)results.ElementAt(1)),
            ExpressionKind.Lower => Create((results.ElementAt(0) < results.ElementAt(1)).Int()),
            ExpressionKind.LowerEqual => Create((results.ElementAt(0) <= results.ElementAt(1)).Int()),
            ExpressionKind.Greater => Create((results.ElementAt(0) > results.ElementAt(1)).Int()),
            ExpressionKind.GreaterEqual => Create((results.ElementAt(0) >= results.ElementAt(1)).Int()),
            ExpressionKind.Equal => Create((results.ElementAt(0) == results.ElementAt(1)).Int()),
            ExpressionKind.NotEqual => Create((results.ElementAt(0) != results.ElementAt(1)).Int()),
            ExpressionKind.BitwiseAnd => Create((long)results.ElementAt(0) & (long)results.ElementAt(1)),
            ExpressionKind.BitwiseXOr => Create((long)results.ElementAt(0) ^ (long)results.ElementAt(1)),
            ExpressionKind.BitwiseOr => Create((long)results.ElementAt(0) | (long)results.ElementAt(1)),
            ExpressionKind.LogicalAnd => Create((results.ElementAt(0) != 0 && results.ElementAt(1) != 0).Int()),
            ExpressionKind.LogicalOr => Create((results.ElementAt(0) != 0 || results.ElementAt(1) != 0).Int()),
            ExpressionKind.Conditional => Create(results.ElementAt(0) != 0 ? results.ElementAt(1) : results.ElementAt(2)),
            _ => source,
        };
    }
}
