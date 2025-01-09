using ck.Lang.Tree;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using ck.Lang.Type;
using System.Diagnostics;

namespace ck.Ssa;

/// <summary>
/// Generates SSA instructions from a program tree.
/// </summary>
public static class SsaTranslator
{
    private sealed class TempAllocator
    {
        private nint _counter;

        public nint Alloc(SsaType T)
        {
            Temps.Add(new(_counter, T, new(null, null)));
            return _counter++;
        }
        public readonly List<Variable> Temps = new();

        public TempAllocator(nint @base)
        {
            _counter = @base;
        }
    }

    /// <summary>
    /// Builds a program.
    /// </summary>
    public static SsaProgram BuildProgram(FreeTree program)
    {
        var ssa_program = new SsaProgram();
        var all_ast_functions =
               program.Where(n => n is FunctionNode).Cast<FunctionNode>()
        /*referenced*/.Where(f => f.FuncSym.InternalTo == null || f.FuncSym.Referenced);
        foreach (var fx in all_ast_functions)
            ssa_program.Functions[FuncName(fx)] = BuildFunction(fx);
        return ssa_program;
    }

    /// <summary>
    /// Builds a function in SSA form.
    /// </summary>
    /// <param name="ast_func">AST node of the function.</param>
    public static SsaFunction BuildFunction(FunctionNode ast_func)
    {
        var target = new SsaFunction(ast_func); // target SSA function
        var next_vreg = (nint)0;                // next virtual register id

        // creating the auto variable table
        var func_type = (FunctionSubjugateSignature)ast_func.FuncSym.Type!.SubjugateSignature!;
        var enumerated_autos = ast_func.GetAllUsedSymbols();
        foreach (var auto in enumerated_autos)
        {
            var n = AutoName(auto);
            if (auto.RefType != Symbol.ArgumentRefType.Value)
            {
                target.RefParamList.Add(n);
                target.Auto.Add(n, new Variable(next_vreg++, SsaType.Ptr, new(null, null)));
                continue;
            }

            target.Auto.Add(n, new Variable(next_vreg++, TFoodToSsa(auto.Type!), new(null, null)));
        }

        // emitting
        var temp_allocator = new TempAllocator(next_vreg);
        var return_type = func_type.ReturnType;

        // block body
        if (ast_func.Body!.Kind == StatementKind.Block)
        {
            EmitStmt(target, temp_allocator, ast_func.Body!, null, null);

            // end of function
            _return(target, TFoodToSsa(return_type), -1);

            // emit finally
            if (ast_func.HasFinally)
            {
                var @finally = ast_func.AllStatements().Where(s => s.Kind == StatementKind.Finally).Single();
                target.Finally = _marker(target);
                EmitStmt(target, temp_allocator, @finally.Statements[0], null, null);
            }
        }
        // expr body
        else
        {
            EmitExpression(target, temp_allocator, ast_func.Body!.Expressions[0], out var Rresult, out _, out _);
            _return(target, TFoodToSsa(return_type), Rresult);
        }        

        // generating live ranges
        target.AllVariables.AddRange(target.Auto.Values);
        target.AllVariables.AddRange(temp_allocator.Temps);
        for (var i = 0; i < target.AllVariables.Count; i++)
        {
            var v = target.AllVariables[i];

            // do not check source operands of possible instructions that bear constant values
            // (which are stored in the same field as the source registers)

            for (
                var current_instruction = target.Base;
                current_instruction != null;
                current_instruction = current_instruction.Next
                )
            {
                if (v.Name != current_instruction.DestinationRegister
                 || current_instruction.Opcode == SsaOpcode.Const
                 || (current_instruction.Opcode.ElementOf(SsaOpcode.AddC, SsaOpcode.SubC, SsaOpcode.MulC, SsaOpcode.DivC)
                  && current_instruction.Source0 != v.Name)
                 )
                    continue;

                v.Live = new(v.Live.Start ?? current_instruction, current_instruction);
            }

            if (v.Live.Start is null || v.Live.End is null)
            {
                v.Live = new(target.Base, target.Base);
                v.SpillCost = int.MinValue;
            }

            // TODO: more sophisticated heuristics
            if (v.Live.Start is not null && v.Live.End is not null)
                v.SpillCost = -(v.Live.End - v.Live.Start);
        }

        // param table
        target.Params = ((FunctionSubjugateSignature)ast_func.FuncSym.Type.SubjugateSignature!).Params.Arguments
                .Select(p => p.GetSymbol(ast_func).Name).Select(name => target.Auto[name + ast_func.Id.ToString()]);

        return target;
    }

    private static void EmitInfiniteLoop(SsaFunction target, TempAllocator temp_allocator, Statement body)
    {
        var clbreak = target.MakeUnnamedLabelName();
        var clcontinue = target.MakeUnnamedLabelName();
        target.Labels.Add(clcontinue, _marker(target));
        EmitStmt(target, temp_allocator, body, clbreak, clcontinue);
        _branch(target, clcontinue);
        target.Labels.Add(clbreak, _marker(target));
        return;
    }

