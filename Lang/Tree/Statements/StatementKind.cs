namespace ck.Lang.Tree.Statements;

/// <summary>
/// The kind of a statement.
/// </summary>
public enum StatementKind
{
    Error,

    // Basics
    Expression,
    Empty,
    Block,
    Label,
    Finally,

    /// <summary>
    /// Inline assembly
    /// </summary>
    Asm,

    /// <summary>
    /// Variable initialization. Essentially
    /// an <see cref="Expression"/> statement
    /// with extra semantic sugar.
    /// </summary>
    VariableInit,

    /// <summary>
    /// A case in a switch.
    /// </summary>
    SwitchCase,

    /// <summary>
    /// A default case in a switch.
    /// </summary>
    SwitchDefault,

    // Branching & control
    If,
    While,
    DoWhile,
    For,
    Break,
    Continue,
    Goto,
    Switch,
    Return,

    // Debugging
    Assert, // TODO
    Sponge,
}