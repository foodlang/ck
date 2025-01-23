using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Reflection.Emit;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lowering.Lowers;

/// <summary>
/// Prepares the code for generation.
/// </summary>
public sealed class LowPass : LowerOptStep
{
    public Expression DecomposeBinaryAssign(BinaryExpression src, ExpressionKind new_kind)
        => new BinaryExpression(
                src.Token, ExpressionKind.Assign, src.Left,
                new BinaryExpression(src.Token, ExpressionKind.Subtract, src.Left, src.Right)
                { Type = src.Left.Type })
        { Type = src.Type, Parent = src };

    protected override int MutateExpression(ref Expression expr)
    {
        var expr_parent = expr.Parent;

        var assign_kind = expr.Kind switch
        {
            ExpressionKind.AssignSum => ExpressionKind.Add,
            ExpressionKind.AssignDiff => ExpressionKind.Subtract,
            ExpressionKind.AssignProduct => ExpressionKind.Multiply,
            ExpressionKind.AssignQuotient => ExpressionKind.Divide,
            ExpressionKind.AssignBitwiseAnd => ExpressionKind.BitwiseAnd,
            ExpressionKind.AssignBitwiseOr => ExpressionKind.BitwiseOr,
            ExpressionKind.AssignBitwiseXOr => ExpressionKind.BitwiseXOr,
            ExpressionKind.AssignLeftShift => ExpressionKind.LeftShift,
            ExpressionKind.AssignRightShift => ExpressionKind.RightShift,
            _ => ExpressionKind.Error,
        };

        if (assign_kind != ExpressionKind.Error)
        {
            expr = DecomposeBinaryAssign((BinaryExpression)expr, assign_kind);
            return 0;
        }

        return 0;
    }

