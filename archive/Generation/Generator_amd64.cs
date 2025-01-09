using ck.Ssa;
using System.Diagnostics;
using System.Text;

namespace ck.Generation;

/// <summary>
/// Generator for the amd64 architecture<br/>
/// supported callconvs: <code>sysv</code>
/// </summary>
public sealed class Generator_amd64
    : Generator<Func_amd64, string>
{
    private struct MachineRegister
    {
        public readonly string Name8;
        public readonly string Name16;
        public readonly string Name32;
        public readonly string Name64;
        public readonly string Name128;

        public string NamePtr => Name64;

        public MachineRegister(string n8, string n16, string n32, string n64, string n128 = "???")
        {
            Name8 = n8;
            Name16 = n16;
            Name32 = n32;
            Name64 = n64;
            Name128 = n128;
        }

        public string GetMoniker(SsaType T) => T switch
        {
            SsaType.B8 => Name8,
            SsaType.B16 => Name16,
            SsaType.B32 => Name32,
            SsaType.B64 => Name64,
            SsaType.Ptr => Name64,
            SsaType.F32 => Name32,
            SsaType.F64 => Name64,
            SsaType.None => Name64,

            _ => throw new NotImplementedException(),
        };
    }

    public Generator_amd64(SsaProgram program) : base(program)
    {
    }

    private readonly StringBuilder _literal_buffer = new();
    private readonly Mutex _literal_buffer_mutex = new();

    private int _literal_current_name = 0;

    private int _create_literal(string literal_def)
    {
        _literal_buffer_mutex.WaitOne();
        _literal_buffer.AppendLine($"LL{_literal_current_name}:");
        _literal_buffer.AppendLine(literal_def);
        var ret = _literal_current_name++;
        _literal_buffer_mutex.ReleaseMutex();
        return ret;
    }

    private string _ref_literal(int name) => $"[LL{name}]";

    protected override string ProgramEpilogue(IEnumerable<Func_amd64> datalist)
    {
        var total_buffer = new StringBuilder();
        foreach (var data in datalist)
        {
            total_buffer.AppendLine($"[global {data.Name}]");
            total_buffer.Append(data.Code.ToString());
        }
        total_buffer.Append(_literal_buffer.ToString());
        return total_buffer.ToString();
    }

    protected override void GenFuncGeneratorData(string fname, SsaFunction f, out Func_amd64 funcdata_out)
    {
        // init
        var funcdata = new Func_amd64(fname, f.Params!.ToArray());
        funcdata_out = funcdata;

        // spill detection
        var localized_vars = 0;
        var spill_candidates = new Variable[f.AllVariables.Count]; // worst case scenario
        var spill_candidate_index = 0;

        var unspilled = f.AllVariables.Where(v => !v.LocalStorage);
        var temp_graph = new InterferenceGraph(unspilled);
        temp_graph.Build();
        while (localized_vars != f.AllVariables.Count)
        {
            // sort now
            spill_candidate_index = 0;
            foreach (var v in unspilled)
            {
                var neighbors = temp_graph.Neighbors(v);
                if (neighbors >= _general_purpose_registers.Length)
                    spill_candidates[spill_candidate_index++] = v;
                else localized_vars++;
            }

            // we are done if we can't spill anything
            if (spill_candidate_index == 0)
                break;

            // spill the variable that is the less costy
            Array.Resize(ref spill_candidates, spill_candidate_index);
            var spill_target = spill_candidates.MinBy(v => v.SpillCost);
            spill_target!.LocalStorage = true;
            temp_graph.RemoveVertex(spill_target);
            localized_vars++;
        }

        // coloring registers
        var register_vars = f.AllVariables.Where(v => !v.LocalStorage).OrderBy(v => v.SpillCost);
        var register_graph = new InterferenceGraph(register_vars);
        register_graph.Build();
        register_graph.NaiveColor(_general_purpose_registers.Length);
        foreach (var reg_var in register_vars)
            funcdata.RegVars.Add(reg_var, reg_var.Color);

        // handling spills
        var stack_vars = f.AllVariables.Where(v => v.LocalStorage);
        foreach (var stack_var in stack_vars)
        {
            var sz = _sizeof_ssa(stack_var.Type);
            sz = CompilerRunner.Align(sz, CompilerRunner.AlignmentRequired);

            funcdata.StackFrameSize += sz;
            funcdata.StackVars.Add(stack_var, -funcdata.StackFrameSize);
        }

        // building var table
        foreach (var v in f.AllVariables)
            funcdata.Vars.Add(v.Name, v);
    }

    // amd64/win64
    private void _prelude_win(Func_amd64 funcdata) => throw new NotImplementedException();
    private void _epilogue_win(Func_amd64 funcdata) => throw new NotImplementedException();

    // amd64/sysv
    private void _prelude_sysv(Func_amd64 funcdata)
    {
        _writeline(funcdata, $"{funcdata.Name}:");

        if (funcdata.StackFrameSize != 0)
        {
            var allocate_stack_frame_size = CompilerRunner.Align(funcdata.StackFrameSize, 16);
            _comment(funcdata, $"stack_frame_init start (sz={allocate_stack_frame_size})");

            _push         (funcdata, "rbp");
            _mov_unchecked(funcdata, SsaType.B64, "rbp", "rsp");
            _gen_insn_1d1o(funcdata, "sub",       "rsp", allocate_stack_frame_size.ToString());
            
            _comment(funcdata, "stack_frame_init end");
            _padline(funcdata);
        }

        // preserving volatile registers
        var all_registers = funcdata.RegVars.Values.Distinct();
        _push(funcdata, R15.Name64);
        foreach (var reg in all_registers)
        {
            if (reg == RBX || (reg <= R12 && reg >= R14) && !funcdata.Volatile.Contains(reg))
            {
                _push(funcdata, _gpr_moniker(reg, SsaType.B64));
                funcdata.Volatile.Push(reg);
            }
        }

        if (funcdata.Params.Length == 0)
            goto LEndParams;

        // placing arguments into the correct register
        const int Gpr_Arguments_Max = 6;
        const int Fpr_Arguments_Max = 8;

        int[] Gpr_Table = [RDI, RSI, RDX, RCX, R8, R9];

        var gpr_arguments = 0; // general
        var fpr_arguments = 0; // float
        var stack_offset  = 0; // current stack offset

        // iterating for register arguments

        //////////////////////////////////////////////////
        // Behavior:
        // push all register arguments and pop them into
        // the correct registers to avoid corruption
        //////////////////////////////////////////////////

        int reg_param_index;
        for (reg_param_index = 0; reg_param_index < funcdata.Params.Length; reg_param_index++)
        {
            var p = funcdata.Params[reg_param_index];

            // float in reg
            if (p.Type == SsaType.F32 || p.Type == SsaType.F64)
            {
                if (fpr_arguments >= Fpr_Arguments_Max)
                    break;

                fpr_arguments++;
                _push(funcdata, _fpr_moniker(fpr_arguments + XMM0, SsaType.F64));
            }
            // int in reg
            else if (p.Type >= SsaType.B8 && p.Type <= SsaType.B64 || p.Type == SsaType.Ptr)
            {
                if (gpr_arguments >= Gpr_Arguments_Max)
                    break;

                gpr_arguments++;
                if (p.Type < SsaType.B32)
                    _gen_insn_1d1o(funcdata, "movzx", _gpr_moniker(Gpr_Table[gpr_arguments], SsaType.B64), _gpr_moniker(Gpr_Table[gpr_arguments], p.Type));
                _push(funcdata, _gpr_moniker(gpr_arguments + RBX, SsaType.B64));
            }
        }

        for (var i = reg_param_index - 1; i >= 0; i--)
        {
            var p = funcdata.Params[i];

            if (p.Type == SsaType.F32 || p.Type == SsaType.F64)
                _pop(funcdata, _varname(funcdata, p, SsaType.F64));
            else
                _pop(funcdata, _varname(funcdata, p, SsaType.B64));
        }

        // no stack arguments, skip ahead
        if (reg_param_index == funcdata.Params.Length)
            goto LEndParams;

        _padline(funcdata);

        // iterating for stack arguments
        for (var stack_param_index = funcdata.Params.Length - 1; stack_param_index > reg_param_index; stack_param_index++)
        {
            var p = funcdata.Params[stack_param_index];
            _mov_addr(funcdata, dest: p, src: $"[rbp+0x{stack_offset.ToString("X2")}]");
            stack_offset += _sizeof_ssa(p.Type);
        }

    LEndParams:
        return;
    }

    private void _epilogue_sysv(Func_amd64 funcdata)
    {
        // restoring volatile registers
        while (funcdata.Volatile.Count != 0)
        {
            var top = funcdata.Volatile.Pop();
            _pop(funcdata, _gpr_moniker(top, SsaType.B64));
        }
        _pop(funcdata, R15.Name64);

        if (funcdata.StackFrameSize != 0)
            _pop(funcdata, "rbp");
        _ret(funcdata);
    }

    protected override void GenFuncPrelude(Func_amd64 funcdata)
    {
        if (CompilerRunner.Target == TargetMachine.Amd64_SysV)
            _prelude_sysv(funcdata);
        else if (CompilerRunner.Target == TargetMachine.Amd64_Win)
            _prelude_win(funcdata);
        else Debug.Fail(null);
    }

    protected override void GenFuncEpilogue(Func_amd64 funcdata)
    {
        if (CompilerRunner.Target == TargetMachine.Amd64_SysV)
            _epilogue_sysv(funcdata);
        else if (CompilerRunner.Target == TargetMachine.Amd64_Win)
            _epilogue_win(funcdata);
        else Debug.Fail(null);
    }
    
    protected override void GenInstruction(Func_amd64 f, SsaInstruction insn)
    {
        var insn_sb = new StringBuilder();
        insn.Print(insn_sb);
        _comment(f, $"{insn_sb.ToString().TrimEnd('\n')}");
        switch (insn.Opcode)
        {
        case SsaOpcode.Nop:
            return;

        case SsaOpcode.Const:
            _mov_const(f, f.Vars[insn.DestinationRegister], /* reinterpret */ insn.Source0);
            return;

        case SsaOpcode.Cast:
        {
            var Tin = (SsaType)((int)insn.Type & 0xFF);
            var Tout = (SsaType)((int)insn.Type >> 8 & 0xFF);
            var to_cast = f.Vars[insn.Source0];
            var result = f.Vars[insn.DestinationRegister];
            var mov_resize = insn.SignFlag ? "movsx" : "movzx";

            if (Tin == SsaType.F32)
            {
                switch (Tout)
                {
                case >= SsaType.B8 and <= SsaType.B64:
                case SsaType.Ptr:
                    _gen_insn_1d1o(f, "movss", _reserved(SsaType.F64), _varname(f, to_cast));
                    _gen_insn_1d1o(f, "cvttss2si", _gpr_moniker(result.Color, SsaType.B64), _reserved(SsaType.F64));
                    break;

                case SsaType.F64:
                    if (result.Color != -1)
                    {
                        _gen_insn_1d1o(f, "movss", _reserved(SsaType.F64), _varname(f, to_cast));
                        _gen_insn_1d1o(f, "pxor", _varname(f, result), _varname(f, result));
                        _gen_insn_1d1o(f, "cvtss2sd", _varname(f, result), _reserved(SsaType.F64));
                        return;
                    }

                    // push f32
                    _gen_insn_1d1o(f, "lea", "rsp", "[rsp-4]");
                    _gen_insn_1d1o(f, "movapd", _reserved(SsaType.F32), _varname(f, to_cast));
                    _gen_insn_1d1o(f, "movss", "[rsp-4]", _reserved(SsaType.F32));

                    // set result to zero
                    // TODO: verify usage of `movapd`:
                    //  - in theory, it should be better than alternatives
                    //    because everything stays in the fp unit
                    _gen_insn_1d1o(f, "pxor", _reserved(SsaType.F64), _reserved(SsaType.F64));
                    _gen_insn_1d1o(f, "movapd", _varname(f, result), _reserved(SsaType.F64));

                    _gen_insn_1d1o(f, "cvtss2sd", _varname(f, result), "[rsp-4]");
                    _gen_insn_1d1o(f, "lea", "rsp", "[rsp+4]"); // pop f32 <discard>
                    return;
                }
            }

            if (Tin == SsaType.F64)
            {
                switch (Tout)
                {
                case >= SsaType.B8 and <= SsaType.B64:
                case SsaType.Ptr:
                    _gen_insn_1d1o(f, "movsd", _reserved(SsaType.F64), _varname(f, to_cast));
                    _gen_insn_1d1o(f, "cvttsd2si", _gpr_moniker(result.Color, SsaType.B64), _reserved(SsaType.F64));
                    break;

                case SsaType.F32:
                    if (result.Color != -1)
                    {
                        _gen_insn_1d1o(f, "movss", _reserved(SsaType.F64), _varname(f, to_cast));
                        _gen_insn_1d1o(f, "pxor", _varname(f, result), _varname(f, result));
                        _gen_insn_1d1o(f, "cvtss2sd", _varname(f, result), _reserved(SsaType.F64));
                        return;
                    }

                    // push f64
                    _gen_insn_1d1o(f, "lea", "rsp", "[rsp-8]");
                    _gen_insn_1d1o(f, "movapd", _reserved(SsaType.F64), _varname(f, to_cast));
                    _gen_insn_1d1o(f, "movss", "[rsp-8]", _reserved(SsaType.F64));

                    // set result to zero
                    // TODO: verify usage of `movapd`:
                    //  - in theory, it should be better than alternatives
                    //    because everything stays in the fp unit
                    _gen_insn_1d1o(f, "pxor", _reserved(SsaType.F64), _reserved(SsaType.F64));
                    _gen_insn_1d1o(f, "movapd", _varname(f, result), _reserved(SsaType.F64));

                    _gen_insn_1d1o(f, "cvtsd2ss", _varname(f, result), "[rsp-8]");
                    _gen_insn_1d1o(f, "lea", "rsp", "[rsp+8]"); // pop f64 <discard>
                    return;
                }
            }

            // Tin is i?, Tout is unknown

            ///////////// INT->FLOAT CONVERSIONS

            if (Tout == SsaType.F32)
            {
                var rax_type = (SsaType)Math.Max((byte)SsaType.B32, (byte)Tin);

                // sign extend if too small to be used in cvtsi2ss
                if (Tin == SsaType.B8 || Tin == SsaType.B16)
                    _gen_insn_1d1o(f, mov_resize, _reserved(SsaType.B32), _varname(f, to_cast));
                else
                    _mov_unchecked(f, rax_type, _reserved(rax_type), _varname(f, to_cast));

                _gen_insn_1d1o(f, "pxor", _reserved(SsaType.F32), _reserved(SsaType.F32));
                _gen_insn_1d1o(f, "cvtsi2ss", _reserved(SsaType.F32), _reserved(rax_type));
                _gen_insn_1d1o(f, "movapd", _varname(f, result), _reserved(SsaType.F32));
            }
            else if (Tout == SsaType.F64)
            {
                var rax_type = (SsaType)Math.Max((byte)SsaType.B32, (byte)Tin);

                // sign extend if too small to be used in cvtsi2ss
                if (Tin == SsaType.B8 || Tin == SsaType.B16)
                    _gen_insn_1d1o(f, mov_resize, _reserved(SsaType.B32), _varname(f, to_cast));
                else
                    _mov_unchecked(f, rax_type, _reserved(rax_type), _varname(f, to_cast));

                _gen_insn_1d1o(f, "pxor", _reserved(SsaType.F32), _reserved(SsaType.F32));
                _gen_insn_1d1o(f, "cvtsi2sd", _reserved(SsaType.F32), _reserved(rax_type));
                _gen_insn_1d1o(f, "movapd", _varname(f, result), _reserved(SsaType.F32));
            }

            ///////////// INT->INT

            if (_sizeof_ssa(Tin) >= _sizeof_ssa(Tout))
                _mov(f, result, to_cast);
            // sign extend
            else if (insn.SignFlag)
                _movsx(f, result, to_cast);
            // zero extend
            else
                _movzx(f, result, to_cast);

            // exit
            return;
        }

        case SsaOpcode.Copy:
            _mov(f, f.Vars[insn.DestinationRegister], f.Vars[insn.Source0]);
            return;

        case SsaOpcode.Store:
        {
            var d = f.Vars[insn.DestinationRegister];
            var s = f.Vars[insn.Source0];

            // reg/reg
            if (d.Color != -1 && s.Color != -1)
            {
                var d_ = $"[{_varname(f, d, SsaType.Ptr)}]";
                var s_ = _varname(f, s, insn.Type);
                _mov_unchecked(f, insn.Type, d_, s_);
                return;
            }

            // stack/reg
            if (d.Color != -1)
            {
                _mov_unchecked(f, SsaType.Ptr, _reserved(SsaType.Ptr), _varname(f, d, SsaType.Ptr));
                _mov_unchecked(f, insn.Type, $"[{_reserved(SsaType.Ptr)}]", _varname(f, s, insn.Type));
                return;
            }

            // stack/stack (pain)
            _mov_unchecked(f, SsaType.Ptr, _reserved(SsaType.Ptr), _varname(f, d, SsaType.Ptr));
            _mov_unchecked(f, insn.Type, R15.GetMoniker(insn.Type), _varname(f, s, insn.Type));
            _mov_unchecked(f, insn.Type, $"[{_reserved(SsaType.Ptr)}]", R15.GetMoniker(insn.Type));
            return;
        }

        case SsaOpcode.Deref:
        {
            var d = f.Vars[insn.DestinationRegister];
            var s = f.Vars[insn.Source0];

            // reg/reg
            if (d.Color != -1 && s.Color != -1)
            {
                _mov_unchecked(f, insn.Type, _varname(f, d, insn.Type), $"[{_varname(f, s, SsaType.Ptr)}]");
                return;
            }

            // reg/stack
            if (s.Color != -1)
            {
                _mov_unchecked(f, SsaType.Ptr, _reserved(SsaType.Ptr), _varname(f, s, SsaType.Ptr));
                _mov_unchecked(f, insn.Type, _varname(f, d, insn.Type), $"[{_reserved(SsaType.Ptr)}]");
                return;
            }

            // stack/stack (pain)
            _mov_unchecked(f, SsaType.Ptr, _reserved(SsaType.Ptr), $"[{_varname(f, s, SsaType.Ptr)}]");
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), $"[{_reserved(SsaType.Ptr)}]");
            _mov_unchecked(f, insn.Type, _varname(f, d, insn.Type), _reserved(insn.Type));
            return;
        }

        case SsaOpcode.LabelRef:
            _mov_unchecked(f, insn.Type, _varname(f, f.Vars[insn.DestinationRegister]), insn.Fork!);
            return;

        case SsaOpcode.AutoBlockAddr:
        case SsaOpcode.StringLiteralAddr:
        case SsaOpcode.ZeroBlock:
        case SsaOpcode.MemCopy:
            _comment(f, "{not implemented}");
            return;

        case SsaOpcode.AddC:
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, f.Vars[insn.Source0]));

            // float
            if (_is_float_ssa(insn.Type))
            {
                var fconst = _create_literal((insn.Type == SsaType.B32 ? "dd" : "dq") + " " + insn.Source1.ToString());
                _mov_unchecked(f, SsaType.F64, XMM14.GetMoniker(insn.Type), _ref_literal(fconst));
                _gen_insn_1d1o(f, insn.Type == SsaType.B32 ? "addss" : "addsd", _reserved(insn.Type), XMM14.GetMoniker(insn.Type));
            }
            else // integers
            {
                // no imm64
                if (insn.Source1 > int.MaxValue)
                {
                    _mov_unchecked(f, SsaType.B64, R15.GetMoniker(SsaType.B64), insn.Source1.ToString());
                    _gen_insn_1d1o(f, "add", _reserved(insn.Type), R15.GetMoniker(insn.Type));
                }
                else
                    _gen_insn_1d1o(f, "add", _reserved(insn.Type), insn.Source1.ToString());
            }

            _mov_unchecked(f, insn.Type, _varname(f, f.Vars[insn.DestinationRegister]), _reserved(insn.Type));
            return;

        case SsaOpcode.SubC:
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, f.Vars[insn.Source0]));

            // float
            if (_is_float_ssa(insn.Type))
            {
                var fconst = _create_literal((insn.Type == SsaType.B32 ? "dd" : "dq") + " " + insn.Source1.ToString());
                _mov_unchecked(f, SsaType.F64, XMM14.GetMoniker(insn.Type), _ref_literal(fconst));
                _gen_insn_1d1o(f, insn.Type == SsaType.B32 ? "subss" : "subsd", _reserved(insn.Type), XMM14.GetMoniker(insn.Type));
            }
            else // integers
            {
                // no imm64
                if (insn.Source1 > int.MaxValue)
                {
                    _mov_unchecked(f, SsaType.B64, R15.GetMoniker(SsaType.B64), insn.Source1.ToString());
                    _gen_insn_1d1o(f, "sub", _reserved(insn.Type), R15.GetMoniker(insn.Type));
                }
                else
                    _gen_insn_1d1o(f, "sub", _reserved(insn.Type), insn.Source1.ToString());
            }

            _mov_unchecked(f, insn.Type, _varname(f, f.Vars[insn.DestinationRegister]), _reserved(insn.Type));
            return;

        case SsaOpcode.MulC:
            if (_is_float_ssa(insn.Type))
            {
                var fconst = _create_literal((insn.Type == SsaType.B32 ? "dd" : "dq") + " " + insn.Source1.ToString());
                _mov_unchecked(f, SsaType.F64, _reserved(SsaType.F64), _ref_literal(fconst));
                _mov(f, f.Vars[insn.DestinationRegister], f.Vars[insn.Source0]);
                _gen_insn_1d1o(f, insn.Type == SsaType.B32 ? "mulss" : "mulsd", _varname(f, f.Vars[insn.DestinationRegister]), _reserved(SsaType.F64));
                return;
            }

            // integers
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, f.Vars[insn.Source0]));
            _mov_unchecked(f, insn.Type, R15.GetMoniker(insn.Type), insn.Source1.ToString()); // imm64 support
            _gen_insn_1d0o(f, insn.SignFlag ? "imul" : "mul", R15.GetMoniker(insn.Type));
            _mov_unchecked(f, insn.Type, _varname(f, f.Vars[insn.DestinationRegister]), _reserved(insn.Type));
            return;

        case SsaOpcode.DivC:
            if (_is_float_ssa(insn.Type))
            {
                var fconst = _create_literal((insn.Type == SsaType.B32 ? "dd" : "dq") + " " + insn.Source1.ToString());
                _mov_unchecked(f, SsaType.F64, _reserved(SsaType.F64), _ref_literal(fconst));
                _mov(f, f.Vars[insn.DestinationRegister], f.Vars[insn.Source0]);
                _gen_insn_1d1o(f, insn.Type == SsaType.B32 ? "divss" : "divsd", _varname(f, f.Vars[insn.DestinationRegister]), _reserved(SsaType.F64));
                return;
            }

            // integers
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, f.Vars[insn.Source0]));
            _mov_unchecked(f, insn.Type, R15.GetMoniker(insn.Type), insn.Source1.ToString()); // imm64 support
            _gen_insn_0d0o(f, "cdq");
            _gen_insn_1d0o(f, insn.SignFlag ? "idiv" : "div", R15.GetMoniker(insn.Type));
            _mov_unchecked(f, insn.Type, _varname(f, f.Vars[insn.DestinationRegister]), _reserved(insn.Type));
            return;

        case SsaOpcode.EqualC:
            _cmp_const(f, f.Vars[insn.DestinationRegister], f.Vars[insn.Source0], insn.Source1);
            return;

        case SsaOpcode.Inc:
            _gen_insn_1d1o(f, "add", _varname(f, f.Vars[insn.DestinationRegister]), "1");
            return;

        case SsaOpcode.Dec:
            _gen_insn_1d1o(f, "sub", _varname(f, f.Vars[insn.DestinationRegister]), "1");
            return;

        case SsaOpcode.Add:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            // floats
            _mov(f, dest, src0);
            if (_is_float_ssa(insn.Type))
            {
                if (dest.Color == -1 && src1.Color == -1)
                {
                    _mov_unchecked(f, SsaType.F64, _reserved(SsaType.F64), _varname(f, src1));
                    _gen_insn_1d1o(f, insn.Type == SsaType.F32 ? "addss" : "addsd", _varname(f, dest), _reserved(SsaType.F64));
                    return;
                }

                _gen_insn_1d1o(f, insn.Type == SsaType.F32 ? "addss" : "addsd", _varname(f, dest), _varname(f, src1));
            }

            // integers
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, SsaType.F64, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "add", _varname(f, dest), _reserved(insn.Type));
                return;
            }

            _gen_insn_1d1o(f, "add", _varname(f, dest, dest.Type), _varname(f, src1, dest.Type));
            return;
        }

        case SsaOpcode.Sub:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            // floats
            _mov(f, dest, src0);
            if (_is_float_ssa(insn.Type))
            {
                if (dest.Color == -1 && src1.Color == -1)
                {
                    _mov_unchecked(f, SsaType.F64, _reserved(SsaType.F64), _varname(f, src1));
                    _gen_insn_1d1o(f, insn.Type == SsaType.F32 ? "subss" : "subsd", _varname(f, dest), _reserved(SsaType.F64));
                    return;
                }

                _gen_insn_1d1o(f, insn.Type == SsaType.F32 ? "subss" : "subsd", _varname(f, dest), _varname(f, src1));
            }

            // integers
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, SsaType.F64, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "sub", _varname(f, dest), _reserved(insn.Type));
                return;
            }

            _gen_insn_1d1o(f, "sub", _varname(f, dest), _varname(f, src1));
            return;
        }

        case SsaOpcode.Mul:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            // floats
            if (_is_float_ssa(insn.Type))
            {
                _mov(f, dest, src0);
                _mov_unchecked(f, SsaType.F64, _reserved(SsaType.F64), _varname(f, src1));
                _gen_insn_1d1o(f, insn.Type == SsaType.F32 ? "mulss" : "mulsd", _varname(f, dest), _reserved(SsaType.F64));
                return;
            }

            // integers
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src0));
            _gen_insn_1d0o(f, insn.SignFlag ? "imul" : "mul", _varname(f, src1));
            _mov_unchecked(f, insn.Type, _varname(f, dest), _reserved(insn.Type));
            return;
        }

        case SsaOpcode.Div:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            // floats
            if (_is_float_ssa(insn.Type))
            {
                _mov(f, dest, src0);
                _mov_unchecked(f, SsaType.F64, _reserved(SsaType.F64), _varname(f, src1));
                _gen_insn_1d1o(f, insn.Type == SsaType.F32 ? "divss" : "divsd", _varname(f, dest), _reserved(SsaType.F64));
                return;
            }

            // integers
            _mov_unchecked(f, SsaType.B64, R15.Name64, _general_purpose_registers[RDX].Name64);
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src0));
            _gen_insn_1d0o(f, insn.SignFlag ? "idiv" : "div", _varname(f, src1));
            _mov_unchecked(f, insn.Type, _varname(f, dest), _reserved(insn.Type));
            _mov_unchecked(f, SsaType.B64, _general_purpose_registers[RDX].Name64, R15.Name64);
            return;
        }

        case SsaOpcode.Mod:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            // integers only
            _mov_unchecked(f, SsaType.B64, R15.Name64, _general_purpose_registers[RDX].Name64);
            _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src0));
            _gen_insn_1d0o(f, insn.SignFlag ? "idiv" : "div", _varname(f, src1));
            _mov_unchecked(f, insn.Type, _varname(f, dest), _general_purpose_registers[RDX].Name64);
            _mov_unchecked(f, SsaType.B64, _general_purpose_registers[RDX].Name64, R15.Name64);
            return;
        }

        case SsaOpcode.And:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            _mov(f, dest, src0);
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "and", _varname(f, dest), _reserved(insn.Type));
                return;
            }

            _gen_insn_1d1o(f, "and", _varname(f, dest), _varname(f, src1));
            return;
        }

        case SsaOpcode.Or:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            _mov(f, dest, src0);
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "or", _varname(f, dest), _reserved(insn.Type));
                return;
            }

            _gen_insn_1d1o(f, "or", _varname(f, dest), _varname(f, src1));
            return;
        }

        case SsaOpcode.Xor:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            _mov(f, dest, src0);
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "xor", _varname(f, dest), _reserved(insn.Type));
                return;
            }

            _gen_insn_1d1o(f, "xor", _varname(f, dest), _varname(f, src1));
            return;
        }

        case SsaOpcode.Lsh:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            _mov(f, dest, src0);
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "sal", _varname(f, dest), _reserved(SsaType.B8));
                return;
            }

            _gen_insn_1d1o(f, "sal", _varname(f, dest), _varname(f, src1, SsaType.B8));
            return;
        }

        case SsaOpcode.Rsh:
        {
            var src0 = f.Vars[insn.Source0];
            var src1 = f.Vars[insn.Source1];
            var dest = f.Vars[insn.DestinationRegister];

            _mov(f, dest, src0);
            if (dest.Color == -1 && src1.Color == -1)
            {
                _mov_unchecked(f, insn.Type, _reserved(insn.Type), _varname(f, src1));
                _gen_insn_1d1o(f, "sar", _varname(f, dest), _reserved(SsaType.B8));
                return;
            }

            _gen_insn_1d1o(f, "sar", _varname(f, dest), _varname(f, src1, SsaType.B8));
            return;
        }

        case SsaOpcode.Branch:
            _gen_insn_1d0o(f, "jmp", "." + insn.Fork!);
            return;

        default:
            _gen_insn_0d0o(f, "nop");
            return;

        }
    }

    protected override void GenLabel(Func_amd64 f, string label)
        => _writeline(f, $".{label}:");

    private string _varname(Func_amd64 funcdata, Variable v, SsaType T_override = SsaType.None) =>
        v.Color != -1 ? _gpr_moniker(v.Color, T_override != SsaType.None ? T_override : v.Type) : $"[rbp-{funcdata.StackVars[v]}]";

    #region cmp

    private void _cmp_const(Func_amd64 funcdata, Variable dest, Variable a, nint @const)
    {
        _mov_const(funcdata, dest, 0);

        if (_is_float_ssa(dest.Type))
        {
            // comparison (if @const==0)
            if (@const == 0)
            {
                _gen_insn_1d1o(funcdata, "pxor", _reserved(SsaType.F64), _reserved(SsaType.F64));
                _gen_insn_1d1o(funcdata, dest.Type == SsaType.F32 ? "ucomiss" : "ucomisd", _varname(funcdata, a, a.Type), _reserved(SsaType.F64));
            }
            else
            {
                var fconst = _create_literal(a.Type == SsaType.F32 ? "dd" : "dq");
                _mov_unchecked(funcdata, SsaType.F64, _reserved(SsaType.F64), _ref_literal(fconst));
                _gen_insn_1d1o(funcdata, dest.Type == SsaType.F32 ? "ucomiss" : "ucomisd", _varname(funcdata, a, a.Type), _reserved(SsaType.F64));
            }

            // setting
            _gen_insn_1d1o(funcdata, "xor", R15.Name64, R15.Name64);
            _gen_insn_1d0o(funcdata, "setnp", _varname(funcdata, dest, SsaType.B8));
            _gen_insn_1d1o(funcdata, "cmovne", _varname(funcdata, dest), R15.GetMoniker(dest.Type));
            return;
        }

        // comparison (if @const==0)
        if (@const == 0 && a.Color != -1)
            _gen_insn_1d1o(funcdata, "test", _varname(funcdata, dest), _varname(funcdata, dest));
        else
        {
            if (@const > int.MaxValue)
            {
                // imm64
                _mov_unchecked(funcdata, SsaType.B64, _reserved(SsaType.B64), @const.ToString());
                _gen_insn_1d1o(funcdata, "cmp", _varname(funcdata, dest), _reserved(SsaType.B64));
            }
            else
                _gen_insn_1d1o(funcdata, "cmp", _varname(funcdata, dest), @const.ToString());
        }

        // setting
        _gen_insn_1d0o(funcdata, "sete", _varname(funcdata, dest, SsaType.B8));
    }

    private void _cmp(Func_amd64 funcdata, Variable dest, Variable a, Variable b)
    {
        _mov_const(funcdata, dest, 0);

        if (_is_float_ssa(dest.Type))
        {
            // comparison
            if (a.Color == 1 && b.Color == -1)
            {
                _mov_unchecked(funcdata, a.Type, _reserved(a.Type), _varname(funcdata, a, a.Type));
                _gen_insn_1d1o(funcdata, dest.Type == SsaType.F32 ? "ucomiss" : "ucomisd", _reserved(a.Type), _varname(funcdata, b, b.Type));
            }
            else
                _gen_insn_1d1o(funcdata, dest.Type == SsaType.F32 ? "ucomiss" : "ucomisd", _varname(funcdata, a, a.Type), _varname(funcdata, b, b.Type));

            // setting
            _gen_insn_1d1o(funcdata, "xor", R15.Name64, R15.Name64);
            _gen_insn_1d0o(funcdata, "setnp", _varname(funcdata, dest, SsaType.B8));
            _gen_insn_1d1o(funcdata, "cmovne", _varname(funcdata, dest), R15.GetMoniker(dest.Type));
            return;
        }

        // comparison
        if (a.Color == -1 && b.Color == -1)
        {
            _mov_unchecked(funcdata, a.Type, _reserved(a.Type), _varname(funcdata, a, a.Type));
            _gen_insn_1d1o(funcdata, "cmp", _reserved(a.Type), _varname(funcdata, b, a.Type));
        }
        else
            _gen_insn_1d1o(funcdata, "cmp", _varname(funcdata, a, a.Type), _varname(funcdata, b, b.Type));

        // setting
        _gen_insn_1d0o(funcdata, "sete", _varname(funcdata, dest, SsaType.B8));
    }

    #endregion

    #region mov

    private void _mov(Func_amd64 funcdata, Variable dest, Variable src)
    {
        if (dest.Color == -1 && src.Color == -1)
        {
            _mov_unchecked(funcdata, dest.Type, _reserved(dest.Type), _varname(funcdata, src, dest.Type));
            _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _reserved(dest.Type));
            return;
        }

        _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _varname(funcdata, src, dest.Type));
    }

    private void _movzx(Func_amd64 funcdata, Variable dest, Variable src)
    {
        // standard behaviour
        if (src.Type != SsaType.B32)
        {
            if (dest.Color == -1 && src.Color == -1)
            {
                _gen_insn_1d1o(funcdata, "movzx", _reserved(dest.Type), _varname(funcdata, src));
                _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _reserved(dest.Type));
                return;
            }

            _gen_insn_1d1o(funcdata, "movzx", _varname(funcdata, dest), _varname(funcdata, src));
            return;
        }

        // only r8/r16/m8/m16 supported, otherwise use regular mov between 32-bit registers
        if (dest.Color == -1 && src.Color == -1)
        {
            _mov_unchecked(funcdata, src.Type, _reserved(dest.Type), _varname(funcdata, src));
            _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _reserved(dest.Type));
            return;
        }

        _mov_unchecked(funcdata, src.Type, _varname(funcdata, dest, src.Type), _varname(funcdata, src, src.Type));
    }

    private void _movsx(Func_amd64 funcdata, Variable dest, Variable src)
    {
        if (dest.Color == -1 && src.Color == -1)
        {
            _gen_insn_1d1o(funcdata, "movsx", _reserved(dest.Type), _varname(funcdata, src));
            _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _reserved(dest.Type));
            return;
        }

        _gen_insn_1d1o(funcdata, "movsx", _varname(funcdata, dest), _varname(funcdata, src));
    }

    private void _mov_const(Func_amd64 funcdata, Variable v, nint @const)
    {
        if (@const == 0 && v.Color != -1)
        {
            _gen_insn_1d1o(funcdata, "xor", _varname(funcdata, v), _varname(funcdata, v));
            return;
        }

        if (v.Type == SsaType.F32)
        {
            var float_literal = _create_literal($"dd {@const}");
            _mov_addr(funcdata, dest: v, src: _ref_literal(float_literal));
            return;
        }
        else if (v.Type == SsaType.F64)
        {
            var float_literal = _create_literal($"dq {@const}");
            _mov_addr(funcdata, dest: v, src: _ref_literal(float_literal));
            return;
        }

        _mov_unchecked(funcdata, v.Type, _varname(funcdata, v), $"0x{@const.ToString("X2")}");
    }

    private void _mov_nofloat(Func_amd64 funcdata, Variable dest, int src)
        => _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _gpr_moniker(src, dest.Type));
    private void _mov_nofloat(Func_amd64 funcdata, int dest, Variable src)
        => _mov_unchecked(funcdata, src.Type, _gpr_moniker(dest, src.Type), _varname(funcdata, src));

    private void _mov_float(Func_amd64 funcdata, Variable dest, int src)
        => _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _fpr_moniker(src, dest.Type));
    private void _mov_float(Func_amd64 funcdata, int dest, Variable src)
        => _mov_unchecked(funcdata, src.Type, _fpr_moniker(dest, src.Type), _varname(funcdata, src));

    private void _mov_addr(Func_amd64 funcdata, string dest, Variable src)
    {
        if (src.Color == -1)
        {
            _mov_unchecked(funcdata, src.Type, _reserved(src.Type), _varname(funcdata, src));
            _mov_unchecked(funcdata, src.Type, dest, _reserved(src.Type));
            return;
        }

        _mov_unchecked(funcdata, src.Type, dest, _varname(funcdata, src));
    }

    private void _mov_addr(Func_amd64 funcdata, Variable dest, string src)
    {
        if (dest.Color == -1)
        {
            _mov_unchecked(funcdata, dest.Type, _reserved(dest.Type), src);
            _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), _reserved(dest.Type));
            return;
        }

        _mov_unchecked(funcdata, dest.Type, _varname(funcdata, dest), src);
    }

    private void _mov_unchecked(Func_amd64 funcdata, SsaType T, string dest, string src)
    {
        // (e.g. `mov eax, eax` is valid to zero-extend rax)
        if (((dest == src) && (T != SsaType.B32)) || T == SsaType.None)
        {
            _gen_insn_0d0o(funcdata, "nop ; (opt out)");
            return;
        }

        _gen_insn_1d1o(funcdata, T switch
        {
            SsaType.B8 => "mov",
            SsaType.B16 => "mov",
            SsaType.B32 => "mov",
            SsaType.B64 => "mov",
            SsaType.Ptr => "mov",
            SsaType.F32 => "movss",
            SsaType.F64 => "movsd",
            _ => throw new NotImplementedException()
        }, dest, src);
        return;
    }

    #endregion

    private void _ret(Func_amd64 funcdata) => _gen_insn_0d0o(funcdata, "ret");

    private void _pop(Func_amd64 funcdata, string d) => _gen_insn_1d0o(funcdata, "pop", d);
    private void _push(Func_amd64 funcdata, string s) => _gen_insn_1d0o(funcdata, "push", s);

    private void _gen_insn_0d0o(Func_amd64 funcdata, string name) => _code(funcdata, $"{name}");
    private void _gen_insn_1d0o(Func_amd64 funcdata, string name, string d) => _code(funcdata, $"{name} {d}");
    private void _gen_insn_1d1o(Func_amd64 funcdata, string name, string d, string s) => _code(funcdata, $"{name} {d}, {s}");

        /// <summary>
        /// The red-zone (amd64/sysv) is 128 bytes long.
        /// </summary>
    private const int RedzoneSize = 128;

    private readonly MachineRegister RAX = new("al", "ax", "eax", "rax");
    private readonly MachineRegister R15 = new("r15b", "r15w", "r15d", "r15");
    private readonly MachineRegister XMM14 = new("???", "???", "xmm14", "xmm14", "xmm14");
    private readonly MachineRegister XMM15 = new("???", "???", "xmm15", "xmm15", "xmm15");

    private readonly MachineRegister[] _general_purpose_registers =
    {
        //new("al", "ax", "eax", "rax"), used for internal systems (like copying from mem to mem)
        new("bl", "bx", "ebx", "rbx"),
        new("cl", "cx", "ecx", "rcx"),
        new("dl", "dx", "edx", "rdx"),

        new("r8b", "r8w", "r8d", "r8"),
        new("r9b", "r9w", "r9d", "r9"),
        new("r10b", "r10w", "r10d", "r10"),
        new("r11b", "r11w", "r11d", "r11"),
        new("r12b", "r12w", "r12d", "r12"),
        new("r13b", "r13w", "r13d", "r13"),
        new("r14b", "r14w", "r14d", "r14"),
        //new("r15b", "r15w", "r15d", "r15"), used for internal systems (as a second register)

        new("sil", "si", "esi", "rsi"),
        new("dil", "di", "edi", "rdi"),
    };

    private const int RBX = 0;
    private const int RCX = 1;
    private const int RDX = 2;
    private const int R8 = 3;
    private const int R9 = 4;
    private const int R10 = 5;
    private const int R11 = 6;
    private const int R12 = 7;
    private const int R13 = 8;
    private const int R14 = 9;
    private const int RSI = 10;
    private const int RDI = 11;

    private readonly MachineRegister[] _floating_point_registers =
    {
        // n128 support
        new("???", "???", "xmm0", "xmm0", "xmm0"),
        new("???", "???", "xmm1", "xmm1", "xmm1"),
        new("???", "???", "xmm2", "xmm2", "xmm2"),
        new("???", "???", "xmm3", "xmm3", "xmm3"),
        new("???", "???", "xmm4", "xmm4", "xmm4"),
        new("???", "???", "xmm5", "xmm5", "xmm5"),
        new("???", "???", "xmm6", "xmm6", "xmm6"),
        new("???", "???", "xmm7", "xmm7", "xmm7"),
        new("???", "???", "xmm8", "xmm8", "xmm8"),
        new("???", "???", "xmm9", "xmm9", "xmm9"),
        new("???", "???", "xmm10", "xmm10", "xmm10"),
        new("???", "???", "xmm11", "xmm11", "xmm11"),
        new("???", "???", "xmm12", "xmm12", "xmm12"),
        new("???", "???", "xmm13", "xmm13", "xmm13"),
        // new("???", "???", "xmm14", "xmm14", "xmm14"), reserved
        //new("???", "???", "xmm15", "xmm15", "xmm15"),  reserved
    };

    private const int XMM0 = 0;
    private const int XMM1 = 1;
    private const int XMM2 = 2;
    private const int XMM3 = 3;
    private const int XMM4 = 4;
    private const int XMM5 = 5;
    private const int XMM6 = 6;
    private const int XMM7 = 7;
    private const int XMM8 = 8;
    private const int XMM9 = 9;
    private const int XMM10 = 10;
    private const int XMM11 = 11;
    private const int XMM12 = 12;
    private const int XMM13 = 13;

    private string _reserved(SsaType T) => T switch
    {
        SsaType.B8 => RAX.Name8,
        SsaType.B16 => RAX.Name16,
        SsaType.B32 => RAX.Name32,
        SsaType.B64 => RAX.Name64,
        SsaType.Ptr => RAX.Name64,
        SsaType.None => RAX.Name64,

        SsaType.F32 => XMM15.Name32,
        SsaType.F64 => XMM15.Name64,

        _ => throw new NotImplementedException(),
    };

    private string _fpr_moniker(int reg, SsaType T)
    {
        Debug.Assert(reg >= 0 && reg < _floating_point_registers.Length);
        var gpr = _floating_point_registers[reg];
        return T switch
        {
            SsaType.F32 => gpr.Name32,
            SsaType.F64 => gpr.Name64,
            _ => throw new NotImplementedException()
        };
    }

    private string _gpr_moniker(int reg, SsaType T)
    {
        Debug.Assert(reg >= 0 && reg < _general_purpose_registers.Length);
        var gpr = _general_purpose_registers[reg];
        return T switch
        {
            SsaType.B8 => gpr.Name8,
            SsaType.B16 => gpr.Name16,
            SsaType.B32 => gpr.Name32,
            SsaType.B64 => gpr.Name64,

            SsaType.Ptr => gpr.Name64,

            SsaType.None => gpr.Name64,
            _ => throw new NotImplementedException()
        };
    }

    private void _writeline(Func_amd64 f, string text) => f.Code.AppendLine(text);
    private void _comment(Func_amd64 f, string text) => _writeline(f, $";{text}");
    private void _code(Func_amd64 f, string line) => _writeline(f, $"    {line}");
    private void _padline(Func_amd64 f) => f.Code.AppendLine();

    private SsaType _cast_t(SsaType Tin, SsaType Tout) => Tin | (SsaType)((int)Tout << 8);
    private bool _is_float_ssa(SsaType T) => T == SsaType.F32 || T == SsaType.F64;
    private int _sizeof_ssa(SsaType T) => T switch
    {
        SsaType.B8 => 1,
        SsaType.B16 => 2,
        SsaType.B32 => 4,
        SsaType.B64 => 8,

        SsaType.Ptr => 8,

        SsaType.F32 => 4,
        SsaType.F64 => 8,

        SsaType.None => 8,
        _ => throw new NotImplementedException()
    };
}
