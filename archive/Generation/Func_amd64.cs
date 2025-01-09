using ck.Ssa;
using System.Text;

namespace ck.Generation;

public sealed class Func_amd64
{
    /// <summary>
    /// The size of the local stack frame.
    /// </summary>
    public int StackFrameSize = 0;

    /// <summary>
    /// The table for register-held variables. -1 means held in stack.
    /// </summary>
    public readonly Dictionary<Variable, int> RegVars = new();

    /// <summary>
    /// The table for register-held floating point variables. -1 means held in stack.
    /// </summary>
    public readonly Dictionary<Variable, int> FloatRegVars = new();

    /// <summary>
    /// The table for stack-held variables.
    /// </summary>
    public readonly Dictionary<Variable, int> StackVars = new();

    /// <summary>
    /// Variable table
    /// </summary>
    public readonly Dictionary<nint, Variable> Vars = new();

    /// <summary>
    /// Volatile register stack
    /// </summary>
    public readonly Stack<int> Volatile = new();

    /// <summary>
    /// The params of the function.
    /// </summary>
    public readonly Variable[] Params;

    /// <summary>
    /// The code for the function.
    /// </summary>
    public readonly StringBuilder Code = new();

    /// <summary>
    /// The name of the symbol.
    /// </summary>
    public readonly string Name;

    public Func_amd64(string name, Variable[] @params)
    {
        Name = name;
        Params = @params;
    }
}