    protected override int MutateStatement(ref Statement stmt)
    {
        var stmt_list = new Statement[stmt.Statements.Count];
        for (var i = 0; i < stmt_list.Length; i++)
        {
            stmt_list[i] = stmt.Statements[i];
            MutateStatement(ref stmt_list[i]);
        }
        stmt.Statements.Clear();
        stmt.Statements.AddRange(stmt_list);

        var expr_list = new Expression[stmt.Expressions.Count];
        for (var i = 0; i < stmt_list.Length; i++)
        {
            expr_list[i] = stmt.Expressions[i];
            MutateExpression(ref expr_list[i]);
        }
        stmt.Expressions.Clear();
        stmt.Expressions.AddRange(expr_list);

        var stmt_parent = stmt.Parent;
        switch (stmt.Kind)
        {

        case StatementKind.VariableInit:
        {
            var expr = stmt.Expressions[0];
            if (expr.Kind == ExpressionKind.Assign)
            {
                var assign = (BinaryExpression)expr;
                if (assign.Right.Kind == ExpressionKind.CollectionInitialization)
                {
                    var blk = new Statement(stmt_parent, StatementKind.Block, stmt.KwToken);
                    var collection = (AggregateExpression)assign.Right;
                    var i = 0;
                    foreach (var element in collection.GetChildrenExpressions())
                    {
                        var single_assign = new Statement(blk, StatementKind.Expression, stmt.KwToken);
                        single_assign.Expressions.Add(
                            new BinaryExpression(stmt.KwToken, ExpressionKind.Assign,
                            new BinaryExpression(stmt.KwToken, ExpressionKind.ArraySubscript, assign.Right,
                            new ChildlessExpression(new Token(stmt.KwToken.Position, TokenKind.NumberLiteral, FType.I64, (decimal)i), ExpressionKind.Literal)
                            { Type = FType.I64 })
                            { Type = element.Type }, element));
                        blk.AddStatement(single_assign);
                        i++;
                    }
                    stmt = blk;
                }
            }
            break;
        }

        // =============== CONTROL FLOW REDUCER ===============
        // All control flow is reduced into goto/if statements.
        // O0 already reduces for and do while into normal while
        // loops.
        // 
        // Format for break destinations: $<BrStmt>_tail
        // Format for continue destination: $<ContinueStmt>_head
        // ====================================================

        case StatementKind.While:
        {
            var condition = stmt.Expressions[0];
            var while_id = stmt.Id;
            var blk = new Statement(stmt_parent, StatementKind.Block, stmt.KwToken);

            // O0 eliminated the zero case, meaning this is an infinite loop.
            if (condition.Kind == ExpressionKind.Literal)
            {
                // continue destination
                var L_head = new Statement(blk, StatementKind.Label, stmt.KwToken);
                L_head.Objects.Add($"${while_id}_head");
                blk.AddStatement(L_head);
                blk.AddStatement(stmt.Statements[0]); // body

                var L_goto_head = new Statement(blk, StatementKind.Goto, stmt.KwToken);
                L_goto_head.Objects.Add($"${while_id}_head");
                blk.AddStatement(L_goto_head);

                // break destination
                var L_tail = new Statement(blk, StatementKind.Label, stmt.KwToken);
                L_tail.Objects.Add($"${while_id}_tail");
                blk.AddStatement(L_tail);
            }
            // we have a condition. no worries, this is still quite easy:
            else
            {
                // continue destination
                var L_head = new Statement(blk, StatementKind.Label, stmt.KwToken);
                L_head.Objects.Add($"${while_id}_head");
                blk.AddStatement(L_head);

                // the condition is represented by a then-only if with a continue statement
                // at the end of the loop.
                var condition_stmt = new Statement(blk, StatementKind.If, stmt.KwToken);
                condition_stmt.AddExpression(condition);

                var true_body = new Statement(condition_stmt, StatementKind.Block, stmt.KwToken);
                true_body.AddStatement(stmt.Statements[0]); // the while body
                var L_goto_head = new Statement(true_body, StatementKind.Goto, stmt.KwToken);
                L_goto_head.Objects.Add($"${while_id}_head");
                true_body.AddStatement(L_goto_head); // the 'continue' statement

                condition_stmt.AddStatement(true_body);

                // break destination
                var L_tail = new Statement(blk, StatementKind.Label, stmt.KwToken);
                L_tail.Objects.Add($"${while_id}_tail");

                blk.AddStatement(condition_stmt);
                blk.AddStatement(L_tail);
            }

            stmt = blk;
            break;
        }

            // goal: find closest breakable statement, and aim for the tail C:
        case StatementKind.Break:
        {
            var breakable = stmt.FindNearestBreakableStatement()!;
            var goto_breakable_tail = new Statement(stmt_parent, StatementKind.Goto, stmt.KwToken);
            goto_breakable_tail.Objects.Add($"${breakable.Id}_tail");
            stmt = goto_breakable_tail;
            break;
        }

            // goal: find closest breakable statement, and aim for the head C:<
        case StatementKind.Continue:
        {
            var continueable = stmt.FindNearestBreakableStatement()!;
            var goto_breakable_head = new Statement(stmt_parent, StatementKind.Goto, stmt.KwToken);
            goto_breakable_head.Objects.Add($"${continueable.Id}_head");
            stmt = goto_breakable_head;
            break;
        }

        case StatementKind.Switch:
        {
            if (stmt.SwitchDecorated)
                break;

            var switch_id = stmt.Id;
            var blk = new Statement(stmt_parent, StatementKind.Block, stmt.KwToken);
            var L_head = new Statement(stmt_parent, StatementKind.Label, stmt.KwToken);
            L_head.Objects.Add($"${switch_id}_head");

            blk.Add(L_head);
            stmt.SwitchDecorated = true;
            blk.Add(stmt); // wait... trust me

            var L_tail = new Statement(stmt_parent, StatementKind.Label, stmt.KwToken);
            L_tail.Objects.Add($"${switch_id}_tail");
            blk.Add(L_tail);

            stmt = blk;
            break;
        }

        case StatementKind.SwitchCase:
        {
            var switch_stmt = stmt.FindNearestSwitch()!;
            var switch_id = switch_stmt.Id;
            

            var label = new Statement(stmt_parent, StatementKind.Label, stmt.KwToken);
            var name = $"{switch_id}_case{label.Id}";
            label.Objects.Add(name);
            switch_stmt.Objects.Add((Name: name, Cval: (decimal)stmt.Expressions[0].Token.Value!));
            stmt = label;
            break;
        }

        case StatementKind.SwitchDefault:
        {
            var switch_stmt = stmt.FindNearestSwitch()!;
            var switch_id = switch_stmt.Id;
            var label = new Statement(stmt_parent, StatementKind.Label, stmt.KwToken);
            label.Objects.Add($"${switch_id}_default");
            stmt = label;
            break;
        }

        case StatementKind.Assert:
        {
            var if_stmt = new Statement(stmt_parent, StatementKind.If, stmt.KwToken);
            if_stmt.AddExpression(new UnaryExpression(stmt.KwToken, ExpressionKind.LogicalNot, stmt.Expressions[0]) { Type = stmt.Expressions[0].Type! });
            var failure_code = new Statement(if_stmt, StatementKind.Expression, stmt.KwToken);
            failure_code.AddExpression(
                new AggregateExpression(stmt.KwToken, ExpressionKind.FunctionCall, [
                    new ChildlessExpression(new Token(stmt.KwToken.Position, TokenKind.Identifier, null, "__ck_assert_abort"), ExpressionKind.Identifier)
                    { Type = FType.Function(FType.VOID, new FunctionArgumentSignature([], null)) }
                    ]));
            if_stmt.AddStatement(
                failure_code
                );
            stmt = if_stmt;
            break;
        }

        }


        return 0;
    }

    protected override int MutateFunction(ref FunctionNode func) => MutateStatement(ref func.Body!);
}
