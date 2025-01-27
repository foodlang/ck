using System.Text;

namespace ck.XIL;

/// <summary>
/// An X-op module.
/// </summary>
public sealed class XModule
{
    /// <summary>
    /// The name of the module.
    /// </summary>
    public readonly string Name;

    /// <summary>
    /// The methods of the module.
    /// </summary>
    private readonly HashSet<XMethod> _methods = new();

    /// <summary>
    /// Adds a method if not already present in the method table.
    /// Already performed by method constructor.
    /// </summary>
    internal void AddMethod(XMethod m)
        => _methods.Add(m);

    public XModule(string name)
        => Name = name;

    public override string ToString()
    {
        var sb = new StringBuilder();
        sb.AppendLine($"module({Name})");
        foreach (var method in _methods)
            sb.Append(method.ToString());
        sb.AppendLine("end module");
        return sb.ToString();
    }
}
