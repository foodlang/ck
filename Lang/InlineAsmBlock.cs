using ck.Lang.Tree.Scopes;

namespace ck.Lang;

/// <summary>
/// The data required for an inline asm block.
/// </summary>
public sealed class InlineAsmBlock
{
    /// <summary>
    /// The template.
    /// </summary>
    public string Template;

    /// <summary>
    /// The list of registers to explicitly preserve.
    /// </summary>
    public readonly List<string> PreserveList;

    /// <summary>
    /// The list of registers present in the template that
    /// don't require preserving.
    /// </summary>
    public readonly List<string> IgnoreList;

    /// <summary>
    /// The references to symbols inside of the inline asm block.
    /// </summary>
    public readonly List<Symbol> SymReferences;

    public InlineAsmBlock(string template, List<string> preserve_list, List<string> ignore_list)
    {
        Template = template;
        PreserveList = preserve_list;
        IgnoreList = ignore_list;
        SymReferences = new();
    }
}