    /// <summary>
    /// Emits a statement.
    /// </summary>
    /// <param name="target">The target function.</param>
    /// <param name="stmt">The statement to generate.</param>
    private static void EmitStmt(SsaFunction target, TempAllocator temp_allocator, Statement stmt, string? lbreak, string? lcontinue)
    {
        // dont mind the evil goto
    LGenNewStmt:
        switch (stmt.Kind)
        {
        case StatementKind.Error:
            Debug.Fail(null);
            return; /* dead code */

        case StatementKind.Empty:
            return;

        case StatementKind.Expression:
        case StatementKind.VariableInit:
            EmitExpression(target, temp_allocator, stmt.Expressions[0], out _, out _, out _);
            return;

        case StatementKind.Block:
            foreach (var cstmt in stmt.Statements)
                EmitStmt(target, temp_allocator, cstmt, lbreak, lcontinue);
            return;

        case StatementKind.Sponge:
            stmt = stmt.Statements[0];
            goto LGenNewStmt;

        case StatementKind.If:
        {
            var condition = stmt.Expressions[0];
            if (condition.Kind == ExpressionKind.Literal)
            {
                // true
                if ((decimal)condition.Token.Value! != 1)
                {
                    stmt = stmt.Statements[0];
                    goto LGenNewStmt;
                }
                // false with else stmt
                else if (stmt.Statements.Count == 2)
                {
                    stmt = stmt.Statements[1];
                    goto LGenNewStmt;
                }
                // false without else
                // -> generate no code
                return;
            }

            // normal if generation
            var then_label = target.MakeUnnamedLabelName();
            var end_label = target.MakeUnnamedLabelName();
            EmitExpression(target, temp_allocator, stmt.Expressions[0], out var Rcond, out var Tcond, out _);
            _conditional_branch(target, Tcond, Rcond, then_label);

            // else stmt
            if (stmt.Statements.Count != 1)
                EmitStmt(target, temp_allocator, stmt.Statements[1], lbreak, lcontinue);
            _branch(target, end_label); // jump to end

            // then stmt
            target.Labels.Add(then_label, _marker(target));
            EmitStmt(target, temp_allocator, stmt.Statements[0], lbreak, lcontinue);

            // end marker
            target.Labels.Add(end_label, _marker(target));
            return;
        }

        case StatementKind.While:
        {
            var condition = stmt.Expressions[0];
            if (condition.Kind == ExpressionKind.Literal)
            {
                // true, dead code if false
                if ((decimal)condition.Token.Value! != 1)
                    EmitInfiniteLoop(target, temp_allocator, stmt.Statements[0]);
                return;
            }

            var clbreak = target.MakeUnnamedLabelName();
            var clcontinue = target.MakeUnnamedLabelName();
            target.Labels.Add(clcontinue, _marker(target));
            EmitExpression(target, temp_allocator, condition, out var Rcond, out var Tcond, out _);
            _zconditional_branch(target, Tcond, Rcond, clbreak);
            EmitStmt(target, temp_allocator, stmt.Statements[0], clbreak, clcontinue);
            _branch(target, clcontinue);
            target.Labels.Add(clbreak, _marker(target));
            return;
        }

        case StatementKind.DoWhile:
        {
            var condition = stmt.Expressions[0];
            if (condition.Kind == ExpressionKind.Literal)
            {
                // true, dead code if false
                if ((decimal)condition.Token.Value! != 1)
                    EmitInfiniteLoop(target, temp_allocator, stmt.Statements[0]);
                return;
            }

            var clbreak = target.MakeUnnamedLabelName();
            var clcontinue = target.MakeUnnamedLabelName();
            var top = target.MakeUnnamedLabelName();
            target.Labels.Add(top, _marker(target));
            EmitStmt(target, temp_allocator, stmt.Statements[0], clbreak, clcontinue);
            target.Labels.Add(clcontinue, _marker(target));
            EmitExpression(target, temp_allocator, condition, out var Rcond, out var Tcond, out _);
            _conditional_branch(target, Tcond, Rcond, top);
            target.Labels.Add(clbreak, _marker(target));
            return;
        }

        case StatementKind.Break:
            _branch(target, lbreak!);
            return;

        case StatementKind.Continue:
            _branch(target, lcontinue!);
            return;

        case StatementKind.Return:
            if (stmt.Expressions.Count == 0)
            {
                _return(target, SsaType.None, -1);
                return;
            }

            EmitExpression(target, temp_allocator, stmt.Expressions[0], out var Rret, out var Tret, out _);
            _return(target, Tret, Rret);
            return;

        case StatementKind.Label:
        {
            var fullname = $"._{(string)stmt.Objects[0]}";
            target.Labels.Add(fullname, _marker(target));
            return;
        }

        case StatementKind.Goto:
            // direct
            if (stmt.Objects.Count == 1)
            {
                var fullname = $"._{(string)stmt.Objects[0]}";
                _branch(target, fullname);
            }
            else // indirect
            {
                EmitExpression(target, temp_allocator, stmt.Expressions[0], out var Raddr, out _, out _);
                _computed_branch(target, Raddr);
            }
            return;

        case StatementKind.SwitchCase:
        {
            var that_switch = stmt.FindNearestSwitch()!;
            var name = $".case{that_switch.Id}_{stmt.Id}";
            target.Labels.Add(name, _marker(target));
            return;
        }

        case StatementKind.SwitchDefault:
        {
            var that_switch = stmt.FindNearestSwitch()!;
            var name = $".default{that_switch.Id}";
            target.Labels.Add(name, _marker(target));
            return;
        }

        case StatementKind.For:
        {
            // note:
            // for loops are generated as if infinite loop didn't have
            // a special function, because of how complex the syntax object
            // is

            var clcontinue = target.MakeUnnamedLabelName();
            var clbreak = target.MakeUnnamedLabelName();
            var lcond = target.MakeUnnamedLabelName();

            var init = stmt.Statements.Count == 2 ? stmt.Statements[0] : null;
            var cond = stmt.Expressions.Count >= 1 ? stmt.Expressions[0] : null;
            var iterator = stmt.Expressions.Count == 2 ? stmt.Expressions[1] : stmt.Expressions[0];
            var body = stmt.Statements.Count == 2 ? stmt.Statements[1] : stmt.Statements[0];

            if (init != null)
                EmitStmt(target, temp_allocator, init, lbreak, lcontinue);

            // cond
            target.Labels.Add(lcond, _marker(target));
            if (cond != null)
            {
                EmitExpression(target, temp_allocator, cond, out var Rcond, out var Tcond, out _);
                _zconditional_branch(target, Tcond, Rcond, clbreak);
            }

            // body
            EmitStmt(target, temp_allocator, body, clbreak, clcontinue);

            // continue
            target.Labels.Add(clcontinue, _marker(target));
            if (iterator != null)
                EmitExpression(target, temp_allocator, iterator, out _, out _, out _);
            _branch(target, clcontinue);

            // leave
            target.Labels.Add(clbreak, _marker(target));
            return;
        }

        case StatementKind.Switch:
        {
            var evaluated = stmt.Expressions[0];
            var body = stmt.Statements[0];

            var clbreak = target.MakeUnnamedLabelName();
            var table = target.MakeUnnamedLabelName();
            var cases = stmt.AllStatementsExcept(StatementKind.Switch).Where(s => s.Kind == StatementKind.SwitchCase);
            var @default = stmt.AllStatementsExcept(StatementKind.Switch).Where(s => s.Kind == StatementKind.SwitchDefault).SingleOrDefault();

            // jump to table
            _branch(target, table);

            // generate body first
            EmitStmt(target, temp_allocator, body, clbreak, lcontinue);

            // table
            // TODO: add support for true switch (for switches whos integer ranges are small
            //       enough for the L1 cache)
            target.Labels.Add(table, _marker(target));
            EmitExpression(target, temp_allocator, evaluated, out var Reval, out var Teval, out _);
            foreach (var @case in cases)
            {
                var key = Constant(TFoodToSsa(@case.Expressions[0].Type!), (decimal)@case.Expressions[0].Token.Value!);
                _equalc(target, Teval, Reval, Reval, key);
                _conditional_branch(target, Teval, Reval, $".case{stmt.Id}_{@case.Id}");
            }
            if (@default != null)
                _branch(target, $".default{stmt.Id}");

            // break
            target.Labels.Add(clbreak, _marker(target));
            return;
        }

        case StatementKind.Finally:
            // no code here
            return;

        case StatementKind.Assert:
            _nop(target);
            return;

        }
    }

