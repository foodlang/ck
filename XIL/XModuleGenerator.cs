﻿using ck.Diagnostics;
using System.Diagnostics;

namespace ck.XIL;

/// <summary>
/// Generates a module using a given target.
/// </summary>
public sealed class XModuleGenerator<T, U> where T : XTarget<T, U> where U : struct
{
    /// <summary>
    /// The target that will be used.
    /// </summary>
    public readonly T Target;

    /// <summary>
    /// The module that will be used.
    /// </summary>
    public readonly XModule Module;

    /// <summary>
    /// If true, the module is done generating.
    /// </summary>
    public bool Done { get; private set; } = false;

    /// <summary>
    /// Target-specific object for each function
    /// </summary>
    public readonly U FunctionObject;

    public XModuleGenerator(XModule module, T target, U function_object)
    {
        Target = target;
        Module = module;
        FunctionObject = function_object;
    }

    /// <summary>
    /// Runs the target generator on a given function of the module.
    /// </summary>
    /// <param name="xil_name">The name, in the XIL module, of the function.</param>
    public ulong RunSingle(string xil_name, U arguments)
    {
        var state = new XTargetState<U>();
        state.Frame = arguments;

        XMethod? entrypoint = null;
        foreach (var func in Module.GetMethods())
        {
            if (func.Name == xil_name)
                entrypoint = func;
        }

        if (entrypoint is null)
        {
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", $"could not find function with name '{xil_name}' (mangled) in module '{Module.Name}'.");
            return state.ReturnValue;
        }

        Target.CreateFunctionHeader(entrypoint, ref state);
        if (state.Sig_Done || state.Sig_Return)
            return state.ReturnValue;
        if (state.Sig_Abort)
        {
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", Target.DumpDebugInformation(entrypoint, -1));
            return state.ReturnValue;
        }

        int ip = 0;
        ulong last_return_value = 0;
        do
        {
            var Ncall = Target.ProcessInsn(entrypoint, ref state, ref ip, last_return_value);
            if (state.Sig_Abort)
            {
                Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", Target.DumpDebugInformation(entrypoint, -1));
                return state.ReturnValue;
            }

            if (state.Sig_Return)
            {
                /*Debug.Assert(state.Callstack.Count > 1);

                state.Sig_Return = false;
                var last_frame = state.Callstack.Pop();
                if (last_frame is null)
                    break;
                entrypoint = last_frame.Value.From;
                ip = last_frame.Value.NextInsn;*/
                break;
            }

            if (Ncall is not null)
            {
                var call = Ncall.Value;
                last_return_value = RunSingle(call.From.Name, call.CallObject);
            }

        } while (!state.Sig_Done);

        Target.CreateFunctionFooter(entrypoint, ref state);
        if (state.Sig_Abort)
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", Target.DumpDebugInformation(entrypoint, -1));

        return state.ReturnValue;
    }

    /// <summary>
    /// Runs the target generator on the whole module.
    /// </summary>
    public void Run()
    {
        foreach (var method in Module.GetMethods())
            RunSingle(method.Name, default);
    }
}
