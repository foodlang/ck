using ck.Diagnostics;
using ck.Lang.Type;
using ck.Lang.Tree.Expressions;
using ck.Syntax;
using ck.Util;
using ck.Lang.Tree.Scopes;
using System.Diagnostics;

namespace ck.Lowering;

public sealed partial class Analyzer
{
    // Analyzer for expressions
    
    /// <returns>Returns a cast expression that supersets <paramref name="b"/>.</returns>
    public static Expression ImplicitConversionAvailableTU(FType T, Expression b)
    {
        var parent = b.Parent;
        Expression CreateBCast(FType nT) => new UnaryExpression(b.Token, ExpressionKind.Cast, b)
        {
            LValue = b.LValue,
            Type = nT,
            Parent = parent,
        };

        var U = b.Type!;
        var u_integer_or_float
            = (U.Traits.HasFlag(TypeTraits.Floating) || U.Traits.HasFlag(TypeTraits.Integer)) && !U.Traits.HasFlag(TypeTraits.Pointer);

        if (T == U)
            return b;

        if (T == FType.VOID)
            return CreateBCast(FType.VOID);

        if (T == FType.BOOL)
            return CreateBCast(FType.BOOL);
        else if ((T == FType.F64 && u_integer_or_float) || (T == FType.F32 && u_integer_or_float)
            || ((T.Traits & TypeTraits.Integer) != 0 && (U.Traits & TypeTraits.Integer) != 0))
            return CreateBCast(T);
        else if (T.Traits.HasFlag(TypeTraits.Pointer) && U.Traits.HasFlag(TypeTraits.Pointer))
        {
            var p_U = (PointerSubjugateSignature)U.SubjugateSignature!;

            if (p_U.ObjectType == FType.VOID)
                return CreateBCast(T);
        }
        else if (T.Traits.HasFlag(TypeTraits.Array) && U.Traits.HasFlag(TypeTraits.Array))
        {
            var a_T = (ArraySubjugateSignature)T.SubjugateSignature!;
            var a_U = (ArraySubjugateSignature)U.SubjugateSignature!;

            if (a_T.ObjectType == a_U.ObjectType)
            {
                // Arrays are store linearly, therefore shrinking them is allowed.
                if (U.Size > T.Size)
                    return CreateBCast(T);
            }
        }

        return b;
    }

    /// <summary>
    /// Performs implicit conversion to bring the two types to a common denominator. Will create
    /// cast expressions.
    /// </summary>
    private static void ImplicitConversion(ref Expression a, ref Expression b)
    {
        b = ImplicitConversionAvailableTU(a.Type!, b);
        a = ImplicitConversionAvailableTU(b.Type!, a);
    }

    /// <summary>
    /// Checks function argument for implicit conversion. <paramref name="caller"/> may
    /// be mutated to reflect implicit conversion.
    /// </summary>
    /// <param name="callee">The function's signature.</param>
    /// <param name="caller">The passed arguments. May be mutated.</param>
    /// <returns>False if failure.</returns>
    private static void FunctionArgsImplicit(in FunctionArgumentSignature callee, Expression[] caller)
    {
        for (var i = 0; i < callee.Arguments.Count() && i + 1 < caller.Length; i++)
            caller[i + 1] = ImplicitConversionAvailableTU(callee.Arguments.ElementAt(i).Type, caller[i + 1]);
    }

