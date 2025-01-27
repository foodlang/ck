using ck.Lang.Type;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.XIL;

public enum XTypeKind
{
    /// <summary>
    /// Unsigned integer
    /// </summary>
    U,

    /// <summary>
    /// Signed integer
    /// </summary>
    S,

    /// <summary>
    /// Floating-point
    /// </summary>
    Fp,

    /// <summary>
    /// (not-implemented) vector of Fp4
    /// </summary>
    Vs,

    /// <summary>
    /// (not-implemented) vector of Fp8
    /// </summary>
    Vd,
}

/// <summary>
/// X types.
/// </summary>
public sealed class XType
{
    /// <summary>
    /// Size, in bytes
    /// </summary>
    public readonly short Size;

    /// <summary>
    /// Whether the data is unsigned, signed or floating-point.
    /// </summary>
    public readonly XTypeKind Kind;

    public XType(short size, XTypeKind kind)
    {
        Size = size;
        Kind = kind;
    }

    public static readonly XType U0 = new(0, XTypeKind.U);
    public static readonly XType S8 = new(1, XTypeKind.S);
    public static readonly XType U8 = new(1, XTypeKind.U);
    public static readonly XType S16 = new(2, XTypeKind.S);
    public static readonly XType U16 = new(2, XTypeKind.U);
    public static readonly XType S32 = new(4, XTypeKind.S);
    public static readonly XType U32 = new(4, XTypeKind.U);
    public static readonly XType S64 = new(8, XTypeKind.S);
    public static readonly XType U64 = new(8, XTypeKind.U);
    public static readonly XType F32 = new(4, XTypeKind.Fp);
    public static readonly XType F64 = new(8, XTypeKind.Fp);
    public static XType Ptr => new(TargetMachine.PointerSize(), XTypeKind.U);

    /* currently unsupported types */
    public static readonly XType Mm128_F32 = new(16, XTypeKind.Vs);
    public static readonly XType Mm128_F64 = new(16, XTypeKind.Vd);
    public static readonly XType Mm256_F32 = new(32, XTypeKind.Vs);
    public static readonly XType Mm256_F64 = new(32, XTypeKind.Vd);
    public static readonly XType Mm512_F32 = new(64, XTypeKind.Vs);
    public static readonly XType Mm512_F64 = new(64, XTypeKind.Vd);

    public static XType FromFood(FType Food)
    {
        if (Food.Traits.HasFlag(TypeTraits.Void))
            return U0;

        if (Food.Traits.HasFlag(TypeTraits.Pointer))
            return Ptr;

        if (Food.Traits.HasFlag(TypeTraits.Unsigned))
            return Food.Size switch
            {
                1 => U8,
                2 => U16,
                4 => U32,
                8 => U64,
                _ => throw new InvalidOperationException()
            };

        if (Food.Traits.HasFlag(TypeTraits.Integer))
            return Food.Size switch
            {
                1 => S8,
                2 => S16,
                4 => S32,
                8 => S64,
                _ => throw new InvalidOperationException()
            };

        if (Food.Traits.HasFlag(TypeTraits.Floating))
            return Food.Size switch
            {
                4 => F32,
                8 => F64,
                _ => throw new InvalidOperationException()
            };

        /* probably ptr */
        return Ptr;
    }

    public override string ToString() => $"{Kind}{Size * 8}";
}
