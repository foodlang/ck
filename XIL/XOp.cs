using ck.Lang.Tree.Expressions;
using System.Runtime.Intrinsics.Arm;
using System.Text;

namespace ck.XIL;

/// <summary>
/// Emits an X-op.
/// </summary>
/// <typeparam name="T">Result type</typeparam>
/// <typeparam name="U">Context struct type</typeparam>
/// <param name="context">Context</param>
/// <param name="op">Opcode to emit</param>
public delegate T XOpEmitter<T, U>(in U context, XOp op);

/// <summary>
/// Produces an X expression and pastes the contents into a page.
/// </summary>
public delegate void XOpExprPattern(Expression e, List<XOp> page);

/// <summary>
/// X-data operation (instruction)
/// </summary>
public sealed class XOp
{
    public string PrettyPrint()
    {
        var sb = new StringBuilder();
        sb.Append($"{Op.ToString().ToLowerInvariant()}::{Type.ToString()} ");
        sb.Append($"Rd={Rd.R}");
        if (Rsx.Length != 0)
            sb.Append($", Rsx=[{Rsx.Select(r => r.R.ToString()).Aggregate((a, b) => $"{a}, {b}")}]");
        if (Kx.Length != 0)
            sb.Append($", Kx=[{Kx.Select(k => k.ToString()).Aggregate((a, b) => $"{a}, {b}")}]");
        if (Fn is not null)
            sb.Append($", Fn={Fn}");
        return sb.ToString();
    }

    /// <summary>
    /// The operation to perform.
    /// </summary>
    public readonly XOps Op;

    /// <summary>
    /// The type of the operation.
    /// </summary>
    public readonly XType Type;

    /// <summary>
    /// Src types.
    /// </summary>
    public readonly XType[] Tsx;

    /// <summary>
    /// Destination register
    /// </summary>
    public readonly XReg Rd;

    /// <summary>
    /// Input registers
    /// </summary>
    public readonly XReg[] Rsx;

    /// <summary>
    /// Input constants
    /// </summary>
    public readonly nuint[] Kx;

    /// <summary>
    /// Function
    /// </summary>
    public readonly string? Fn;

    private XOp(XOps code, XType T, XReg rd, XReg[] rsx, nuint[] kx, XType[]? tsx = null, string? fn = null)
    {
        Op = code;
        Type = T;
        Rd = rd;

        if (tsx is not null)
        {
            Tsx = new XType[tsx.Length];
            tsx.CopyTo(Tsx, 0);
        }
        else Tsx = [];

        Rsx = new XReg[rsx.Length];
        Kx = new nuint[kx.Length];
        rsx.CopyTo(Rsx, 0);
        kx.CopyTo(Kx, 0);
        Fn = fn;
    }

    private static XOp _Binary(XType T, XOps Op, XReg Rd, XReg Rs0, XReg Rs1)
        => new(Op, T, Rd, [Rs0, Rs1], []);
    private static XOp _Unary(XType T, XOps Op, XReg Rd, XReg Rs0)
        => new(Op, T, Rd, [Rs0], []);

    public static XOp Nop()
        => new(XOps.Nop, XType.U0, XReg.NoReg, [], []);

    public static XOp Const(XType T, XReg Rd, nuint K0)
        => new(XOps.ConstSet, T, Rd, [], [K0]);

    public static XOp Set(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.Set, Rd, Rs0);

    /*public static XOp ZExt(XType dT, XType sT, XReg Rd, XReg Rs0)
        => new(XOps.ZExt, dT, Rd, [Rs0], [], [sT]);

    public static XOp SExt(XType dT, XType sT, XReg Rd, XReg Rs0)
        => new(XOps.SExt, dT, Rd, [Rs0], [], [sT]);*/

    public static XOp Read(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.Read, Rd, Rs0);

    public static XOp Write(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.Write, Rd, Rs0);

    public static XOp Copy(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.Copy, Rd, Rs0);

    public static XOp AddrOf(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.AddressOf, Rd, Rs0);

    public static XOp Increment(XType T, XReg Rd, nuint K0)
        => new(XOps.Increment, T, Rd, [], [K0]);

    public static XOp Decrement(XType T, XReg Rd, nuint K0)
        => new(XOps.Decrement, T, Rd, [], [K0]);

    public static XOp MulConst(XType T, XReg Rd, nuint K0)
        => new(XOps.MulConst, T, Rd, [], [K0]);

    public static XOp DivConst(XType T, XReg Rd, nuint K0)
        => new(XOps.DivConst, T, Rd, [], [K0]);

    public static XOp UMinus(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.UnaryMinus, Rd, Rs0);

    public static XOp Add(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Add, Rd, Rs0, Rs1);

    public static XOp Sub(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Sub, Rd, Rs0, Rs1);

    public static XOp Mul(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Mul, Rd, Rs0, Rs1);

    public static XOp Div(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Div, Rd, Rs0, Rs1);

    public static XOp Rem(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Rem, Rd, Rs0, Rs1);

