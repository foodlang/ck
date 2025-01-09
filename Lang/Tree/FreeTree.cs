using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree;

/// <summary>
/// A tree that can hold as many members as wished, dynamically.
/// </summary>
public class FreeTree : SyntaxTree, ICollection<SyntaxTree>, IEnumerable<SyntaxTree>
{
    /// <summary>
    /// The list of children.
    /// </summary>
    private readonly HashSet<SyntaxTree> _children = new();

    public int Count => ((ICollection<SyntaxTree>)_children).Count;
    public bool IsReadOnly => ((ICollection<SyntaxTree>)_children).IsReadOnly;
    public override IEnumerable<SyntaxTree> GetChildren() => _children;
    public void Add(SyntaxTree item) => ((ICollection<SyntaxTree>)_children).Add(item);
    public void Clear() => ((ICollection<SyntaxTree>)_children).Clear();
    public bool Contains(SyntaxTree item) => ((ICollection<SyntaxTree>)_children).Contains(item);
    public void CopyTo(SyntaxTree[] array, int arrayIndex) => ((ICollection<SyntaxTree>)_children).CopyTo(array, arrayIndex);
    public bool Remove(SyntaxTree item) => ((ICollection<SyntaxTree>)_children).Remove(item);
    public IEnumerator<SyntaxTree> GetEnumerator() => ((IEnumerable<SyntaxTree>)_children).GetEnumerator();
    IEnumerator IEnumerable.GetEnumerator() => ((IEnumerable)_children).GetEnumerator();

    public FreeTree(SyntaxTree? parent)
    {
        Parent = parent;
    }
}
