using ck.Lang.Tree;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using ck.Lang.Type;
using ck.Syntax;
using System.Collections.Generic;

namespace ck.Lowering.Lowers;

/// <summary>
/// Removes dead code and compiles some const expressions
/// that made it through the analyzer.
/// </summary>
public sealed class O0Pass : LowerOptStep
{

    private static int WeightOfExpression(Expression tree)
        => 1 + tree.GetRecursiveChildrenExpressions().Select(WeightOfExpression).Sum();

    private bool DoesExpressionHaveSideEffects(Expression e)
    {
        foreach (var child in e.GetChildrenExpressions())
            if (DoesExpressionHaveSideEffects(child))
                return true;

        return e.Kind switch
        {
            ExpressionKind.Assign
            or ExpressionKind.AssignSum
            or ExpressionKind.AssignDiff
            or ExpressionKind.AssignProduct
            or ExpressionKind.AssignQuotient
            or ExpressionKind.AssignRemainder
            or ExpressionKind.AssignBitwiseAnd
            or ExpressionKind.AssignBitwiseOr
            or ExpressionKind.AssignBitwiseXOr
            or ExpressionKind.AssignLeftShift
            or ExpressionKind.AssignRightShift
            or ExpressionKind.PrefixIncrement
            or ExpressionKind.PostfixIncrement
            or ExpressionKind.PrefixDecrement
            or ExpressionKind.PostfixDecrement
            or ExpressionKind.VariableInit
            or ExpressionKind.FunctionCall => true,
            _ => false,
        };
    }

