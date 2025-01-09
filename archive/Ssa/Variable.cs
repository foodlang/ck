namespace ck.Ssa;

/// <summary>
/// A variable in code. This is also used for temporaries.
/// </summary>
public sealed class Variable
{
    /// <summary>
    /// The name of a variable. An integer (a table is used for
    /// string names.)
    /// </summary>
    public readonly nint Name;

    /// <summary>
    /// The type of a variable.
    /// </summary>
    public readonly SsaType Type;

    /// <summary>
    /// The interval through which the variable is live.
    /// </summary>
    public LiveInterval Live;

    /// <summary>
    /// Whether local storage has to be used for this variable.
    /// This is automatically true for variables that must bear
    /// an address.
    /// </summary>
    public bool LocalStorage;

    /// <summary>
    /// The cost to spill this variable.
    /// </summary>
    public int SpillCost;

    /// <summary>
    /// Used in interference graph. <c>-1</c> means unassigned.
    /// </summary>
    public int Color;

    public Variable(nint name, SsaType type, LiveInterval live)
    {
        Name = name;
        Type = type;
        Live = live;
        LocalStorage = false;
        SpillCost = 0;
        Color = -1;
    }

    /// <summary>
    /// Checks if a variable overlaps another's lifetime.
    /// </summary>
    public bool Overlap(Variable other) => Live.Overlap(in other.Live);
}