    // turns floats into integers using bits
    private static nint Constant(SsaType T, decimal d)
    {
        switch (T)
        {
        case SsaType.B8:
        case SsaType.B16:
        case SsaType.B32:
        case SsaType.B64:
        case SsaType.Ptr:
        case SsaType.None:
            return (nint)d;

        case SsaType.F32: return BitConverter.SingleToInt32Bits((float)d);
        case SsaType.F64: return (nint)BitConverter.DoubleToInt64Bits((double)d);
        }

        throw new NotImplementedException();
    }

    /// <summary>
    /// Emits an expression.
    /// </summary>
    /// <param name="target">The target function.</param>
    /// <param name="next_vreg">reference to next_vreg</param>
    /// <param name="e">The expression to generate.</param>
    /// <returns>The resulting register. <c>-1</c> if no resulting value.</returns>
    private static void EmitExpression(SsaFunction target, TempAllocator temp_allocator, Expression e, out nint Rout, out SsaType T, out bool addressable, bool addr_required = false)
    {
        Expression CFirst(Expression e) => e.GetChildrenExpressions().ElementAt(0);
        Expression CSecond(Expression e) => e.GetChildrenExpressions().ElementAt(1);
        Expression CThird(Expression e) => e.GetChildrenExpressions().ElementAt(2);

        nint create_operand_register(TempAllocator temp_allocator, SsaType T, bool addressable, nint @default)
        {
            if (addressable)
                return temp_allocator.Alloc(T);
            return @default;
        }

        FType Tfood;

    LGenNewExpr:
        addressable = false;

        Tfood = e.Type!;
        T = TFoodToSsa(Tfood);

        switch (e.Kind)
        {
        default:
            Debug.Fail(null);
            /* dead code */
            Rout = -1;
            T = SsaType.None;
            addressable = false;
            return;

        case ExpressionKind.Literal:
            Rout = temp_allocator.Alloc(T);
            if (Tfood == FType.STRING && e.Token.Kind == Syntax.TokenKind.StringLiteral)
                _stringliteraladdr(target, Rout, target.NewStringLiteral((string)e.Token.Value!));
            else
                _const(target, T, Rout, Constant(T, (decimal)e.Token.Value!));
            addressable = false;
            return;

        case ExpressionKind.CollectionInitialization:
        {
            // creating block
            var block = target.NewAutoBlock(e.Type!.SizeOf());

            // getting reference to block
            Rout = temp_allocator.Alloc(SsaType.Ptr);
            _autoblockaddr(target, Rout, block.Offset);

            // setting array
            var Tobj = TFoodToSsa(((ArraySubjugateSignature)e.Type!.SubjugateSignature!).ObjectType);
            var elems = e.GetChildrenExpressions();
            var Toffset = temp_allocator.Alloc(SsaType.Ptr);
            _copy(target, SsaType.Ptr, Toffset, Rout);
            foreach (var elem in elems)
            {
                var sz = elem.Type!.SizeOf();
                EmitExpression(target, temp_allocator, elem, out var Relem, out _, out _);
                _addc(target, SsaType.Ptr, Toffset, sz);
                if (elem.Type!.Traits.HasFlag(TypeTraits.Members))
                    _memcopy(target, Toffset, Relem, sz);
                else
                    _store(target, Tobj, Toffset, Relem);
            }

            addressable = false;
            return;
        }

        case ExpressionKind.Default:
        {
            // creating block
            var block = target.NewAutoBlock(e.Type!.SizeOf());

            // getting reference to block
            Rout = temp_allocator.Alloc(SsaType.Ptr);
            _autoblockaddr(target, Rout, block.Offset);
            _zeroblock(target, Rout, block.Size);
            addressable = false;
            return;
        }

        case ExpressionKind.New:
        {
            // creating block
            var block = target.NewAutoBlock(e.Type!.SizeOf());

            // getting reference to block
            Rout = temp_allocator.Alloc(SsaType.Ptr);
            _autoblockaddr(target, Rout, block.Offset);
            _zeroblock(target, Rout, block.Size);

            // apply initializer list
            var initializer_list = (AggregateExpression)e;
            var initializer_list_length = initializer_list.Aggregate.Count();
            var @struct = (StructSubjugateSignature)e.Type!.SubjugateSignature!;
            var members = @struct.Members;
            var Toffset = temp_allocator.Alloc(SsaType.Ptr);
            _copy(target, SsaType.Ptr, Toffset, Rout);

            // unions only have one member so this works fine
            for (var i = 0; i < members.Length; i++)
            {
                var member = members[i];
                for (var j = 0; j < initializer_list_length; j++)
                {
                    var initname = initializer_list.AggregateNames.ElementAt(j);
                    if (initname == member.Name)
                    {
                        var init = initializer_list.Aggregate.ElementAt(j);
                        EmitExpression(target, temp_allocator, init, out var Relem, out _, out _);
                        if (init.Type!.Traits.HasFlag(TypeTraits.Members))
                            _memcopy(target, Toffset, Relem, init.Type!.SizeOf());
                        else
                            _store(target, TFoodToSsa(init.Type!), Toffset, Relem);
                        break;
                    }
                }

                if (i + 1 < members.Length)
                    _addc(target, SsaType.Ptr, Toffset, member.Type!.SizeOf());
            }
            addressable = false;
            return;
        }

        case ExpressionKind.Identifier:
        {
            var autoname = AutoName(e);

            // ref param
            if (target.RefParamList.Contains(autoname))
            {
                if (!addr_required)
                {
                    Rout = temp_allocator.Alloc(T);
                    _deref(target, T, Rout, target.Auto[autoname].Name);
                    addressable = false;
                    return;
                }
                /* fallthrough */
            }

            Rout = target.Auto[AutoName(e)].Name;
            if (e.Parent is Expression parent
                && (parent.Kind == ExpressionKind.AddressOf
                || parent.Kind == ExpressionKind.OpaqueAddressOf))
                target.Auto[AutoName(e)].LocalStorage = true;
            addressable = true;
            return;
        }

        case ExpressionKind.Cast:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var Rin, out var Tin, out var caddr);
            if (Tin != T)
            {
                Rout = create_operand_register(temp_allocator, T, caddr, Rin);
                if (Tfood.Traits.HasFlag(TypeTraits.Unsigned))
                    _cast(target, Tin, T, Rout, Rin);
                else _cast_signed(target, Tin, T, Rout, Rin);
            }
            Rout = Rin;
            return;
        }

