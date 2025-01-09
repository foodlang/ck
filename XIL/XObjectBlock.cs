namespace ck.XIL;

/// <summary>
/// A block used for storing objects such as
/// structs, arrays and other such instances.
/// </summary>
public sealed class XObjectBlock
{
    /// <summary>
    /// Offset from the segment base
    /// </summary>
    public readonly nuint Offset;

    /// <summary>
    /// The size of the block.
    /// </summary>
    public readonly nuint Size;

    /// <summary>
    /// The storage of the X object block
    /// </summary>
    public readonly XObjectBlockStorage Storage;

    /// <summary>
    /// The method holding the block
    /// </summary>
    public readonly nuint Index;

    public XObjectBlock(nuint index, nuint offset, nuint size, XObjectBlockStorage storage)
    {
        Index = index;
        Offset = offset;
        Size = size;
        Storage = storage;
    }

    /// <summary>
    /// Returns the offset of the next free byte.
    /// </summary>
    public nuint End() => Offset + Size;
}
