using ck.Lang.Tree;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lowering;

/// <summary>
/// Lowering/optimization step
/// </summary>
public abstract class LowerOptStep
{
    /// <summary>
    /// The optimization level of the compiler.<br/>
    /// </summary>
    public static readonly int OptimizationLevel;

    /// <summary>
    /// Mutates an expression.
    /// </summary>
    /// <param name="expr">The expression to mutate.</param>
    /// <returns>Score of optimizations (always pick best optimization)</returns>
    protected abstract int MutateExpression(ref Expression expr);

    /// <summary>
    /// Mutates a statement.
    /// </summary>
    /// <param name="stmt">The statement to mutate.</param>
    /// <returns>Score of optimizations (always pick best optimization)</returns>
    protected abstract int MutateStatement(ref Statement stmt);

    /// <summary>
    /// Mutates a function.
    /// </summary>
    protected abstract int MutateFunction(ref FunctionNode func);

    /// <summary>
    /// Mutates the whole ast.
    /// </summary>
    public void MutateAst(FreeTree ast)
    {
        foreach (var ast_child in ast.GetChildren())
        {
            if (ast_child is not FunctionNode fn)
                continue;

            MutateFunction(ref fn);
        }
    }
}