    public static XOp Bit_Not(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.BNot, Rd, Rs0);

    public static XOp Bit_And(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.BAnd, Rd, Rs0, Rs1);

    public static XOp Bit_Xor(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.BXor, Rd, Rs0, Rs1);

    public static XOp Bit_Or(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.BOr, Rd, Rs0, Rs1);

    public static XOp Bit_Lsh(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.BLsh, Rd, Rs0, Rs1);

    public static XOp Bit_Rsh(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.BRsh, Rd, Rs0, Rs1);

    public static XOp Arthm_Lsh(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.ALsh, Rd, Rs0, Rs1);

    public static XOp Arthm_Rsh(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.ARsh, Rd, Rs0, Rs1);

    public static XOp Lower(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Lower, Rd, Rs0, Rs1);

    public static XOp LowerEqual(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.LowerEqual, Rd, Rs0, Rs1);

    public static XOp Greater(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Greater, Rd, Rs0, Rs1);

    public static XOp GreaterEqual(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.GreaterEqual, Rd, Rs0, Rs1);

    public static XOp Equal(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.Equal, Rd, Rs0, Rs1);

    public static XOp NotEqual(XType T, XReg Rd, XReg Rs0, XReg Rs1)
        => _Binary(T, XOps.NotEqual, Rd, Rs0, Rs1);

    public static XOp LNot(XType T, XReg Rd, XReg Rs0)
        => _Unary(T, XOps.LNot, Rd, Rs0);

    public static XOp Jmp(nuint Insn)
        => new(XOps.Jmp, XType.U0, XReg.NoReg, [], [Insn]);

    public static XOp IJmp(XReg Rd)
        => new(XOps.IJmp, XType.U0, Rd, [], []);

    public static XOp If(XType T, XReg Rd, nuint Insn)
        => new(XOps.If, T, Rd, [], [Insn]);

    public static XOp Ifz(XType T, XReg Rd, nuint Insn)
        => new(XOps.If, T, Rd, [], [Insn]);

    public static XOp FCall(XType T, XReg Rd, in XReg[] RsX, in XType[] TsX, string fn)
        => new XOp(XOps.FCall, T, Rd, RsX, [], TsX, fn);

    public static XOp PCall(in XReg[] RsX, in XType[] TsX, string fn)
        => new XOp(XOps.PCall, XType.U0, XReg.NoReg, RsX, [], TsX, fn);

    /// <summary>
    /// only provide TsX for arguments
    /// </summary>
    public static XOp FiCall(XType T, XReg Rd, in XReg[] RsX, in XType[] TsX)
        => new XOp(XOps.FiCall, T, Rd, RsX, [], [XType.Ptr, ..TsX]);

    /// <summary>
    /// only provide TsX for arguments
    /// </summary>
    public static XOp PiCall(in XReg[] RsX, in XType[] TsX)
        => new XOp(XOps.PiCall, XType.U0, XReg.NoReg, RsX, [], [XType.Ptr, .. TsX]);

    public static XOp Retv(XType T, XReg Rd)
        => new XOp(XOps.Retv, T, Rd, [], []);

    public static XOp Ret()
        => new XOp(XOps.Ret, XType.U0, XReg.NoReg, [], []);

    public static XOp Blkzero(XReg Rd, nuint K0)
        => new XOp(XOps.Blkzero, XType.Ptr, Rd, [], [K0]);

    public static XOp Blkcopy(XReg Rd, XReg Rs0, nuint K0)
        => new XOp(XOps.Blkcopy, XType.Ptr, Rd, [Rs0], [K0]);

    public static XOp Blkmove(XReg Rd, XReg Rs0, nuint K0)
        => new XOp(XOps.Blkmove, XType.Ptr, Rd, [Rs0], [K0]);

    public static XOp Blkaddr(XReg Rd, nuint K0)
        => new XOp(XOps.Blkaddr, XType.Ptr, Rd, [], [K0]);

    public static XOp IntToFloat(XType dT, XType sT, XReg Rd, XReg Rs0)
        => new XOp(XOps.IntToFloat, dT, Rd, [Rs0], [], [sT]);

    public static XOp FloatToInt(XType dT, XType sT, XReg Rd, XReg Rs0)
        => new XOp(XOps.FloatToInt, dT, Rd, [Rs0], [], [sT]);

    public static XOp FloatResize(XType dT, XType sT, XReg Rd, XReg Rs0)
        => new XOp(XOps.FloatResize, dT, Rd, [Rs0], [], [sT]);

    public static XOp Icast(XType dT, XType sT, XReg Rd, XReg Rs0)
        => new XOp(XOps.Icast, dT, Rd, [Rs0], [], [sT]);

    public static XOp Leave()
        => new XOp(XOps.Leave, XType.U0, XReg.NoReg, [], []);

    public static XOp InlineAsm(uint K0)
        => new XOp(XOps.Asm, XType.U0, XReg.NoReg, [], [K0]);
}