    /// <summary>
    /// Checks for a type constraint.
    /// </summary>
    /// <param name="e">The expression to check for.</param>
    /// <param name="name">The name of the expression.</param>
    /// <param name="traits">The traits.</param>
    private static bool TConstraint(Expression e, string name, TypeTraits traits)
    {
        if (!e.Type!.Traits.HasFlag(traits))
        {
            Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                $"{name} requires `{traits}` type traits");
            return false;
        }
        return true;
    }

    /// <summary>
    /// Checks for l-value.
    /// </summary>
    /// <param name="e">The expression to check for.</param>
    /// <param name="name">The name of the expression.</param>
    private static bool TLValue(Expression e, string name)
    {
        if (!e.LValue)
        {
            Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                $"{name} must be an l-value");
            return false;
        }
        return true;
    }

    private static Expression CFirst(Expression e) => e.GetChildrenExpressions().ElementAt(0);
    private static Expression CSecond(Expression e) => e.GetChildrenExpressions().ElementAt(1);
    private static Expression CThird(Expression e) => e.GetChildrenExpressions().ElementAt(2);

    /// <summary>
    /// Returns the type that can hold the most information.
    /// </summary>
    /// <param name="t1">First type</param>
    /// <param name="t2">Second type</param>
    private static FType MostInformation(in FType t1, in FType t2)
    {
        // Table of score:
        // (size)
        // void pointer = +1
        // T pointer = +2
        // unsigned = -1
        // float +1

        nint t1_score, t2_score;

        // Pointer checks!!!
        if (t1.Traits.HasFlag(TypeTraits.Array))
            t1_score = 1 + ((ArraySubjugateSignature)t1.SubjugateSignature!).Length;
        else if (t1.Traits.HasFlag(TypeTraits.Pointer))
        {
            var ptr = (PointerSubjugateSignature)t1.SubjugateSignature!;
            t1_score = 1 + (ptr.ObjectType == FType.VOID ? 1 : 0);
        }
        else t1_score = 0;

        if (t2.Traits.HasFlag(TypeTraits.Array))
            t2_score = 1 + ((ArraySubjugateSignature)t2.SubjugateSignature!).Length;
        else if (t2.Traits.HasFlag(TypeTraits.Pointer))
        {
            var ptr = (PointerSubjugateSignature)t2.SubjugateSignature!;
            t2_score = 1 + (ptr.ObjectType == FType.VOID ? 1 : 0);
        }
        else t2_score = 0;

        t1_score += t1.Size
            + (t1.Traits.HasFlag(TypeTraits.Unsigned) ? -1 : 0)
            + (t1.Traits.HasFlag(TypeTraits.Floating) ? 1 : 0)
            ;
        t2_score += t2.Size
            + (t2.Traits.HasFlag(TypeTraits.Unsigned) ? -1 : 0)
            + (t2.Traits.HasFlag(TypeTraits.Floating) ? 1 : 0)
            ;

        return t1_score < t2_score ? t2 : t1;
    }

    public static int AnalyzeExpression(ref Expression e)
    {
        var summed = 0;
        var children = e.GetChildrenExpressions();
        var new_expressions = new Expression[children.Count()];
        var new_expression_index = 0;
        var parent = e.Parent;
        foreach (var child in children)
        {
            new_expressions[new_expression_index] = child;
            summed += AnalyzeExpression(ref new_expressions[new_expression_index++]);
        }
        e.SetChildren(new_expressions);

        // We don't analyze if the children are not analyzed
        if (summed != 0 && e.Kind != ExpressionKind.VariableInit)
            return summed + 1;

        switch (e.Kind)
        {
        case ExpressionKind.Error:
            e.Type = FType.VOID;
            break;

        case ExpressionKind.Literal:
            e.Type = e.Token.ValueType;
            break;

        case ExpressionKind.SizeOf:
            e = new ChildlessExpression(
                new Token(
                    e.Token.Source!, e.Token.Position, TokenKind.NumberLiteral,
                    FType.USZ, (decimal)e.Type!.SizeOf()),
                ExpressionKind.Literal)
            {
                Parent = parent
            };
            e.Type = FType.USZ;
            break;

        case ExpressionKind.LengthOf:
        {
            var what = CFirst(e);
            var expr_ss = what.Type!.SubjugateSignature!;
            if (what.Type! == FType.STRING)
            {
                e = new AggregateExpression(
                    e.Token, ExpressionKind.FunctionCall,
                    [
                        new ChildlessExpression(new Token(e.Token.Position, TokenKind.Identifier, null, "__ck_impl_strlen"), ExpressionKind.Identifier)
                        { Type = FType.Function(FType.USZ, new([new(new Token(e.Token.Position, TokenKind.Identifier, null, "cstr"), FType.STRING, Symbol.ArgumentRefType.Value, new(), null)], null )) },

                        /* as an argument */ what,
                    ])
                { Type = FType.USZ };
            }
            if (expr_ss.LengthOf() != 0)
            {
                e = new ChildlessExpression(new Token(e.Token.Source!, e.Token.Position, TokenKind.NumberLiteral,
                    FType.USZ, (decimal)expr_ss.LengthOf()), ExpressionKind.Literal)
                {
                    Parent = parent,
                };
                break;
            }
            break;
        }

        case ExpressionKind.Default:
        {
            var replacement_candidate = e.Type!.NamedConstants.Where(nc => nc.Name == null);
            if (replacement_candidate.Any())
            {
                var old_parent = e.Parent;
                var candidate = replacement_candidate.First();
                for (; candidate.Aliasing != null; candidate = candidate.Aliasing) ;
                e = candidate.Value!;
                e.Parent = old_parent;
            }
            break;
        }

        case ExpressionKind.NameOf:
        {
            var name = string.Empty;
            if (e.Type is not null)
                name = e.Type.Name;
            else
            {
                var identifier = CFirst(e);
                name = (string)identifier.Token.Value!;
            }
            e = new ChildlessExpression(
                new Token(
                    e.Token.Source!, e.Token.Position, TokenKind.StringLiteral,
                    FType.STRING, name), ExpressionKind.Literal)
            {
                Parent = parent,
            };
            e.Type = FType.STRING;
            break;
        }

        case ExpressionKind.CollectionInitialization:
        {
            var aggregate = (AggregateExpression)e;
            FType? prev = null;
            var casted = new List<Expression>(aggregate.Aggregate.Count());
            foreach (var elem in aggregate.GetChildrenExpressions())
            {
                if (prev is null)
                    prev = elem.Type;
                var elem_implicit = ImplicitConversionAvailableTU(prev!, elem);
                if (prev! != elem.Type!)
                {
                    Diagnostic.Throw(e.Token.Source!, e.Token.Position, DiagnosticSeverity.Error, "",
                        "type mismatch in collection initialization");
                    return 1;
                }
                casted.Add(elem_implicit);
            }
            aggregate.Aggregate = casted;
            e.Type = FType.Array(prev!, aggregate.GetChildrenExpressions().Count());
            break;
        }

        case ExpressionKind.New:
        {
            var aggregate = (AggregateExpression)e;
            if (e.Type!.SubjugateSignature is not StructSubjugateSignature @struct)
            {
                Diagnostic.Throw(e.Token.Source!, e.Token.Position, DiagnosticSeverity.Error, "",
                    "new operator requires struct/union type");
                return 1;
            }
            if (@struct.IsUnion && aggregate.Aggregate.Count() > 1)
            {
                Diagnostic.Throw(e.Token.Source!, e.Token.Position, DiagnosticSeverity.Error, "",
                    "union type cannot have more than one member in its initialization list");
                return 1;
            }
            var member_names = @struct.Members.Select(m => m.Name).ToList();
            var member_types = @struct.Members.Select(m => m.Type);
            var initialized_list = new List<string>(); // members that have been initialized so far
            for (var i = 0; i < aggregate.Aggregate.Count(); i++)
            {
                var expr = aggregate.Aggregate.ElementAt(i);
                var name = aggregate.AggregateNames.ElementAt(i);
                if (initialized_list.Contains(name))
                {
                    Diagnostic.Throw(e.Token.Source!, e.Token.Position, DiagnosticSeverity.Error, "",
                        $"member '{name}' is initialized more than once");
                    return 1;
                }
                if (!member_names.Contains(name))
                {
                    Diagnostic.Throw(e.Token.Source!, e.Token.Position, DiagnosticSeverity.Error, "",
                        $"member '{name}' is not declared in struct/union {e.Type.Name}");
                    return 1;
                }
                initialized_list.Add(name);
                var that_member_type = member_types.ElementAt(member_names.IndexOf(name));
                expr = ImplicitConversionAvailableTU(that_member_type, expr);
                if (expr.Type! != that_member_type)
                {
                    Diagnostic.Throw(e.Token.Source!, e.Token.Position, DiagnosticSeverity.Error, "",
                        $"type mismatch between initializer expression and member '{name}'");
                    return 1;
                }
            }
            break;
        }

        case ExpressionKind.VariableInit:
        {
            var binary = (BinaryExpression)e;
            var nearest_scope = e.FindScopeTree()!;
            var ident = binary.Left.Token;

            // if the symbol is not declared, there's some serious
            // goofy stuff that is going on around here
            if (!nearest_scope.GetDeclaration<Symbol>((string)ident.Value!, ident.Source, out var sym))
                Debug.Fail(null);

            sym!.Referenced = true;
            if (binary.Left.Type is null) //implicit
            {
                sym!.Type = binary.Right.Type!;
                binary.Left.Type = binary.Right.Type!;
                e.Type = FType.VOID;
                break;
            }
            binary.Right = ImplicitConversionAvailableTU(binary.Left.Type!, binary.Right);
            if (binary.Right.Type! != binary.Left.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    "variable couldn't be initialized because of non-matching types");
                return 1;
            }
            e.Type = FType.VOID;
            break;
        }

        case ExpressionKind.Identifier:
        {
            var nearest_scope = e.FindScopeTree();
            var ident = (string)e.Token.Value!;

            if (nearest_scope == null)
                throw new NotImplementedException();

            if (e.Parent is Expression parent_e)
            {
                if (nearest_scope.FindNearestFunction()!.IsLabelDeclared(ident)
                    && parent_e.Kind == ExpressionKind.OpaqueAddressOf)
                {
                    e.LValue = true;
                    e.IsLabelRef = true;
                    // no type
                    break;
                }
                // if the parent is scope resolution, or this is the second
                // operand of the member access operator,
                // the analysis will be done at that level. no need to perform
                // any checks here
                else if (parent_e.Kind == ExpressionKind.ScopeResolution
                      || (parent_e.Kind == ExpressionKind.MemberAccess
                      && CSecond(parent_e) == e))
                    break;
            }

            if (!nearest_scope.GetDeclaration<Symbol>(ident, e.Token.Source!, out var sym))
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    $"identifier '{ident}' is not declared in the current scope");
                return 1;
            }

            if (sym!.Type is null) // wait for type to be figured out first
                return 1;
            e.Type = sym.Type;
            e.LValue = true;
            sym.Referenced = true;

            break;
        }

        case ExpressionKind.MemberAccess:
        {
            var nearest_scope = e.FindNearestParentScope()!;
            var left = CFirst(e);
            var right = CSecond(e);

            var membername = (string)right.Token.Value!;
            if (
                !left.Type!.Traits.HasFlag(TypeTraits.Members)
             && !left.Type!.Traits.HasFlag(TypeTraits.Pointer)
             && !((PointerSubjugateSignature)left.Type!.SubjugateSignature!).ObjectType.Traits.HasFlag(TypeTraits.Members)
              )
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    $"left-hand type must be member or pointer to member");
                return 1;
            }

            var Tmember =
                left.Type!.Traits.HasFlag(TypeTraits.Members)
                ? left.Type!
                : ((PointerSubjugateSignature)left.Type!.SubjugateSignature!).ObjectType;
            var @struct = (StructSubjugateSignature)Tmember.SubjugateSignature!;

            // check for member
            var found = false;
            FType Tresult = FType.VOID;

            for (var i = 0; i < @struct.Members.Length && !found; i++)
            {
                StructMember? member = @struct.Members[i];
                found |= member.Name == membername;
                Tresult = member.Type;
            }

            // :(
            if (!found)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    $"{Tmember.Name} does not have a member named {membername}");
                return 1;
            }

            e.Type = Tresult;
            e.LValue = true;
            break;
        }

        case ExpressionKind.ScopeResolution:
        {
            var nearest_scope = e.FindNearestParentScope()!;
            var left = CFirst(e);
            var right = CSecond(e);

            // TODO: namespaces & modules
            var typename = (string)left.Token.Value!;
            var typename_src = left.Token.Source;
            var constname = (string)right.Token.Value!;

            // enum
            /*if (nearest_scope.GetDeclaration<FEnum>(typename, typename_src, out var @enum))
            {
                // getting enum type
                if (!nearest_scope.GetDeclaration<FClass>(typename, typename_src, out var enumtype))
                    Debug.Fail(null);

                var value = (nint?)0;
                if (constname != "default")
                {
                    @enum!.LookupNamedConstant(constname, out value);
                    if (value is null)
                    {
                        Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                            $"enum '{typename}' does not have a constant named '{constname}'");
                        return 1;
                    }
                }
                e = new ChildlessExpression(new Token(
                    e.Token.Source!, e.Token.Position, TokenKind.NumberLiteral,
                    enumtype!.Class, (decimal)value!), ExpressionKind.Literal)
                {
                    ConstExpr = true,
                    Type = enumtype!.Class,
                    Parent = parent,
                };
                break;
            }
            else*/ if (nearest_scope.GetDeclaration<FClass>(typename, typename_src, out var @class))
            {
                e.Type = @class!.Class;
                if (!e.Type.Traits.HasFlag(TypeTraits.Members) && e.Type.SubjugateSignature is not EnumSubjugateSignature)
                {
                    Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                            $"type {typename} must be an enum, a struct or a union");
                    return 1;
                }

                var nc_name = constname == "default" ? null : constname;
                var const_candidate = e.Type.NamedConstants.Where(nc => nc.Name == nc_name);
                if (!const_candidate.Any())
                {
                    Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                            $"type {typename} does not have the named constant {constname}");
                    return 1;
                }

                var nc = const_candidate.First();
                for (; nc.Aliasing != null; nc = nc.Aliasing) ;
                var old_parent = e.Parent;
                e = nc.Value!;
                e.Parent = old_parent;
                break;
            }

            Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    $"type '{typename}' is not declared in the current scope");
            return 1;
        }

        case ExpressionKind.Cast:
            // parser-level
            break;

        case ExpressionKind.PostfixIncrement:
        case ExpressionKind.PostfixDecrement:
        case ExpressionKind.PrefixIncrement:
        case ExpressionKind.PrefixDecrement:
            if (!TConstraint(CFirst(e), "prefix/postfix increment/decrement operand", TypeTraits.Arithmetic | TypeTraits.Integer)
                && !TLValue(e, "prefix/postfix increment/decrement operand"))
                return 1;
            e.Type = CFirst(e).Type;
            break;

        case ExpressionKind.FunctionCall:
        {
            var callee = CFirst(e);
            if (!TConstraint(callee, "callee", TypeTraits.Function))
                return 1;
            var fx = (FunctionSubjugateSignature)CFirst(e).Type!.SubjugateSignature!;
            var args = e.GetChildrenExpressions().Skip(1).Select(x => x.Type!).ToArray();
            var args_e = e.GetChildrenExpressions().ToArray(); // copy!!!
            var reftypes = ((AggregateExpression)e).RefTypes;
            FunctionArgsImplicit(fx.Params, args_e);

            for (var i = 0; i < reftypes.Count() - 1; i++)
            {
                var reftype = reftypes.ElementAt(i + 1);
                var expected = fx.Params.Arguments.ElementAt(i).Ref;
                if (reftype != fx.Params.Arguments.ElementAt(i).Ref)
                    Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                        $"ref param mismatch for param {i} (expected {expected.ToString().ToLower()}, got {reftype.ToString().ToLower()})");

                var param = args_e[i + 1];
                if (!param.LValue && reftype != Symbol.ArgumentRefType.Value)
                    Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                        "ref param must be lvalue");
            }

            if (fx.Params.Arguments.Count() != args.Count())
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    $"function call signature (n={e.GetChildren().Count() - 1}) does not match with function signature (n={fx.Params.Arguments.Count()})");
            e.Type = fx.ReturnType;
            var aggregate = (AggregateExpression)e;
            aggregate.Aggregate = args_e; // paste!!!
            break;
        }

        case ExpressionKind.ArraySubscript:
        {
            // TODO: pointers, maybe?
            var indexer = CSecond(e);
            var error = 0;
            if (!TConstraint(CFirst(e), "array subscript", TypeTraits.Array))
                error++;
            if (!TConstraint(indexer, "index", TypeTraits.Integer))
                error++;
            if (error != 0) return error;
            var array = (ArraySubjugateSignature)CFirst(e).Type!.SubjugateSignature!;
            if (indexer.Kind == ExpressionKind.Literal)
            {
                var n = (nint)(decimal)indexer.Token.Value!;
                if (array.Length != 0 && n >= array.Length)
                    Diagnostic.Throw(e.Token.Source, e.Token.Position,
                        DiagnosticSeverity.Error, "", $"index {n} is greater than array length {array.Length}");
            }
            e.Type = array.ObjectType;
            e.LValue = true;
            break;
        }

        case ExpressionKind.UnaryPlus:
        case ExpressionKind.UnaryMinus:
            if (!TConstraint(CFirst(e), "unary plus/minus operand", TypeTraits.Arithmetic))
                return 1;
            e.Type = CFirst(e).Type;
            break;

        case ExpressionKind.LogicalNot:
            if (!TConstraint(CFirst(e), "logical not operand", TypeTraits.Integer))
                return 1;
            e.Type = FType.BOOL;
            break;

        case ExpressionKind.BitwiseNot:
            if (!TConstraint(CFirst(e), "bitwise not operand", TypeTraits.Integer | TypeTraits.Arithmetic))
                return 1;
            e.Type = CFirst(e).Type;
            break;

        case ExpressionKind.Dereference:
        {
            if (!TConstraint(CFirst(e), "dereferenced pointer", TypeTraits.Pointer))
                return 1;
            var ptr = (PointerSubjugateSignature)CFirst(e).Type!.SubjugateSignature!;
            e.Type = ptr.ObjectType;
            e.LValue = true;
            break;
        }

        case ExpressionKind.AddressOf:
            if (!TLValue(CFirst(e), "addressed value"))
                return 1;
            e.Type = FType.Pointer(CFirst(e).Type!);
            break;

        case ExpressionKind.OpaqueAddressOf:
            if (!TLValue(CFirst(e), "addressed value"))
                return 1;
            e.Type = FType.VPTR;
            break;

        case ExpressionKind.Multiply:
        case ExpressionKind.Divide:
        case ExpressionKind.Add:
        case ExpressionKind.Subtract:
        {
            var error = 0;
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(left, "arithmetic left operand", TypeTraits.Arithmetic))
                error++;
            if (!TConstraint(right, "arithmetic right operand", TypeTraits.Arithmetic))
                error++;
            if (error != 0) return error;
            ImplicitConversion(ref left, ref right);
            if (left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    "incompatible operand types");
                error++;
            }
            if (error != 0) return error;
            var binary = (BinaryExpression)e;
            binary.Left = left;
            binary.Right = right;
            e.Type = MostInformation(left.Type!, right.Type!);
            break;
        }

        case ExpressionKind.Modulo:
        {
            var error = 0;
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(left, "modulo left operand", TypeTraits.Integer | TypeTraits.Arithmetic))
                error++;
            if (!TConstraint(right, "modulo right operand", TypeTraits.Integer | TypeTraits.Arithmetic))
                error++;
            if (error != 0) return error;
            ImplicitConversion(ref left, ref right);
            if (left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    "incompatible operand types");
                error++;
            }
            if (error != 0) return error;
            var binary = (BinaryExpression)e;
            binary.Left = left;
            binary.Right = right;
            e.Type = MostInformation(left.Type!, right.Type!);
            break;
        }

        case ExpressionKind.LeftShift:
        case ExpressionKind.RightShift:
        case ExpressionKind.BitwiseAnd:
        case ExpressionKind.BitwiseOr:
        case ExpressionKind.BitwiseXOr:
        {
            var error = 0;
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(left, "bitwise first operand", TypeTraits.Integer | TypeTraits.Arithmetic))
                error++;
            if (!TConstraint(right, "bitwise second operand", TypeTraits.Integer | TypeTraits.Arithmetic))
                error++;
            if (error != 0) return error;
            ImplicitConversion(ref left, ref right);
            if (left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    "incompatible operand types");
                error++;
            }
            if (error != 0) return error;
            var binary = (BinaryExpression)e;
            binary.Left = left;
            binary.Right = right;
            e.Type = left.Type!;
            break;
        }

        case ExpressionKind.Lower:
        case ExpressionKind.Greater:
        {
            var error = 0;
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(left, "lower/greater first operand", TypeTraits.Arithmetic))
                error++;
            if (!TConstraint(right, "lower/greater second operand", TypeTraits.Arithmetic))
                error++;
            if (error != 0) return error;
            right = ImplicitConversionAvailableTU(left.Type!, right);
            e.Type = FType.BOOL;
            break;
        }

        case ExpressionKind.LowerEqual:
        case ExpressionKind.GreaterEqual:
        {
            var error = 0;
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(left, "lower/greater left operand", TypeTraits.Arithmetic))
                error++;
            if (!TConstraint(right, "lower/greater right operand", TypeTraits.Arithmetic))
                error++;

            if (left.Type!.Traits.HasFlag(TypeTraits.Floating)
             || right.Type!.Traits.HasFlag(TypeTraits.Floating))
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Info, "floating-point-comparison",
                    "comparing floats with equality-like operators may yield undesired results, as the floats may be imprecise as compared to real world numbers.");

            if (error != 0) return error;
            right = ImplicitConversionAvailableTU(left.Type!, right);
            e.Type = FType.BOOL;
            break;
        }

        case ExpressionKind.MemberOf:
        {
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(right, "member-of array", TypeTraits.Array))
                return 1;
            var array = (ArraySubjugateSignature)right.Type!.SubjugateSignature!;
            left = ImplicitConversionAvailableTU(array.ObjectType!, left);
            if (left.Type! != array.ObjectType!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Error, "", "member-of operator requires first value to be compatible with array object type");
                return 1;
            }
            e.Type = FType.BOOL;
            break;
        }

        case ExpressionKind.AllMembersOf:
        case ExpressionKind.AnyMembersOf:
        {
            {
                var left = CFirst(e);
                var right = CSecond(e);
                if (!TConstraint(right, "member-of array", TypeTraits.Array))
                    return 1;
                var left_array = (ArraySubjugateSignature)left.Type!.SubjugateSignature!;
                var right_array = (ArraySubjugateSignature)right.Type!.SubjugateSignature!;
                if (left.Type! != right.Type!)
                {
                    Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Error, "", "Any/all Member-of operator requires both arrays to have compatible object types");
                    return 1;
                }
                e.Type = FType.BOOL;
                break;
            }
        }

        case ExpressionKind.Equal:
        case ExpressionKind.NotEqual:
        {
            var left = CFirst(e);
            var right = CSecond(e);
            ImplicitConversion(ref left, ref right);
            if (left.Type! == FType.VOID || right.Type! == FType.VOID)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Error, "", "incomplete type in equality operator");
                return 1;
            }

            if (left.Type! == FType.BOOL || right.Type! == FType.BOOL)
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Warning, "bool-comparison",
                    "comparing boolean values is an unsafe and futile operation");


            if ((left.Type!.Traits.HasFlag(TypeTraits.Members) || right.Type!.Traits.HasFlag(TypeTraits.Members))
                && left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Error, "", "cannot compare member objects of different types");
                return 1;
            }

            if (left.Type!.Traits.HasFlag(TypeTraits.Floating)
             || right.Type!.Traits.HasFlag(TypeTraits.Floating))
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Info, "floating-point-comparison",
                    "comparing floats with equality-like operators may yield undesired results, as the floats may be imprecise as compared to real world numbers.");

            e.Type = FType.BOOL;
            break;
        }

        case ExpressionKind.LogicalAnd:
        case ExpressionKind.LogicalOr:
        {
            var error = 0;
            var left = CFirst(e);
            var right = CSecond(e);
            if (!TConstraint(left, "logical operator left operand", TypeTraits.Integer))
                error++;
            if (!TConstraint(right, "logical operator right operand", TypeTraits.Integer))
                error++;
            if (error != 0) return error;
            e.Type = FType.BOOL;
            break;
        }

        case ExpressionKind.Conditional:
        {
            var error = 0;
            var condition = CFirst(e);
            var left = CSecond(e);
            var right = CThird(e);
            if (!TConstraint(condition, "condition", TypeTraits.Integer))
                error++;
            ImplicitConversion(ref left, ref right);
            if (left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                    "conditional operator requires non-condition operands to be of compatible type.");
                error++;
            }
            if (error != 0) return error;
            var ternary = (TernaryExpression)e;
            ternary.Second = left;
            ternary.Third = right;
            e.Type = left.Type!;
            break;
        }

        case ExpressionKind.Assign:
        case ExpressionKind.AssignSum:
        case ExpressionKind.AssignDiff:
        case ExpressionKind.AssignProduct:
        case ExpressionKind.AssignQuotient:
        case ExpressionKind.AssignRemainder:
        case ExpressionKind.AssignBitwiseAnd:
        case ExpressionKind.AssignBitwiseXOr:
        case ExpressionKind.AssignBitwiseOr:
        case ExpressionKind.AssignLeftShift:
        case ExpressionKind.AssignRightShift:
        {
            var left = CFirst(e);
            var right = CSecond(e);
            right = ImplicitConversionAvailableTU(left.Type!, right);

            // If not pure assigment `=`, requires arithmetic
            if (e.Kind != ExpressionKind.Assign
                && !(TConstraint(left, "accumulator", TypeTraits.Arithmetic)
                && TConstraint(right, "source", TypeTraits.Arithmetic)))
                return 1;

            if (left.Type! == FType.VOID || right.Type! == FType.VOID)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Error, "", "incomplete type in assignment");
                return 1;
            }

            if ((left.Type!.Traits.HasFlag(TypeTraits.Members) || right.Type!.Traits.HasFlag(TypeTraits.Members))
                && left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                    DiagnosticSeverity.Error, "",
                    $"cannot assign a value of type {left.Type!.Name} with a value of {right.Type!.Name}");
                return 1;
            }

            if (left.Type! != right.Type!)
            {
                Diagnostic.Throw(e.Token.Source, e.Token.Position,
                   DiagnosticSeverity.Error, "", "invalid types in assignment");
                return 1;
            }

            e.Type = FType.VOID;
            break;
        }

        default:
            Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                $"unsupported expression stumbled upon in analyzer `{e.Kind}`");
            break;
        }

        // const expr reduction;
        if (e.GetChildrenExpressions().Where(e => e.ConstExpr).FullMatch(e.GetChildrenExpressions()))
            e = ReduceConstExpr(e);
        if (e.Parent is null && e.Kind != ExpressionKind.Literal)
            Diagnostic.Throw(e.Token.Source, e.Token.Position, DiagnosticSeverity.Error, "",
                "expected constexpr");
        return 0;
    }
}
