using ck.Lang.Tree.Scopes;
using ck.Util;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Ssa;

/// <summary>
/// A function for the SSA IL code.
/// </summary>
public sealed class SsaFunction
{
    /// <summary>
    /// The first instruction of the function.
    /// </summary>
    public readonly SsaInstruction Base;

    /// <summary>
    /// The finally block.
    /// </summary>
    public SsaInstruction? Finally;

    /// <summary>
    /// Table for auto-stored variables.
    /// </summary>
    public readonly Dictionary<string, Variable> Auto = new();

    /// <summary>
    /// The params of the function.
    /// </summary>
    public IEnumerable<Variable>? Params = null;

    /// <summary>
    /// The list of parameters passed by reference.
    /// </summary>
    public readonly List<string> RefParamList = new();

    /// <summary>
    /// All the variables (including temps)
    /// </summary>
    public readonly List<Variable> AllVariables = new();

    /// <summary>
    /// The auto blocks of the function.
    /// </summary>
    private readonly List<AutoBlock> _auto_blocks = new();

    /// <summary>
    /// The string literals of the function.
    /// </summary>
    private readonly List<string> _string_literals = new();

    public IEnumerable<AutoBlock> AutoBlocks => _auto_blocks;
    public IEnumerable<string> StringLiterals => _string_literals;

    /// <summary>
    /// The source ast.
    /// </summary>
    public readonly FunctionNode Source;

    /// <summary>
    /// Creates a new auto block. Does not create a new block if one already exists
    /// for that size.
    /// </summary>
    public AutoBlock NewAutoBlock(nint size)
    {
        var aligned_size = (size + (CompilerRunner.AlignmentRequired - 1)) & ~(CompilerRunner.AlignmentRequired - 1); // align

        // no duplicates
        foreach (var iterator in _auto_blocks)
            if (iterator.Size == aligned_size)
                return iterator;

        var block = new AutoBlock(_auto_blocks.LastOrDefault(new AutoBlock(0, 0)).Top(), aligned_size);
        _auto_blocks.Add(block);
        return block;
    }

    /// <summary>
    /// Creates a new literal string. Does not create a new one if an identical one is present.
    /// </summary>
    public int NewStringLiteral(string s)
    {
        if (_string_literals.Contains(s))
            return _string_literals.IndexOf(s);

        _string_literals.Add(s);
        return _string_literals.Count - 1;
    }

    /// <summary>
    /// The labels of the function.
    /// </summary>
    public readonly Dictionary<string, SsaInstruction> Labels = new();

    private int _unnamed_label_counter = 0;
    public string MakeUnnamedLabelName() => $"L{_unnamed_label_counter++}";

    public SsaFunction(FunctionNode source)
    {
        Source = source;

        // nop
        Base = new SsaInstruction(SsaOpcode.Nop, SsaType.None, -1, -1, -1);
    }

    public void Print(StringBuilder sb, string name)
    {
        sb.AppendLine($"func ${name}({Params!.Aggregate(string.Empty, (a, b) => $"{a}, {b.Type}").TrimStart(',')})");

        sb.AppendLine("\tstring_table {");
        for (var i = 0; i < StringLiterals.Count(); i++)
            sb.AppendLine($"\t\t{i} := \"{StringLiterals.ElementAt(i).Escapize()}\"");
        sb.AppendLine("\t}");

        sb.AppendLine("\tblock_table {");
        for (var i = 0; i < AutoBlocks.Count(); i++)
            sb.AppendLine($"\t\t{i} := [offset=0x{AutoBlocks.ElementAt(i).Offset.ToString("X2")},size=0x{AutoBlocks.ElementAt(i).Size.ToString("X2")}]");
        sb.AppendLine("\t}");

        // auto table
        sb.AppendLine("\tauto_table {");
        foreach (var auto in Auto)
            sb.AppendLine($"\t\t{auto.Key} := %v{auto.Value.Name}<{auto.Value.Type}>");
        sb.AppendLine("\t}");

        // code
        var label_mut = Labels.ToDictionary();
        var label_remove_list = new List<string>();
        var instruction_counter = 0;
        sb.AppendLine("\tcode {");
        for (var current_instruction = Base;
             current_instruction != null;
             current_instruction =  current_instruction.Next, instruction_counter++)
        {
            var label_found = false;
            // print labels
            foreach (var label in label_mut)
            {
                if (label.Value == current_instruction)
                {
                    label_remove_list.Add(label.Key);
                    sb.AppendLine($"\tlabel ${label.Key}");
                    label_found |= true;
                }
            }
            label_remove_list.ForEach(L => label_mut.Remove(L));
            label_remove_list.Clear();

            sb.Append("\t\t");
            current_instruction.Print(sb);
        }
        sb.AppendLine("\t}");
    }
}
