using ck.Lang;
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
        {
            if (e.Token.Value!.GetType() == typeof(decimal))
                return (nuint)(decimal)e.Token.Value!;
            return (nuint)(double)e.Token.Value!;
        }

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

    private static XReg GenerateExpression(XMethod method, Expression e, ObjectAccess access, XReg? target)
    {
        Debug.Assert(e.Kind != ExpressionKind.Error);

        XReg Ret = target ?? method.AllocateRegister(XRegStorage.Fast);
        FType FT = e.Type!;
        XType XT = XType.FromFood(FT);

        // TODO: variadic function calls
        // TODO: member access
        switch (e.Kind)
        {

        case ExpressionKind.MemberAccess:
        {
            var binary_expression = (BinaryExpression)e;
            var source = binary_expression.Left;
            var src_type = source.Type!;
            var looking_for = (string)binary_expression.Right.Token.Value!;

            StructSubjugateSignature struct_type;
            struct_type =
                src_type.Traits.HasFlag(TypeTraits.Pointer)
                ? (StructSubjugateSignature)((PointerSubjugateSignature)binary_expression.Left.Type!.SubjugateSignature!).ObjectType.SubjugateSignature!
                : (StructSubjugateSignature)binary_expression.Left.Type!.SubjugateSignature!;

            nint total_offset = 0;
            // sponge
            var found = false;
            foreach (var member in struct_type.Members)
            {
                if (member.Name == looking_for)
                {
                    found = true;
                    break;
                }
                total_offset += member.Type.SizeOf();
            }
            Debug.Assert(found);

            // generating code
            _ = GenerateExpression(method, source, ObjectAccess.ByPointer, Ret);
            method.Write(XOp.Increment(XType.Ptr, Ret, (nuint)total_offset));

            if (access == ObjectAccess.ByPointer || FT.Traits.HasFlag(TypeTraits.Members) || FT.Traits.HasFlag(TypeTraits.Array))
                break;

            method.Write(XOp.Read(XT, Ret, Ret));
            break;
        }

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

        case ExpressionKind.Dereference:
        {
            if (access == ObjectAccess.ByPointer || FT.Traits.HasFlag(TypeTraits.Members) || FT.Traits.HasFlag(TypeTraits.Array))
                GenerateExpression(method, e.GetChildrenExpressions().First(), ObjectAccess.ByValue, Ret);
            else
            {
                var Ptr = GenerateExpression(method, e.GetChildrenExpressions().First(), ObjectAccess.ByValue, null);
                method.Write(XOp.Read(XT, Ret, Ptr));
            }

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

        case ExpressionKind.CollectionInitialization:
        {
            var e_aggregate = (AggregateExpression)e;
            var size = e_aggregate.Type!.SizeOf();

            if (e_aggregate.Aggregate.Count() != 0)
            {
                var blk = method.CreateBlock((nuint)size, false);
                method.Write(XOp.Blkaddr(Ret, blk.Index));
                var Dr_Intermediate = method.AllocateRegister(XRegStorage.Fast);
                method.Write(XOp.Copy(XType.Ptr, Dr_Intermediate, Ret));

                var T_elem = e_aggregate.Aggregate.First().Type!;
                var XT_elem = XType.FromFood(T_elem);
                var T_is_blk = T_elem.Traits.HasFlag(TypeTraits.Members) || T_elem.Traits.HasFlag(TypeTraits.Array);
                var elem_access = T_is_blk ? ObjectAccess.ByPointer : ObjectAccess.ByValue;
                var elem_size = (nuint)T_elem.SizeOf();

                foreach (var elem in e_aggregate.Aggregate)
                {
                    var R = GenerateExpression(method, elem, elem_access, null);

                    if (T_is_blk)
                    {
                        method.Write(XOp.Blkcopy(Dr_Intermediate, R, elem_size));
                    }
                    else
                    {
                        method.Write(XOp.Write(XT_elem, Dr_Intermediate, R));
                    }
                    method.Write(XOp.Increment(XType.Ptr, Dr_Intermediate, elem_size));
                }
            }
            break;
        }

        case ExpressionKind.New:
        case ExpressionKind.Default:
            Console.WriteLine("codegen: new/default is unsupported");
            break;

        case ExpressionKind.Cast:
        {
            var Child = (Expression)e.GetChildren().First();
            var To = XType.FromFood(e.Type!);
            var From = XType.FromFood(Child.Type!);

            var Rs = GenerateExpression(method, Child, ObjectAccess.ByValue, null);

            // types dont match
            if (To != From)
            {
                // TODO: vector types
                if (To.Kind == XTypeKind.Fp && From.Kind == XTypeKind.Fp)
                    method.Write(XOp.FloatResize(To, From, Ret, Rs));

                else if (To.Kind == XTypeKind.Fp && (From.Kind == XTypeKind.S
                    || From.Kind == XTypeKind.U))
                    method.Write(XOp.IntToFloat(To, From, Ret, Rs));

                else if ((To.Kind == XTypeKind.S || To.Kind == XTypeKind.U) && From.Kind == XTypeKind.Fp)
                    method.Write(XOp.FloatToInt(To, From, Ret, Rs));

                else
                    method.Write(XOp.Icast(To, From, Ret, Rs));
            }
            break;
        }

        case ExpressionKind.PostfixIncrement:
        {
            var unary_expression = (UnaryExpression)e;

            var R = GenerateExpression(method, unary_expression.Child, ObjectAccess.ByPointer, null);
            var V = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, V, R));
            method.Write(XOp.Copy(XT, Ret, V));
            method.Write(XOp.Increment(XT, V, 1));
            method.Write(XOp.Write(XT, R, V));
            break;
        }

        case ExpressionKind.PostfixDecrement:
        {
            var unary_expression = (UnaryExpression)e;

            var R = GenerateExpression(method, unary_expression.Child, ObjectAccess.ByPointer, null);
            var V = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, V, R));
            method.Write(XOp.Copy(XT, Ret, V));
            method.Write(XOp.Decrement(XT, V, 1));
            method.Write(XOp.Write(XT, R, V));
            break;
        }

        case ExpressionKind.FunctionCall:
        {
            var f_call = (AggregateExpression)e;
            var fx_ref = f_call.Aggregate.First();
            var fx_signature = (FunctionSubjugateSignature)fx_ref.Type!.SubjugateSignature!;
            var r_type = fx_signature.ReturnType;
            var fname = (fx_ref is ChildlessExpression c) ? (string)c.Token.Value! : null;

            var XT_args = new List<XType>();
            foreach (var arg in fx_signature.Params.Arguments)
                XT_args.Add(arg.Ref != Symbol.ArgumentRefType.Value ? XType.Ptr : XType.FromFood(arg.Type!));

            var Rs_args = new List<XReg>();
            foreach (var arg in f_call.Aggregate.Except([fx_ref]))
                Rs_args.Add(GenerateExpression(method, arg, ObjectAccess.ByValue, null));

            if (r_type.Traits.HasFlag(TypeTraits.Void))
            {
                if (fname is not null)
                    method.Write(XOp.PCall(Rs_args.ToArray(), XT_args.ToArray(), fname));
                else
                {
                    var F = GenerateExpression(method, fx_ref, ObjectAccess.ByValue, null);
                    method.Write(XOp.PiCall(new[] { F }.Union(Rs_args).ToArray(), XT_args.ToArray()));
                }
            }
            else
            {
                if (fname is not null)
                    method.Write(XOp.FCall(XT, Ret, Rs_args.ToArray(), XT_args.ToArray(), fname));
                else
                {
                    var F = GenerateExpression(method, fx_ref, ObjectAccess.ByValue, null);
                    method.Write(XOp.FiCall(XT, Ret, new[] { F }.Union(Rs_args).ToArray(), XT_args.ToArray()));
                }
            }
            break;
        }

        case ExpressionKind.ArraySubscript:
        {
            var binary_expression = (BinaryExpression)e;
            var obj_type = ((ArraySubjugateSignature)binary_expression.Left.Type!.SubjugateSignature!).ObjectType;
            var array = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, null);
            var indexer = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);

            method.Write(XOp.MulConst(XType.Ptr, indexer, (nuint)obj_type.SizeOf()));
            method.Write(XOp.Add(XType.Ptr, array, array, indexer));

            // return a pointer instead
            if (access == ObjectAccess.ByPointer
                || obj_type.Traits.HasFlag(TypeTraits.Members)
                || obj_type.Traits.HasFlag(TypeTraits.Array))
            {
                method.Write(XOp.Copy(XType.Ptr, Ret, array));
                break;
            }

            method.Write(XOp.Read(XType.FromFood(obj_type), Ret, array));
            break;
        }


        case ExpressionKind.PrefixIncrement:
        {
            var unary_expression = (UnaryExpression)e;

            var R = GenerateExpression(method, unary_expression.Child, ObjectAccess.ByPointer, null);
            var V = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, V, R));
            method.Write(XOp.Increment(XT, V, 1));
            method.Write(XOp.Copy(XT, Ret, V));
            method.Write(XOp.Write(XT, R, V));
            break;
        }

        case ExpressionKind.PrefixDecrement:
        {
            var unary_expression = (UnaryExpression)e;

            var R = GenerateExpression(method, unary_expression.Child, ObjectAccess.ByPointer, null);
            var V = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, V, R));
            method.Write(XOp.Decrement(XT, V, 1));
            method.Write(XOp.Copy(XT, Ret, V));
            method.Write(XOp.Write(XT, R, V));
            break;
        }

        case ExpressionKind.Multiply:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Mul(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Divide:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Div(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Modulo:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Rem(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Add:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Add(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Subtract:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Sub(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.LeftShift:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XT.Kind == XTypeKind.S ? XOp.Arthm_Lsh(XT, Ret, Ret, Right) : XOp.Bit_Lsh(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.RightShift:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XT.Kind == XTypeKind.S ? XOp.Arthm_Rsh(XT, Ret, Ret, Right) : XOp.Bit_Rsh(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.BitwiseAnd:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Bit_And(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.BitwiseXOr:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Bit_Xor(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.BitwiseOr:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Bit_Or(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Equal:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Equal(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.NotEqual:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.NotEqual(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Lower:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Lower(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.LowerEqual:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.LowerEqual(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.Greater:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Greater(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.GreaterEqual:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.GreaterEqual(XT, Ret, Ret, Right));
            break;
        }

        case ExpressionKind.LogicalAnd:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Leave = XOp.Ifz(Ret, 0);
            method.Write(Leave);
            _ = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, Ret);
            Leave.Kx[0] = (nuint)method.CurCodePosition();

            // current state of Ret
            // if left was false, Ret will be 0
            // otherwise, result of right
            break;
        }

        case ExpressionKind.LogicalOr:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByValue, Ret);
            var Leave = XOp.If(Ret, 0);
            method.Write(Leave);
            _ = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, Ret);
            Leave.Kx[0] = (nuint)method.CurCodePosition();

            // current state of Ret
            // if left was true, Ret will be true
            // otherwise, result of right
            break;
        }

        case ExpressionKind.Conditional:
        {
            var ternary_expression = (TernaryExpression)e;
            _ = GenerateExpression(method, ternary_expression.First, ObjectAccess.ByValue, Ret);
            var False = XOp.Ifz(Ret, 0);
            method.Write(False);

            _ = GenerateExpression(method, ternary_expression.Second, access, Ret);
            var Leave = XOp.Jmp(0);

            method.Write(Leave);
            False.Kx[0] = (nuint)method.CurCodePosition();

            _ = GenerateExpression(method, ternary_expression.Third, access, Ret);

            Leave.Kx[0] = (nuint)method.CurCodePosition();
            break;
        }

        case ExpressionKind.Assign:
        case ExpressionKind.VariableInit:
        {
            var binary_expression = (BinaryExpression)e;
            var destination = binary_expression.Left;
            var source = binary_expression.Right;

            if (FT.Traits.HasFlag(TypeTraits.Array) || FT.Traits.HasFlag(TypeTraits.Members))
            {
                _ = GenerateExpression(method, destination, ObjectAccess.ByPointer, Ret);
                var Rs = GenerateExpression(method, source, ObjectAccess.ByPointer, null);

                method.Write(XOp.Blkcopy(Ret, Rs, (nuint)e.Type!.SizeOf()));
            }
            else
            {
                _ = GenerateExpression(method, destination, ObjectAccess.ByPointer, Ret);
                var Rs = GenerateExpression(method, source, ObjectAccess.ByValue, null);

                method.Write(XOp.Write(XT, Ret, Rs));
            }
            break;
        }

        case ExpressionKind.AssignSum:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Add(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignDiff:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Sub(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignProduct:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Mul(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignQuotient:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Div(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignRemainder:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Rem(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignBitwiseAnd:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Bit_And(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignBitwiseXOr:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Bit_Xor(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignBitwiseOr:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XOp.Bit_Or(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignLeftShift:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);

            method.Write(XT.Kind == XTypeKind.S ? XOp.Arthm_Lsh(XT, AccValue, AccValue, Right) : XOp.Bit_Lsh(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
            break;
        }

        case ExpressionKind.AssignRightShift:
        {
            var binary_expression = (BinaryExpression)e;
            _ = GenerateExpression(method, binary_expression.Left, ObjectAccess.ByPointer, Ret);

            var AccValue = method.AllocateRegister(XRegStorage.Fast);
            method.Write(XOp.Read(XT, AccValue, Ret));

            var Right = GenerateExpression(method, binary_expression.Right, ObjectAccess.ByValue, null);
            method.Write(XT.Kind == XTypeKind.S ? XOp.Arthm_Rsh(XT, AccValue, AccValue, Right) : XOp.Bit_Rsh(XT, AccValue, AccValue, Right));
            method.Write(XOp.Write(XT, Ret, AccValue));
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
         * [X] block
         * [X] labels
         * [.] finally
         * [ ] asm
         * [X] var init / expression
         * [!] switch .. case
         * [!] switch .. default
         * [ ] switch
         * [X] if
         * [X] while
         * [X] do .. while
         * [X] for
         * [!] break
         * [!] continue
         * [X] goto
         * [X] return
         * [!] assert :: for now it has no message and simply calls  __ck_impl_assert_abort()
         * [X] sponge
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

        case StatementKind.Asm:
            method.AsmBlocks.Add(statement.Id, (InlineAsmBlock)statement.Objects[0]);
            method.Write(XOp.InlineAsm(statement.Id));
            return;

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
            //
            // Blocks were previously allocated here, but now we don't, as blocks are related
            // to allocations rather than objects.
        }

        // generate the code!
        GenerateStatement(method, tree.Body);
        method.MakeLabel(method.CurCodePosition(), "$leave");
        method.WriteFinallyBlock();
        method.Write(XOp.Leave());
        return method;
    }
}