        case ExpressionKind.PostfixIncrement:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var Rin, out _, out var caddr);
            Rout = create_operand_register(temp_allocator, T, caddr, Rin);
            _copy(target, T, Rout, Rin);
            _inc(target, T, Rin);
            // Rin cannot be freed, as it is a lvalue
            return;
        }

        case ExpressionKind.PostfixDecrement:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var Rin, out _, out var caddr);
            Rout = create_operand_register(temp_allocator, T, caddr, Rin);
            _copy(target, T, Rout, Rin);
            _dec(target, T, Rin);
            // Rin cannot be freed as it is a lvalue
            return;
        }

        case ExpressionKind.FunctionCall:
        {
            var callee = CFirst(e);

            var args = e.GetChildrenExpressions().Skip(1);
            var arg_reftypes = ((AggregateExpression)e).RefTypes;
            var argc = args.Count();
            var aregs = new (nint, SsaType)[argc];

            for (int i = 0; i < argc; i++)
            {
                var c_addr_required = arg_reftypes.ElementAt(i + 1) != Symbol.ArgumentRefType.Value;
                EmitExpression(
                    target, temp_allocator,
                    args.ElementAt(i),
                    out aregs[i].Item1,
                    out aregs[i].Item2,
                    out _,
                    c_addr_required
                    );
                if (c_addr_required)
                    aregs[i].Item2 = SsaType.Ptr;
            }

            // indirect call
            if (callee.Kind != ExpressionKind.Identifier)
            {
                EmitExpression(target, temp_allocator, callee, out var Rcallee, out _, out var caddr);
                // return value
                if (T != SsaType.None)
                {
                    Rout = create_operand_register(temp_allocator, T, caddr, Rcallee);
                    _computed_call(target, T, Rout, Rcallee, aregs);
                    return;
                }
                Rout = -1;
                _computed_call(target, T, -1, Rcallee, aregs);
                return;
            }

            // return value
            if (T != SsaType.None)
            {
                Rout = temp_allocator.Alloc(T);
                _call(target, T, Rout, FuncName((string)callee.Token.Value!), aregs);
                return;
            }
            _call(target, T, -1, FuncName((string)callee.Token.Value!), aregs);
            Rout = -1;
            return;
        }

        case ExpressionKind.MemberAccess:
        {
            var src = CFirst(e);
            var required = CSecond(e);

            var maybe_struct = src.Type!.Traits.HasFlag(TypeTraits.Pointer)
                // pointer to struct
                ? ((StructSubjugateSignature)((PointerSubjugateSignature)src.Type!.SubjugateSignature!)
                    .ObjectType.SubjugateSignature!)
                // struct
                : (StructSubjugateSignature)src.Type!.SubjugateSignature!;

            var members = maybe_struct.Members;
            EmitExpression(target, temp_allocator, src, out var Rsrc, out _, out _, true /* if member access */);

            var computed_member_offset = (nint)0;

            // structs
            if (!maybe_struct.IsUnion)
            {
                for (var i = 0; i < members.Length; i++)
                {
                    var member = members[i];
                    if (member.Name == (string)required.Token.Value!)
                        break;

                    // otherwise, add size and continue
                    computed_member_offset += member.Type!.SizeOf();
                }
            }

            var Roffset = temp_allocator.Alloc(SsaType.Ptr);
            _copy(target, SsaType.Ptr, Roffset, Rsrc);
            if (computed_member_offset != 0)
                _addc(target, SsaType.Ptr, Roffset, computed_member_offset);
            
            Rout = Roffset;
            if (!addr_required)
            {
                _deref(target, T, Rout, Rout);
                return;
            }
            
            return;
        }

        case ExpressionKind.ArraySubscript:
        {
            var array = CFirst(e);
            var index = CSecond(e);

            EmitExpression(target, temp_allocator, array, out var Rarray_in, out _, out var caddr_array);
            EmitExpression(target, temp_allocator, index, out var Rindex_in, out var Tindex, out var caddr_index);

            // scale index (if necessary)
            var Rarray = create_operand_register(temp_allocator, T, caddr_array, Rarray_in);
            var Rindex = create_operand_register(temp_allocator, T, caddr_index, Rindex_in);
            if (((ArraySubjugateSignature)array.Type!.SubjugateSignature!).ObjectType.SizeOf() > 1)
                _mulc(target, Tindex, Rindex, index.Type!.SizeOf(), false);

            // add pointer and index
            _cast(target, Tindex, SsaType.Ptr, Rindex, Rindex);
            _add(target, SsaType.Ptr, Rarray, Rarray, Rindex);

            // we need address!
            if (addr_required)
            {
                Rout = temp_allocator.Alloc(SsaType.Ptr);
                _copy(target, T, Rout, Rarray);
                return;
            }

            // dereference (value)
            Rout = create_operand_register(temp_allocator, T, caddr_array, Rarray);
            _deref(target, T, Rout, Rarray);
            return;
        }

        case ExpressionKind.UnaryPlus:
            e = CFirst(e);
            goto LGenNewExpr;

        case ExpressionKind.UnaryMinus:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R, out _, out var caddr);
            Rout = create_operand_register(temp_allocator, T, caddr, R);
            _negate(target, T, Rout, R);
            return;
        }

        case ExpressionKind.LogicalNot:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R, out _, out var caddr);
            Rout = create_operand_register(temp_allocator, T, caddr, R);
            _lnot(target, T, Rout, R);
            return;
        }

        case ExpressionKind.BitwiseNot:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R, out _, out var caddr);
            Rout = create_operand_register(temp_allocator, T, caddr, R);
            _not(target, T, Rout, R);
            return;
        }

        case ExpressionKind.Dereference:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var Rptr, out _, out var caddr);
            if (addr_required)
            {
                Rout = create_operand_register(temp_allocator, SsaType.Ptr, caddr, Rptr);
                _copy(target, T, Rout, Rptr);
                return;
            }
            Rout = create_operand_register(temp_allocator, T, caddr, Rptr);
            _deref(target, T, Rout, Rptr);
            return;
        }

        case ExpressionKind.AddressOf:
        case ExpressionKind.OpaqueAddressOf:
        {
            // may be label
            if (CFirst(e).Kind == ExpressionKind.Identifier)
            {
                var labelname = (string)CFirst(e).Token.Value!;
                if (target.Source.IsLabelDeclared(labelname)) // label!!!
                {
                    Rout = temp_allocator.Alloc(T);
                    _labelref(target, SsaType.Ptr, Rout, $"._{labelname}");
                    return;
                }
            }

            // not a label
            EmitExpression(target, temp_allocator, CFirst(e), out var Rlvalue, out _, out var caddr);
            Rout = create_operand_register(temp_allocator, SsaType.Ptr, caddr, Rlvalue);
            _address(target, Rout, Rlvalue);
            return;
        }

        case ExpressionKind.Multiply:
        {
            // TODO: mulc optimization
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            if (Tfood.Traits.HasFlag(TypeTraits.Unsigned))
                _mul(target, T, Rout, R0, R1);
            else _sign_mul(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.Divide:
        {
            // TODO: divc optimization
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            if (Tfood.Traits.HasFlag(TypeTraits.Unsigned))
                _div(target, T, Rout, R0, R1);
            else _sign_div(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.Modulo:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            if (Tfood.Traits.HasFlag(TypeTraits.Unsigned))
                _mod(target, T, Rout, R0, R1);
            else _sign_mod(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.Add:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _add(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.Subtract:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _sub(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.LeftShift:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _lsh(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.RightShift:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _rsh(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.Lower:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _lower(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.LowerEqual:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _lower_equal(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.Greater:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _greater(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.GreaterEqual:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _greater_equal(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.MemberOf:
        case ExpressionKind.AllMembersOf:
        case ExpressionKind.AnyMembersOf:
            // TODO
            Rout = temp_allocator.Alloc(T);
            addressable = false;
            return;

        case ExpressionKind.Equal:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _equal(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.NotEqual:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _not_equal(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.BitwiseAnd:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _and(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.BitwiseXOr:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _xor(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.BitwiseOr:
        {
            EmitExpression(target, temp_allocator, CFirst(e), out var R0, out _, out var caddr);
            EmitExpression(target, temp_allocator, CSecond(e), out var R1, out _, out _);
            Rout = create_operand_register(temp_allocator, T, caddr, R0);
            _or(target, T, Rout, R0, R1);
            return;
        }

        case ExpressionKind.LogicalAnd:
        {
            var zero_branch = target.MakeUnnamedLabelName();
            var end_branch = target.MakeUnnamedLabelName();

            // first
            EmitExpression(target, temp_allocator, CFirst(e), out Rout, out _, out _);
            _zconditional_branch(target, T, Rout, zero_branch);
            // second
            EmitExpression(target, temp_allocator, CSecond(e), out Rout, out _, out _);
            _zconditional_branch(target, T, Rout, zero_branch);

            // both true
            _branch(target, end_branch);

            // zero branch
            target.Labels.Add(zero_branch, _marker(target));
            _const(target, T, Rout, 0);

            // end marker
            target.Labels.Add(end_branch, _marker(target));
            return;
        }

        case ExpressionKind.LogicalOr:
        {
            var end_branch = target.MakeUnnamedLabelName();

            // first
            EmitExpression(target, temp_allocator, CFirst(e), out Rout, out _, out _);
            _conditional_branch(target, T, Rout, end_branch);
            // second
            EmitExpression(target, temp_allocator, CSecond(e), out Rout, out _, out _);

            // implicitly pass result of second expression

            // end marker (for early exit when first == true)
            target.Labels.Add(end_branch, _marker(target));
            return;
        }

        case ExpressionKind.Conditional:
        {
            var cond = CFirst(e);
            var vtrue = CSecond(e);
            var vfalse = CThird(e);

            // constexpr
            if (cond.Kind == ExpressionKind.Literal)
            {
                // ironic
                var dcond = (decimal)cond.Token.Value!;
                e = dcond != 0 ? vtrue : vfalse;
                goto LGenNewExpr;
            }

            var then_label = target.MakeUnnamedLabelName();
            var end_label = target.MakeUnnamedLabelName();
            EmitExpression(target, temp_allocator, cond, out var Rcond, out var Tcond, out var caddr);
            Rout = create_operand_register(temp_allocator, T, caddr, Rcond);
            _conditional_branch(target, Tcond, Rout, then_label);

            EmitExpression(target, temp_allocator, vfalse, out var Rtrue, out _, out _);
            _copy(target, T, Rout, Rtrue);
            _branch(target, end_label); // jump to end

            // then stmt
            target.Labels.Add(then_label, _marker(target));
            EmitExpression(target, temp_allocator, vtrue, out var Rfalse, out _, out _);
            _copy(target, T, Rout, Rfalse);

            // end marker
            target.Labels.Add(end_label, _marker(target));
            return;
        }

        case ExpressionKind.VariableInit:
        case ExpressionKind.Assign:
        {
            var dest = CFirst(e);
            var source = CSecond(e);

            EmitExpression(target, temp_allocator, dest, out var Rdest, out var Tdesc, out _, true);
            EmitExpression(target, temp_allocator, source, out var Rsrc, out _, out _, false);

            // block: use memcopy
            if (dest.Type!.Traits.HasFlag(TypeTraits.Members | TypeTraits.Array))
                _memcopy(target, Rdest, Rsrc, dest.Type!.SizeOf());
            // use copy for locals
            else if (dest.Kind == ExpressionKind.Identifier && target.Auto.Keys.Contains((string)dest.Token.Value!))
                _copy(target, Tdesc, Rdest, Rsrc);
            // use store for others
            else _store(target, Tdesc, Rdest, Rsrc);
            Rout = -1;
            return;
        }

        // e.g. turn x += y into x = x + y
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
            var dest = CFirst(e);
            var parent = e.Parent;
            e = new BinaryExpression(e.Token, ExpressionKind.Assign, dest,
                new BinaryExpression(dest.Token, e.Kind switch
                {
                    ExpressionKind.AssignSum => ExpressionKind.Add,
                    ExpressionKind.AssignDiff => ExpressionKind.Subtract,
                    ExpressionKind.AssignProduct => ExpressionKind.Multiply,
                    ExpressionKind.AssignQuotient => ExpressionKind.Divide,
                    ExpressionKind.AssignRemainder => ExpressionKind.Modulo,
                    ExpressionKind.AssignBitwiseAnd => ExpressionKind.BitwiseAnd,
                    ExpressionKind.AssignBitwiseXOr => ExpressionKind.BitwiseXOr,
                    ExpressionKind.AssignBitwiseOr => ExpressionKind.BitwiseOr,
                    ExpressionKind.AssignLeftShift => ExpressionKind.LeftShift,
                    ExpressionKind.AssignRightShift => ExpressionKind.RightShift,
                    _ => throw new NotImplementedException()
                }, dest, CSecond(e))
                {
                    Type = dest.Type
                }
                )
            {
                Type = FType.VOID,
                Parent = parent,
            };
            goto LGenNewExpr;
        }

        }
    }

    /// <summary>
    /// The name of an automatic-storage variable, using an expression.
    /// </summary>
    private static string AutoName(Expression e)
    {
        if (!e.FindNearestParentScope()!.GetDeclaration<Symbol>((string)e.Token.Value!, e.Token.Source, out var decl))
            Debug.Fail(null);

        return AutoName(decl!);
    }

    /// <summary>
    /// The name of an automatic-storage variable.
    /// </summary>
    private static string AutoName(Symbol sym) => $"{sym.Name}{sym.Table.Parent!.Id}";

    /// <summary>
    /// The name of a function.
    /// </summary>
    private static string FuncName(FunctionNode f) => FuncName(f.Name);
    private static string FuncName(string fname) => $"_{fname}";

    /// <summary>
    /// Converts a Food type to a SSA type.
    /// </summary>
    private static SsaType TFoodToSsa(FType? T) =>
        T is null
            ? SsaType.None
        : (T.Traits & TypeTraits.Pointer) != 0
            ? SsaType.Ptr
        : (T.Traits & TypeTraits.Floating) != 0
            ? T.Size switch
            {
                4 => SsaType.F32,
                8 => SsaType.F64,
                _ => throw new NotImplementedException(),
            }
        : (T.Traits & TypeTraits.Integer) != 0
            ? T.Size switch
            {
                1 => SsaType.B8,
                2 => SsaType.B16,
                4 => SsaType.B32,
                8 => SsaType.B64,
                _ => throw new NotImplementedException(),
            }
        : (T.Traits & TypeTraits.Void) != 0
            ? SsaType.None
        : SsaType.Ptr;

    private static SsaInstruction _marker(SsaFunction target)
    {
        var marker = new SsaInstruction(SsaOpcode.Nop, SsaType.None, -1, -1, -1);
        target.Base.AddInstruction(marker);
        return marker;
    }

    private static void _e(SsaFunction target, SsaInstruction i) => target.Base.AddInstruction(i);
    private static void _nop(SsaFunction target) => _e(target, new(SsaOpcode.Nop, SsaType.None, -1, -1, -1));
    
    private static void _const(SsaFunction target, SsaType T, nint r, nint c) => _e(target, new(SsaOpcode.Const, T, r, c, -1));
    private static void _stringliteraladdr(SsaFunction target, nint r, nint i) => _e(target, new(SsaOpcode.StringLiteralAddr, SsaType.Ptr, r, i, -1));
    private static void _cast(SsaFunction target, SsaType Tin, SsaType Tout, nint d, nint s) => _e(target, new(SsaOpcode.Cast, Tin | (SsaType)((int)Tout << 8), d, s, -1));
    private static void _cast_signed(SsaFunction target, SsaType Tin, SsaType Tout, nint d, nint s) => _e(target, new(SsaOpcode.Cast, Tin | (SsaType)((int)Tout << 8), d, s, -1) { SignFlag = true });
    private static void _copy(SsaFunction target, SsaType T, nint d, nint s) => _e(target, new(SsaOpcode.Copy, T, d, s, -1));
    private static void _store(SsaFunction target, SsaType T, nint d, nint s) => _e(target, new(SsaOpcode.Store, T, d, s, -1));
    private static void _deref(SsaFunction target, SsaType T, nint d, nint s) => _e(target, new(SsaOpcode.Deref, T, d, s, -1));

    private static void _labelref(SsaFunction target, SsaType T, nint r, string label) => _e(target, new(SsaOpcode.LabelRef, T, r, -1, -1, label));

    private static void _autoblockaddr(SsaFunction target, nint d, nint c) => _e(target, new(SsaOpcode.AutoBlockAddr, SsaType.Ptr, d, c, -1));
    private static void _zeroblock(SsaFunction target, nint blk, nint sz) => _e(target, new(SsaOpcode.ZeroBlock, SsaType.Ptr, blk, sz, -1));
    private static void _memcopy(SsaFunction target, nint dest, nint src, nint sz) => _e(target, new(SsaOpcode.MemCopy, SsaType.Ptr, dest, src, sz));

    private static void _add(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Add, T, d, s0, s1));
    private static void _sub(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Sub, T, d, s0, s1));
    private static void _mul(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Mul, T, d, s0, s1));
    private static void _div(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Div, T, d, s0, s1));
    private static void _mod(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Mod, T, d, s0, s1));
    private static void _sign_mul(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Mul, T, d, s0, s1) { SignFlag = true });
    private static void _sign_div(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Div, T, d, s0, s1) { SignFlag = true });
    private static void _sign_mod(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Mod, T, d, s0, s1) { SignFlag = true });

    private static void _negate(SsaFunction target, SsaType T, nint d, nint s) => _e(target, new(SsaOpcode.Negate, T, d, s, -1));
    private static void _and(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.And, T, d, s0, s1));
    private static void _or(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Or, T, d, s0, s1));
    private static void _xor(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Xor, T, d, s0, s1));
    private static void _lsh(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Lsh, T, d, s0, s1));
    private static void _rsh(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Rsh, T, d, s0, s1));
    private static void _not(SsaFunction target, SsaType T, nint d, nint s) => _e(target, new(SsaOpcode.Not, T, d, s, -1));
    private static void _lnot(SsaFunction target, SsaType T, nint d, nint s) => _e(target, new(SsaOpcode.LNot, T, d, s, -1));

    private static void _equal(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Equal, T, d, s0, s1));
    private static void _not_equal(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.NotEqual, T, d, s0, s1));
    private static void _lower(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Lower, T, d, s0, s1));
    private static void _greater(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.Greater, T, d, s0, s1));
    private static void _lower_equal(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.LowerEqual, T, d, s0, s1));
    private static void _greater_equal(SsaFunction target, SsaType T, nint d, nint s0, nint s1) => _e(target, new(SsaOpcode.GreaterEqual, T, d, s0, s1));

    private static void _equalc(SsaFunction target, SsaType T, nint d, nint r, nint c) => _e(target, new(SsaOpcode.EqualC, T, d, r, c));
    private static void _inc(SsaFunction target, SsaType T, nint r) => _e(target, new(SsaOpcode.Inc, T, r, -1, -1));
    private static void _dec(SsaFunction target, SsaType T, nint r) => _e(target, new(SsaOpcode.Dec, T, r, -1, -1));
    private static void _mulc(SsaFunction target, SsaType T, nint r, nint c, bool sign) => _e(target, new(SsaOpcode.MulC, T, r, c, -1));
    private static void _divc(SsaFunction target, SsaType T, nint r, nint c, bool sign) => _e(target, new(SsaOpcode.DivC, T, r, c, -1));
    private static void _sign_mulc(SsaFunction target, SsaType T, nint r, nint c, bool sign) => _e(target, new(SsaOpcode.MulC, T, r, c, -1) { SignFlag = true });
    private static void _sign_divc(SsaFunction target, SsaType T, nint r, nint c, bool sign) => _e(target, new(SsaOpcode.DivC, T, r, c, -1) { SignFlag = true });
    private static void _subc(SsaFunction target, SsaType T, nint r, nint c) => _e(target, new(SsaOpcode.SubC, T, r, c, -1));
    private static void _addc(SsaFunction target, SsaType T, nint r, nint c) => _e(target, new(SsaOpcode.AddC, T, r, c, -1));

    private static void _branch(SsaFunction target, string br) => _e(target, new(SsaOpcode.Branch, SsaType.None, -1, -1, -1, br));
    private static void _conditional_branch(SsaFunction target, SsaType Tc, nint c, string br) => _e(target, new(SsaOpcode.ConditionalBranch, Tc, -1, c, -1, br));
    private static void _zconditional_branch(SsaFunction target, SsaType Tc, nint c, string br) => _e(target, new(SsaOpcode.ZConditionalBranch, Tc, -1, c, -1, br));
    private static void _call(SsaFunction target, SsaType T, nint d, string f, (nint, SsaType)[] args) => _e(target, new(SsaOpcode.Call, T, d, -1, -1, f, args));
    private static void _return(SsaFunction target, SsaType Tr, nint r) => _e(target, new(SsaOpcode.Return, Tr, r, -1, -1));

    private static void _computed_branch(SsaFunction target, nint a) => _e(target, new(SsaOpcode.ComputedBranch, SsaType.None, -1, a, -1));
    private static void _computed_call(SsaFunction target, SsaType Tr, nint d, nint a, (nint, SsaType)[] args) => _e(target, new(SsaOpcode.ComputedCall, Tr, d, a, -1, args));

    private static void _address(SsaFunction target, nint d, nint r) => _e(target, new(SsaOpcode.Address, SsaType.Ptr, d, r, -1));
}
