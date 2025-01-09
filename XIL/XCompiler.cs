using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using ck.Lang.Type;
using System.Runtime.InteropServices;
using System.Text;

namespace ck.XIL;

/// <summary>
/// Serializes AST into X code.
/// </summary>
public static class XCompiler
{
    [StructLayout(LayoutKind.Explicit)]
    private struct BitConversionStruct
    {
        [FieldOffset(0)]
        public long I;
        [FieldOffset(0)]
        public double F;
    }

    private static nuint _Bits(Expression e)
    {
        if ((e.Type!.Traits & TypeTraits.Integer) != 0)
            return (nuint)(decimal)e.Token.Value!;

        var @struct = new BitConversionStruct();
        @struct.F = (double)(decimal)e.Token.Value!;
        return (nuint)@struct.I;
    }

    public static XReg GenerateExpression(XMethod method, Expression expression)
    {

    }

    public static void GenerateStatement(XMethod method, Statement statement)
    {
        switch (statement.Kind)
        {

        /** statements that we dont care about because they dont exist anymore
         * 
         * while
         * do...while
         * for
         * 
         * break
         * continue
         * case
         * default
         * 
         * switch generation is alleged
         * 
         */

        // do nothing
        case StatementKind.Empty:
            return;

        case StatementKind.Block:
            foreach (var substmt in statement.Statements)
                GenerateStatement(method, substmt);
            return;

        case StatementKind.Label:
            method.MakeLabel(method.CurCodePosition(), (string)statement.Objects[0]);
            return;

        case StatementKind.Goto:
            method.Write(XOp.Jmp((nuint)method.GetLabel((string)statement.Objects[0])));
            return;

        // TODO: asm

        case StatementKind.Sponge:
            GenerateStatement(method, statement.Statements[0]);
            return;

        case StatementKind.Finally:
            method.ToggleFinallyWrite();
            GenerateStatement(method, statement.Statements[0]);
            method.ToggleFinallyWrite(); // revert
            return;

        case StatementKind.Expression:
        case StatementKind.VariableInit:
            _ = GenerateExpression(method, statement.Expressions[0]);
            return;

        }
    }

    // TODO: variadic generation
    public static XMethod GenerateFunction(FunctionNode tree, XModule module)
    {
        var func_Tsignature = (FunctionSubjugateSignature)(tree.FuncSym.Type!.SubjugateSignature!);
        var T_return = func_Tsignature.ReturnType;

        var method = new XMethod(module, tree.Name,
            [],
            [],
            []);

        // param list
        var param_ctx = func_Tsignature.Params;
        var param_list = param_ctx.Arguments;
        var reg = 0;
        foreach (var p in param_list)
            method.ParamTable.Add(p.Name, new XReg(reg++, XRegStorage.Param));

        // locals
        var f_scope = tree.Body!.FindScopeTree()!;
        var locals = f_scope.GetAllUsedSymbols();
        foreach (var local in locals)
        {
            // TODO: figure out what variable must and must not have a physical address
            method.LocalTable.Add(local.Name, new XReg(reg++, XRegStorage.LocalMustPhysical));

            // structures and arrays are stored as blocks, so the registers to perform operations
            // on them are treated as pointers.
            if (local.Type!.Traits.HasFlag(TypeTraits.Array) || local.Type!.Traits.HasFlag(TypeTraits.Members))
            {
                var blk = method.CreateBlock((nuint)local.Type!.SizeOf(), false);
                method.Write(XOp.Blkaddr(method.LocalTable[local.Name], blk.Index));
            }
        }
    }
}
