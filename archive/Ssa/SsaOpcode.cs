namespace ck.Ssa;

/// <summary>
/// The operations
/// </summary>
public enum SsaOpcode
{
    /// <summary>
    /// Does nothing.
    /// </summary>
    Nop, // impl

    /// <summary>
    /// Source register will be used as a constant
    /// </summary>
    Const, // impl

    Cast, // impl
    Copy, // impl
    Store, // impl
    Deref, // impl
    LabelRef, // impl

    /// <summary>
    /// Source register will be used as a constant (offset)
    /// </summary>
    AutoBlockAddr,

    /// <summary>
    /// Source register will be used as a constant (index)
    /// </summary>
    StringLiteralAddr,

    /// <summary>
    /// Zeroes a block of memory.
    /// </summary>
    ZeroBlock,

    /// <summary>
    /// Copies a block.
    /// </summary>
    MemCopy,

    Add, // impl
    Sub, // impl
    Mul, // impl
    Div, // impl
    Mod, // impl
    And, // impl
    Or,  // impl
    Xor, // impl
    Lsh, // impl
    Rsh, // impl
    Equal,
    NotEqual,
    Lower,
    Greater,
    LowerEqual,
    GreaterEqual,
    Negate,
    Not,
    LNot,

    EqualC, // impl

    Inc, // impl
    Dec, // impl

    AddC, // impl
    SubC, // impl
    MulC, // impl
    DivC, // impl

    Branch, // impl
    ConditionalBranch,
    ZConditionalBranch,

    Call,
    Return,

    ComputedBranch,
    ComputedCall,

    Address,
}

public static class SsaOpcode_Extensions
{
    public static bool ElementOf(this SsaOpcode op, params SsaOpcode[] array)
    {
        foreach (var o in array)
            if (o == op)
                return true;
        return false;
    }
}
