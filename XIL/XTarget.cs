using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.XIL;

/// <summary>
/// X target generator state context.
/// </summary>
public struct XTargetState<U> where U : struct
{
    /// <summary>
    /// Switch to true to signal the compiler that the code has caused a debug abortion.
    /// </summary>
    public bool Sig_Abort = false;

    /// <summary>
    /// Switch to true to signal the compiler that the target is done generating the function.
    /// </summary>
    public bool Sig_Done = false;

    /// <summary>
    /// Whether the current function should return. On a top-level call, this will have the same
    /// effect as <see cref="Sig_Done"/>.
    /// </summary>
    public bool Sig_Return = false;

    /// <summary>
    /// The return value of the function.
    /// </summary>
    public ulong ReturnValue;

    /// <summary>
    /// The call frame.
    /// </summary>
    public U Frame;

    public XTargetState() { }
}

/// <summary>
/// An object that represents a comptime return destination.
/// </summary>
/// <param name="From">The method it originated from.</param>
/// <param name="NextInsn">The next instruction in the method it originated from.</param>
public record struct XCallObject<U>(XMethod From, int NextInsn, U CallObject);

/// <summary>
/// A target that can be used to produce/execute XIL code.
/// </summary>
/// <typeparam name="T">The resulting type of the image.</typeparam>
/// <typeparam name="U">The call stack object type.</typeparam>
public abstract class XTarget<T, U> where U : struct
{
    /// <summary>
    /// The name of the target. Name used by the user to specify
    /// which target is currently used.
    /// </summary>
    public abstract string GetName();

    /// <summary>
    /// Process an instruction.
    /// </summary>
    /// <param name="func">The function the instruction comes from.</param>
    /// <param name="state">A state structure used to communicate with the compiler generator.</param>
    /// <param name="ip">The instruction pointer, in XIL instructions.</param>
    /// <returns>null in normal scenarios. Otherwise, returns the function to generate.</returns>
    public abstract XCallObject<U>? ProcessInsn(XMethod func, ref XTargetState<U> state, ref int ip, ulong last_return_value);

    /// <summary>
    /// Produces a function header (at the beginning of a function.)
    /// </summary>
    public abstract void CreateFunctionHeader(XMethod func, ref XTargetState<U> state);

    /// <summary>
    /// Produces a function footer (at the end of a function.) You do not need
    /// to manually produce the finally{} block.
    /// </summary>
    public abstract void CreateFunctionFooter(XMethod func, ref XTargetState<U> state);

    /// <summary>
    /// Dumps debug information about the current state of the target.
    /// </summary>
    /// <param name="func">The function that caused the error.</param>
    /// <param name="ip">The instruction pointer that caused the error. If -1, produced either at the header or footer of a function.</param>
    /// <returns></returns>
    public virtual string DumpDebugInformation(XMethod func, int ip)
    {
        return $"[target={GetType().Name}] abortion occured in {func.Name} at 0x{ip:16X}";
    }

    /// <summary>
    /// Queries the result of the target to produce an image.
    /// Output is the final output of the compiler that will be written to a file.
    /// </summary>
    /// <param name="module">The module that will be generated.</param>
    /// <returns>The queried image.</returns>
    public abstract T ProduceImage(XModule module);
}