    protected override int MutateExpression(ref Expression expr)
    {
        var score = 0;
        var expr_parent = expr.Parent;
        switch (expr.Kind)
        {
        case ExpressionKind.Conditional:
        {
            var cond_expr = (TernaryExpression)expr;
            var condition = cond_expr.First;
            var if_true = cond_expr.Second;
            var if_false = cond_expr.Third;

            score += MutateExpression(ref condition);
            score += MutateExpression(ref if_true);
            score += MutateExpression(ref if_false);

            if (condition.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (decimal)condition.Token.Value!;
                if (cmptime_conditional == 0)
                {
                    expr = if_false;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = if_true;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }

            /* nothing available. */
            return score;
        }

        case ExpressionKind.LogicalAnd:
        {
            var logical_and = (BinaryExpression)expr;
            var left = logical_and.Left;
            var right = logical_and.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (left.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (decimal)left.Token.Value!;
                // first condition is false, therefore discard whole
                if (cmptime_conditional == 0)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                // if one part is true, then reduce and only check for other
                expr = right;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }
            else if (right.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (decimal)left.Token.Value!;
                if (cmptime_conditional == 0)
                {
                    expr = right;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = left;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }

            /* nothing available. */
            return score;
        }

        case ExpressionKind.LogicalOr:
        {
            var logical_and = (BinaryExpression)expr;
            var left = logical_and.Left;
            var right = logical_and.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (left.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (decimal)left.Token.Value!;
                if (cmptime_conditional != 0)
                {
                    /* short circuit */
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = right;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }
            else if (right.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (decimal)right.Token.Value!;
                if (cmptime_conditional != 0)
                {
                    /* short circuit */
                    expr = right;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = left;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }

            /* nothing available. */
            return score;
        }

        case ExpressionKind.Add:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                if (incr == 0)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr < 0)
                {
                    expr = new BinaryExpression(new Token(expr.Token.Position, TokenKind.Minus), ExpressionKind.Subtract, left,
                        new ChildlessExpression(new Token(right.Token.Position, TokenKind.NumberLiteral, expr.Type!, incr), ExpressionKind.Literal)
                        { ConstExpr = true, Type = expr.Type! })
                   { Type = expr.Type! };
                   expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.Multiply:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                var incr_lb = Math.Log2((double)incr);

                if (incr == 0)
                {
                    expr = right;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr == 1)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                // x * 2 --> x << 1
                // x * 4 --> x << 2
                // x * 8 --> x << 3
                // ...
                else if (expr.Type!.Traits.HasFlag(TypeTraits.Integer) && (incr_lb - (long)incr_lb == 0))
                {
                    expr = new BinaryExpression(new Token(expr.Token.Position, TokenKind.LeftShift), ExpressionKind.LeftShift, left,
                        new ChildlessExpression(new Token(right.Token.Position, TokenKind.NumberLiteral, expr.Type!, incr_lb), ExpressionKind.Literal)
                        { ConstExpr = true, Type = expr.Type! })
                    { Type = expr.Type! };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }
            else if (left.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)left.Token.Value!;
                var incr_lb = Math.Log2((double)incr);

                if (incr == 0)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr == 1)
                {
                    expr = right;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                // x * 2 --> x << 1
                // x * 4 --> x << 2
                // x * 8 --> x << 3
                // ...
                else if (expr.Type!.Traits.HasFlag(TypeTraits.Integer) && (incr_lb - (long)incr_lb == 0))
                {
                    expr = new BinaryExpression(new Token(expr.Token.Position, TokenKind.LeftShift), ExpressionKind.LeftShift, right,
                        new ChildlessExpression(new Token(left.Token.Position, TokenKind.NumberLiteral, expr.Type!, incr_lb), ExpressionKind.Literal)
                        { ConstExpr = true, Type = expr.Type! })
                    { Type = expr.Type! };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.Divide:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                var incr_lb = Math.Log2((double)incr);

                if (incr == 1)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (expr.Type!.Traits.HasFlag(TypeTraits.Unsigned) && (incr_lb - (long)incr_lb == 0))
                {
                    expr = new BinaryExpression(new Token(expr.Token.Position, TokenKind.RightShift), ExpressionKind.RightShift, left,
                        new ChildlessExpression(new Token(right.Token.Position, TokenKind.NumberLiteral, expr.Type!, incr_lb), ExpressionKind.Literal)
                        { ConstExpr = true, Type = expr.Type! })
                    { Type = expr.Type! };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (expr.Type!.Traits.HasFlag(TypeTraits.Floating))
                {
                    var inv = Math.ReciprocalEstimate((double)incr);
                    expr = new BinaryExpression(
                        new Token(expr.Token.Position, TokenKind.Star), ExpressionKind.Multiply,
                        left, new ChildlessExpression(new Token(right.Token.Position, TokenKind.NumberLiteral, right.Type!, (decimal)inv), ExpressionKind.Literal)
                        {
                            ConstExpr = true,
                            Type = right.Type!
                        }
                        );
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.Modulo:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                if (incr == 1)
                {
                    expr = new ChildlessExpression(new Token(left.Token.Position, TokenKind.NumberLiteral, expr.Type, (decimal)0), ExpressionKind.Literal) { Type = expr.Type };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr == 2)
                {
                    expr = new BinaryExpression(
                        new Token(expr.Token.Position, TokenKind.And), ExpressionKind.BitwiseAnd,
                        left, new ChildlessExpression(new Token(right.Token.Position, TokenKind.NumberLiteral, right.Type!, 1m), ExpressionKind.Literal)
                        {
                            ConstExpr = true,
                            Type = right.Type!
                        }
                        );
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.AssignSum:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                if (incr == 0)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr == 1)
                {
                    expr = new UnaryExpression(expr.Token, ExpressionKind.PrefixIncrement, left) { Type = left.Type };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.AssignDiff:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                if (incr == 0)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr == 1)
                {
                    expr = new UnaryExpression(expr.Token, ExpressionKind.PrefixDecrement, left) { Type = left.Type };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.AssignProduct:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                if (incr == 0)
                {
                    expr = new BinaryExpression(expr.Token, ExpressionKind.Assign, left, right);
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
                else if (incr == 1)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.AssignQuotient:
        {
            var sum = (BinaryExpression)expr;
            var left = sum.Left;
            var right = sum.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (right.Kind == ExpressionKind.Literal)
            {
                var incr = (decimal)right.Token.Value!;
                if (incr == 1)
                {
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }
            }

            return score;
        }

        case ExpressionKind.LogicalNot:
        {
            var lnot = (UnaryExpression)expr;
            var child = lnot.Child;

            score += MutateExpression(ref child);

            if (child.Kind == ExpressionKind.Literal)
            {
                var boolean = (nint)child.Token.Value!;
                if (boolean == 0)
                {
                    expr = new ChildlessExpression(new Token(expr.Token.Position, TokenKind.NumberLiteral, expr.Type, (decimal)1), ExpressionKind.Literal) { Type = expr.Type };
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = new ChildlessExpression(new Token(expr.Token.Position, TokenKind.NumberLiteral, expr.Type, (decimal)1), ExpressionKind.Literal) { Type = expr.Type };
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }
            
            switch (child.Kind)
            {
            case ExpressionKind.Equal:
                child.Kind = ExpressionKind.NotEqual;
                expr = child;
                expr.Parent = expr_parent;
                score += 1;
                return score;

            case ExpressionKind.NotEqual:
                child.Kind = ExpressionKind.Equal;
                expr = child;
                expr.Parent = expr_parent;
                score += 1;
                return score;

            case ExpressionKind.Lower:
                child.Kind = ExpressionKind.GreaterEqual;
                expr = child;
                expr.Parent = expr_parent;
                score += 1;
                return score;

            case ExpressionKind.LowerEqual:
                child.Kind = ExpressionKind.Greater;
                expr = child;
                expr.Parent = expr_parent;
                score += 1;
                return score;

            case ExpressionKind.Greater:
                child.Kind = ExpressionKind.LowerEqual;
                expr = child;
                expr.Parent = expr_parent;
                score += 1;
                return score;

            case ExpressionKind.GreaterEqual:
                child.Kind = ExpressionKind.Lower;
                expr = child;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }

            return score;
        }

        case ExpressionKind.BitwiseAnd:
        {
            var logical_and = (BinaryExpression)expr;
            var left = logical_and.Left;
            var right = logical_and.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (left.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (nint)left.Token.Value!;
                if (cmptime_conditional == 0)
                {
                    /* short circuit */
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = right;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }
            else if (right.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (nint)right.Token.Value!;
                if (cmptime_conditional == 0)
                {
                    /* short circuit */
                    expr = right;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = right;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }

            /* nothing available. */
            return score;
        }

        case ExpressionKind.BitwiseOr:
        {
            var logical_and = (BinaryExpression)expr;
            var left = logical_and.Left;
            var right = logical_and.Right;

            score += MutateExpression(ref left);
            score += MutateExpression(ref right);

            if (left.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (nint)left.Token.Value!;
                if (cmptime_conditional != 0)
                {
                    /* short circuit */
                    expr = left;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = right;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }
            else if (right.Kind == ExpressionKind.Literal)
            {
                var cmptime_conditional = (nint)right.Token.Value!;
                if (cmptime_conditional != 0)
                {
                    /* short circuit */
                    expr = right;
                    expr.Parent = expr_parent;
                    score += 1;
                    return score;
                }

                expr = left;
                expr.Parent = expr_parent;
                score += 1;
                return score;
            }

            /* nothing available. */
            return score;
        }

        case ExpressionKind.PostfixIncrement:
        {
            var sum = (UnaryExpression)expr;
            var child = sum.Child;

            score += MutateExpression(ref child);

            if (expr_parent is Expression)
                return score;

            /* prefix increment is easier on SSA generation */
            expr.Kind = ExpressionKind.PrefixIncrement;
            score += 1;
            return score;
        }

        case ExpressionKind.PostfixDecrement:
        {
            var sum = (UnaryExpression)expr;
            var child = sum.Child;

            score += MutateExpression(ref child);

            if (expr_parent is Expression)
                return score;

            /* prefix increment is easier on SSA generation */
            expr.Kind = ExpressionKind.PrefixDecrement;
            score += 1;
            return score;
        }

        default:
        {
            var children = expr.GetChildrenExpressions().ToArray();
            var count = children.Length;
            for (var i = 0; i < count; i++)
            {
                var tmp = children[i];
                score += MutateExpression(ref tmp);
                children[i] = tmp;
            }
            expr.SetChildren(children);
            return score;
        }

        }
    }

    protected override int MutateStatement(ref Statement stmt)
    {
        var score = 0;
        var stmt_parent = stmt.Parent;
        for (var i = 0; i < stmt.Expressions.Count; i++)
        {
            var tmp = stmt.Expressions[i];
            score += MutateExpression(ref tmp);
            stmt.Expressions[i] = tmp;
        }
        for (var i = 0; i < stmt.Statements.Count; i++)
        {
            var tmp = stmt.Statements[i];
            score += MutateStatement(ref tmp);
            stmt.Statements[i] = tmp;
        }

        switch (stmt.Kind)
        {
        case StatementKind.Expression:
            if (!DoesExpressionHaveSideEffects(stmt.Expressions[0]))
                stmt = new Statement(stmt_parent, StatementKind.Empty, stmt.KwToken);
            break;

        case StatementKind.If:
        {
            var expr = stmt.Expressions[0];
            if (expr.Kind == ExpressionKind.Literal)
            {
                var constexpr = (decimal)expr.Token.Value!;
                if (constexpr == 0)
                {
                    stmt = stmt.Statements.Count == 2 ? stmt.Statements[1] : new Statement(stmt_parent, StatementKind.Empty, stmt.KwToken);
                    score += 1;
                }
                else
                {
                    stmt = stmt.Statements[0];
                    score += 1;
                }
            }
            break;
        }

        case StatementKind.While:
        {
            var condition = stmt.Expressions[0];
            if (condition.Kind == ExpressionKind.Literal && (decimal)condition.Token.Value! == 0)
            {
                stmt = new Statement(stmt_parent, StatementKind.Empty, stmt.KwToken);
                score += 1;
            }
            break;
        }

        case StatementKind.DoWhile:
        {
            var condition = stmt.Expressions[0];
            /* do once */
            if (condition.Kind == ExpressionKind.Literal && (decimal)condition.Token.Value! == 0)
            {
                stmt = stmt.Statements[0];
                score += 1;
            }
            else
            {
                /* expand into normal while loop */
                var newstmt = new Statement(stmt_parent, StatementKind.While, stmt.KwToken);

                /* infinite */
                newstmt.AddExpression(new ChildlessExpression(new Token(stmt.KwToken.Position, TokenKind.NumberLiteral, FType.BOOL, 1M), ExpressionKind.Literal));

                /* original body */
                var body = new Statement(stmt, StatementKind.Block, stmt.KwToken);
                body.AddStatement(stmt.Statements[0]);

                /* do condition `if (!original_condition) break;` */
                var cond = new Statement(body, StatementKind.If, stmt.KwToken);
                cond.AddExpression(new UnaryExpression(stmt.KwToken, ExpressionKind.LogicalNot, stmt.Expressions[0]));
                cond.AddStatement(new Statement(cond, StatementKind.Break, stmt.KwToken));

                body.AddStatement(cond);
                newstmt.AddStatement(body);

                /* new mutations will be sent to the while mutation block, no infinite recursion */
                stmt = newstmt;
                score += MutateStatement(ref stmt);
            }
            break;
        }

        case StatementKind.For:
        {
            var condition = stmt.GetDecoratedExpression("for-condition");
            var src_table = stmt.Table;
            if (condition is not null && condition.Kind == ExpressionKind.Literal && (decimal)condition.Token.Value! == 0)
            {
                stmt = new Statement(stmt_parent, StatementKind.Empty, stmt.KwToken);
                score += 1;
            }
            else
            {
                var initstmt = stmt.Statements.Count <= 2 ? stmt.Statements[0] : null;

                /* todo: misnomer, inside the loop */
                var loopstmt = stmt.Statements.Count == 2 ? stmt.Statements[1] : stmt.Statements[0];

                var postiterate = stmt.GetDecoratedExpression("for-iteration");

                Statement full /* whole statement */, loop /* sometimes part of full, the loop part */;
                if (initstmt is not null)
                {
                    full = new Statement(stmt_parent, StatementKind.Block, stmt.KwToken);
                    full.AddStatement(initstmt);

                    loop = new Statement(full, StatementKind.While, stmt.KwToken);
                    full.AddStatement(loop);
                }
                else
                {
                    loop = new Statement(stmt_parent, StatementKind.While, stmt.KwToken);
                    full = loop;
                }

                loop.AddExpression(condition!);
                /* if theres a post iteration expression, add it */
                if (postiterate is not null)
                {
                    var body_extended = new Statement(loop, StatementKind.Block, stmt.KwToken);
                    body_extended.AddStatement(loopstmt);
                    var postiteratestmt = new Statement(body_extended, StatementKind.Expression, postiterate.Token);
                    postiteratestmt.AddExpression(postiterate);
                    body_extended.AddStatement(postiteratestmt);
                    loop.AddStatement(body_extended);
                }
                else
                {
                    loop.AddStatement(loopstmt);
                }

                stmt = full;
                score += MutateStatement(ref stmt);
            }
            // make sure all symbols get transposed
            if (stmt.Table != src_table)
                stmt.Table.Merge(src_table);
            break;
        }

        case StatementKind.Sponge:
            stmt = stmt.Statements[0];
            break;

        case StatementKind.Assert:
            // FOR NOW...
            stmt = new Statement(stmt.Parent, StatementKind.Empty, new Token(stmt.KwToken.Position, TokenKind.Semicolon));
            break;

        }

        return score;
    }

    protected override int MutateFunction(ref FunctionNode func) => MutateStatement(ref func.Body!);
}
