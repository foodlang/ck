using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using ck.Lang.Type;
using System.Diagnostics;
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

    private enum ObjectAccess
    {
        /// <summary>
        /// Request object as a value.
        /// </summary>
        ByValue,

        /// <summary>
        /// Request object as a pointer.
        /// </summary>
        ByPointer,
    }

    public static XReg GenerateExpression(XMethod method, Expression e, ObjectAccess access, XReg? target)
    {
        Debug.Assert(e.Kind != ExpressionKind.Error);

        XReg Ret = target ?? method.AllocateRegister(XRegStorage.Fast);
        FType FT = e.Type!;
        XType XT = XType.FromFood(FT);

        switch (e.Kind)
        {
            // will be scalar, access is restricted to ByValue
        case ExpressionKind.Literal:
        {
            Debug.Assert(access == ObjectAccess.ByValue);

            var n_value = _Bits(e);
            method.Write(XOp.Const(XT, Ret, n_value));
            break;
        }

        case ExpressionKind.Identifier:
        {
            var ident = (string)e.Token.Value!;

            // register already stores address
            if (FT.Traits.HasFlag(TypeTraits.Members) || FT.Traits.HasFlag(TypeTraits.Array))
            {
                method.Write(XOp.Copy(XT, Ret, method.GetLocalOrParam(ident, out _ /* doesn't matter */)));
            }
            // scalar, by pointer
            else if (access == ObjectAccess.ByPointer)
            {
                method.Write(XOp.AddrOf(XType.Ptr, Ret, method.GetLocalOrParam(ident, out var is_param)));
                if (is_param)
                    method.ParamTable[ident] = method.ParamTable[ident] with { MustHavePhysicalStorage = true };
            }
            // scalar, by value
            else
            {
                method.Write(XOp.Copy(XT, Ret, method.GetLocalOrParam(ident, out _ /* doesn't matter */)));
            }

            break;
        }

        case ExpressionKind.AddressOf:
        case ExpressionKind.OpaqueAddressOf:
        {
            Debug.Assert(access == ObjectAccess.ByValue);
            GenerateExpression(method, e.GetChildrenExpressions().First(), ObjectAccess.ByPointer, Ret);
            break;
        }

        // just wrap around
        case ExpressionKind.UnaryPlus:
            GenerateExpression(method, e.GetChildrenExpressions().First(), access, target);
            break;

        case ExpressionKind.UnaryMinus:
        {
            Debug.Assert(access == ObjectAccess.ByValue);

            var T = method.AllocateRegister(XRegStorage.Fast);
            GenerateExpression(method, e.GetChildrenExpressions().First(), ObjectAccess.ByValue, T);
            method.Write(XOp.UMinus(XT, Ret, T));
            break;
        }

        case ExpressionKind.BitwiseNot:
        {
            Debug.Assert(access == ObjectAccess.ByValue);

            var T = method.AllocateRegister(XRegStorage.Fast);
            GenerateExpression(method, e.GetChildrenExpressions().First(), ObjectAccess.ByValue, T);
            method.Write(XOp.Bit_Not(XT, Ret, T));
            break;
        }

        case ExpressionKind.LogicalNot:
        {
            Debug.Assert(access == ObjectAccess.ByValue);

            var T = method.AllocateRegister(XRegStorage.Fast);
            GenerateExpression(method, e.GetChildrenExpressions().First(), ObjectAccess.ByValue, T);
            method.Write(XOp.LNot(XT, Ret, T));
            break;
        }

        }

        return Ret;
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
         * ---------------------------------------
         * Implementation Checklist (.=implemented, X=tested, !=lowering step reduced this)
         * [X] empty
         * [.] block
         * [.] labels
         * [.] finally
         * [ ] asm
         * [.] var init / expression
         * [!] switch .. case
         * [!] switch .. default
         * [.] if
         * [!] while
         * [X] do .. while
         * [X] for
         * [!] break
         * [!] continue
         * [.] goto
         * [ ] switch
         * [.] return
         * [!] assert :: for now it has no message and simply calls a function __ck_assert_abort()
         * [!] sponge
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
            _ = GenerateExpression(method, statement.Expressions[0], ObjectAccess.ByValue, null);
            return;

        case StatementKind.If:
        {
            var C_reg = GenerateExpression(method, statement.Expressions[0], ObjectAccess.ByValue, null);
            var Insn_if_true = XOp.If(C_reg, 0);
            var Insn_tail = XOp.Jmp(0);

            // if the condition was true, jump to the true block
            method.Write(Insn_if_true);

            // else code, if any
            if (statement.Statements.Count == 2)
                GenerateStatement(method, statement.Statements[1]);
            method.Write(Insn_tail); // jump past the true block

            // the true block
            Insn_if_true.Kx[0] = (nuint)method.CurCodePosition();
            GenerateStatement(method, statement.Statements[0]);

            // tail
            Insn_tail.Kx[0] = (nuint)method.CurCodePosition();
            return;
        }

        case StatementKind.Return:
        {
            // with return value
            if (statement.Expressions.Count == 1)
            {
                var e = statement.Expressions[0];
                var R_reg = GenerateExpression(method, e, ObjectAccess.ByValue, null);
                method.Write(XOp.Retv(XType.FromFood(e.Type!), R_reg));
                return;
            }

            // no return value
            method.Write(XOp.Ret());
            return;
        }

        case StatementKind.Switch:
        {
            // TODO: create a jump table somewhere else in the function that
            // can be pasted at this Insn location, essentially a table of
            // instruction offsets that you can use to find the address of
            // the corresponding case.
            //
            // as a temporary alternative, implement it as an if statement.
            method.Write(XOp.Nop());
            return;
        }

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
        foreach (var p in param_list)
            method.ParamTable.Add(p.Name, method.AllocateRegister(XRegStorage.Param));

        // locals
        var f_scope = tree.Body!.FindScopeTree()!;
        var locals = f_scope.GetAllUsedSymbols();
        foreach (var local in locals)
        {
            // TODO: figure out what variable must and must not have a physical address
            method.LocalTable.Add(local.Name, method.AllocateRegister(XRegStorage.LocalMustPhysical));

            // structures and arrays are stored as blocks, so the registers to perform operations
            // on them are treated as pointers.
            if (local.Type!.Traits.HasFlag(TypeTraits.Array) || local.Type!.Traits.HasFlag(TypeTraits.Members))
            {
                var blk = method.CreateBlock((nuint)local.Type!.SizeOf(), false);
                method.Write(XOp.Blkaddr(method.LocalTable[local.Name], blk.Index));
            }
        }

        // generate the code!
        GenerateStatement(method, tree.Body);
        method.MakeLabel(method.CurCodePosition(), "$leave");
        method.WriteFinallyBlock();
        method.Write(XOp.Leave());
        return method;
    }
}
