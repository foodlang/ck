using System.Text;
using ck.Ssa;

namespace ck.Generation;

/// <summary>
/// A generator that can generate a <see cref="SsaProgram"/>.
/// </summary>
/// <typeparam name="T">Function data structure</typeparam>
public abstract class Generator<T, U>
{
    /// <summary>
    /// The program the generator will use.
    /// </summary>
    public readonly SsaProgram Program;

    /// <summary>
    /// Creates a generator object.
    /// </summary>
    protected Generator(SsaProgram program)
    {
        Program = program;
    }

    /// <summary>
    /// Generates generator helper data.
    /// </summary>
    protected abstract void GenFuncGeneratorData(string fname, SsaFunction f, out T data);

    /// <summary>
    /// Generates a function's prelude.
    /// </summary>
    protected abstract void GenFuncPrelude(T data);

    /// <summary>
    /// Generates a function's epilogue.
    /// </summary>
    protected abstract void GenFuncEpilogue(T data);

    /// <summary>
    /// Generates an instruction.
    /// </summary>
    protected abstract void GenInstruction(T data, SsaInstruction instruction);

    /// <summary>
    /// Generates a label.
    /// </summary>
    protected abstract void GenLabel(T data, string label);

    /// <summary>
    /// Program epilogue.
    /// </summary>
    protected abstract U ProgramEpilogue(IEnumerable<T> data);

    /// <summary>
    /// Generates a function.
    /// </summary>
    private void GenFunc(string fname, SsaFunction f, List<T> datalist)
    {
        GenFuncGeneratorData(fname, f, out var data);

        GenFuncPrelude(data);

        var labels = f.Labels.ToDictionary();
        var label_remove_list = new List<string>();
        for (var current_instruction = f.Base;
             current_instruction != null;
             current_instruction = current_instruction.Next)
        {
            foreach (var label in labels)
            {
                if (label.Value != current_instruction)
                    continue;

                GenLabel(data, label.Key);
                label_remove_list.Add(label.Key);
            }
            label_remove_list.ForEach(k => labels.Remove(k));
            label_remove_list.Clear();

            // skip nop
            if (current_instruction.Opcode != SsaOpcode.Nop)
                GenInstruction(data, current_instruction);
        }

        GenFuncEpilogue(data);
        datalist.Add(data);
    }

    /// <summary>
    /// Runs the generator.
    /// </summary>
    public U Run()
    {
        var datalist = new List<T>();
        Parallel.ForEach(Program.Functions, f => GenFunc(f.Key, f.Value, datalist));
        return ProgramEpilogue(datalist);
    }
}
