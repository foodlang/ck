namespace ck.XIL;

/// <summary>
/// An X-op method.
/// </summary>
public sealed class XMethod
{
    /// <summary>
    /// The parent module.
    /// </summary>
    public readonly XModule Module;

    /// <summary>
    /// The undecorated name of the method.
    /// </summary>
    public readonly string Name;

    /// <summary>
    /// Parameter table
    /// </summary>
    public readonly Dictionary<string, XReg> ParamTable;

    /// <summary>
    /// Local variable table
    /// </summary>
    public readonly Dictionary<string, XReg> LocalTable;

    /// <summary>
    /// Local block table
    /// </summary>
    public readonly Dictionary<string, XObjectBlock> MethodBlocks;

    /// <summary>
    /// Label table
    /// </summary>
    private readonly Dictionary<string, int> _labeltable = new(4);

    /// <summary>
    /// The internal ops.
    /// </summary>
    private readonly List<XOp> _ops = new(64);

    private readonly List<XOp> _finally_block = new(64);

    /// <summary>
    /// Temporary registers
    /// </summary>
    private readonly List<XReg> _temps = new(16);

    /// <summary>
    /// Stack memory blocks
    /// </summary>
    private readonly List<XObjectBlock> _blocks = new(6);

    public XMethod(XModule module, string name, Dictionary<string, XReg> ptable, Dictionary<string, XReg> ltable, Dictionary<string, XObjectBlock> blocks)
    {
        Module = module;
        Name = name;
        ParamTable = ptable;
        LocalTable = ltable;
        MethodBlocks = blocks;
        module.AddMethod(this);
    }

    /// <summary>
    /// Creates a temporary storage register.
    /// </summary>
    public XReg CreateTemp()
    {
        var r = new XReg(_temps.Count, XRegStorage.Fast);
        _temps.Add(r);
        return r;
    }

    /// <summary>
    /// Creates a new block.
    /// </summary>
    public XObjectBlock CreateBlock(nuint size, bool module_storage = false)
    {
        var new_off = _blocks.LastOrDefault()?.End() ?? 0;
        var blk = new XObjectBlock((nuint)_blocks.Count, new_off, size, module_storage ? XObjectBlockStorage.Method : XObjectBlockStorage.Module);
        _blocks.Add(blk);
        return blk;
    }

    /// <summary>
    /// Adds code.
    /// </summary>
    /// <param name="op">The operation to add.</param>
    public void Write(XOp op) => (_finally_writing ? _finally_block : _ops).Add(op);

    public void ToggleFinallyWrite() => _finally_writing ^= true;

    private bool _finally_writing = false;

    /// <summary>
    /// The current position in the code.
    /// </summary>
    public int CurCodePosition() => _ops.Count;

    public void MakeLabel(int offset, string name)
        => _labeltable[name] = offset;

    public int GetLabel(string name)
        => _labeltable[name];
}
