using System.Security;
using System.Text;

namespace ck.Ssa;

/// <summary>
/// An instruction in SSA form.
/// </summary>
public sealed class SsaInstruction
{
    /// <summary>
    /// The opcode of the instruction.
    /// </summary>
    public readonly SsaOpcode Opcode;

    /// <summary>
    /// The register used as a destination.
    /// </summary>
    public readonly nint DestinationRegister;

    /// <summary>
    /// The first source register
    /// </summary>
    public readonly nint Source0;

    /// <summary>
    /// The second source register
    /// </summary>
    public readonly nint Source1;

    /// <summary>
    /// Registers of function parameters
    /// </summary>
    public readonly (nint, SsaType)[]? Arguments;

    /// <summary>
    /// The type of the operands.
    /// </summary>
    public readonly SsaType Type;

    /// <summary>
    /// The next instruction (linked list)
    /// </summary>
    public SsaInstruction? Next;

    /// <summary>
    /// The function to call, or the label to branch to.
    /// </summary>
    public readonly string? Fork;

    /// <summary>
    /// Whether to use the signed version of an instruction.
    /// </summary>
    public bool SignFlag = false;

    public SsaInstruction(SsaOpcode op, SsaType type, nint destination_register, nint source0, nint source1, (nint, SsaType)[]? args = null)
    {
        Opcode = op;
        Type = type;
        DestinationRegister = destination_register;
        Source0 = source0;
        Source1 = source1;
        Fork = null;
        Arguments = args;
    }

    public SsaInstruction(SsaOpcode op, SsaType type, nint destination_register, nint source0, nint source1, string? branch)
    {
        Opcode = op;
        Type = type;
        DestinationRegister = destination_register;
        Source0 = source0;
        Source1 = source1;
        Fork = branch;
        Arguments = null;
    }

    public SsaInstruction(SsaOpcode op, SsaType type, nint destination_register, nint source0, nint source1, string? callee, (nint, SsaType)[] args)
    {
        Opcode = op;
        Type = type;
        DestinationRegister = destination_register;
        Source0 = source0;
        Source1 = source1;
        Fork = callee;
        Arguments = args;
    }

    /// <summary>
    /// Gets the last instruction.
    /// </summary>
    /// <returns></returns>
    public SsaInstruction Last()
    {
        for (var last = this;; last = last.Next)
        {
            if (last.Next == null)
                return last;
        }
    }

    /// <summary>
    /// Adds an instruction to the linked list.
    /// </summary>
    public void AddInstruction(SsaInstruction instruction) => Last().Next = instruction;

    /// <summary>
    /// Inserts an instruction (or many) in between the current next instruction
    /// and this instruction.
    /// </summary>
    /// <param name="instruction">The instruction(s) to add.</param>
    public void InsertInstruction(SsaInstruction instruction)
    {
        if (Next != null)
            instruction.Last().AddInstruction(Next);
        Next = instruction;
    }

    public static bool operator <(SsaInstruction lhs, SsaInstruction rhs)  => lhs.HasNext(rhs);
    public static bool operator <=(SsaInstruction lhs, SsaInstruction rhs) => lhs == rhs || lhs.HasNext(rhs);
    public static bool operator >(SsaInstruction lhs, SsaInstruction rhs)  => rhs.HasNext(lhs);
    public static bool operator >=(SsaInstruction lhs, SsaInstruction rhs) => rhs == lhs || rhs.HasNext(lhs);

    public static int operator -(SsaInstruction lhs, SsaInstruction rhs)
    {
        if (ReferenceEquals(lhs, rhs))
            return 0;

        if (rhs.HasNext(lhs))
            return rhs - lhs; // reverse operations

        var i = 0;
        for (var current_instruction = lhs;
            current_instruction != rhs && current_instruction != null;
            current_instruction = current_instruction.Next, i++)
            ;

        return i;
    }

    /// <summary>
    /// Whether the instruction has <paramref name="instruction"/> as one of its children.
    /// </summary>
    public bool HasNext(SsaInstruction instruction)
    {
        if (ReferenceEquals(Next, instruction))
            return true;

        if (Next is null)
            return false;

        for (
            var current_instruction = this;
            current_instruction != null;
            current_instruction = current_instruction.Next)

            if (current_instruction == instruction)
                return true;

        return false;
    }

    public void Print(StringBuilder sb)
    {
        sb.Append(Opcode.ToString().ToLowerInvariant());
        if (Type != SsaType.None)
        {
            if (Opcode == SsaOpcode.Cast)
                sb.Append($"<{(Type & (SsaType)0xFF).ToString().ToLowerInvariant()} to {((SsaType)((int)Type >> 8)).ToString().ToLowerInvariant()}>");
            else sb.Append($"<{Type.ToString().ToLowerInvariant()}>");
        }
        sb.Append(' ');
        if (DestinationRegister != -1) sb.Append($"%v{DestinationRegister} ");
        if (Source0 != -1) sb.Append($"<- %v{Source0} ");
        if (Source1 != -1) sb.Append($"& %v{Source1} ");
        if (Fork != null) sb.Append($">>> ${Fork} ");
        if (Arguments != null)
            sb.Append($"({Arguments.Aggregate(string.Empty, (a, b) => $"{a}, %v{b.Item1}[{b.Item2.ToString().ToLowerInvariant()}]")})");
        sb.Append('\n');
    }
}
