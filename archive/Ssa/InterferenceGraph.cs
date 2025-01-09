using ck.Util;
using System.Diagnostics;

namespace ck.Ssa;

/// <summary>
/// An interference graph that is potentially colorable by k colors. This is
/// used for register allocation.
/// </summary>
public struct InterferenceGraph
{
    /// <summary>
    /// The vertices in the graph.
    /// </summary>
    private List<Variable> _vertices;

    /// <summary>
    /// Cache storing the neighbors of the vertices in the same order.
    /// </summary>
    private List<int> _neighbors_cache;

    /// <summary>
    /// The edges in the graph.
    /// </summary>
    private List<(int A, int B)>? _edges = null;

    /// <summary>
    /// The edges in the graph.
    /// </summary>
    public IEnumerable<(int A, int B)> Edges => _edges!;

    /// <summary>
    /// Creates the interference graph object. Does not build the graph.
    /// </summary>
    /// <param name="vertices">An enumerable enumerating all of the vertices. Copied into a list.</param>
    public InterferenceGraph(IEnumerable<Variable> vertices)
    {
        var vcount = vertices.Count();
        _vertices = vertices.ToList(); // copy
        _neighbors_cache = new List<int>(vcount);
        _neighbors_cache.AddRange(Enumerable.Repeat(0, vcount));
    }

    /// <summary>
    /// Builds the tree. Cannot be done twice.
    /// </summary>
    public void Build()
    {
        Debug.Assert(_edges == null);

        var edges = new HashSet<(int, int)>();

        for (var a = 0; a < _vertices.Count; a++)
        {
            var neighbors = 0;
            for (var b = 0; b < _vertices.Count; b++)
            {
                if (a == b || edges.Contains((b, a)) || !_vertices[a].Overlap(_vertices[b]))
                    continue;

                if (edges.Add((a, b)))
                    neighbors++;
            }
            _neighbors_cache[a] = neighbors;
        }

        _edges = edges.ToList();
    }

    private void _compute_neighbors_cache()
    {
        for (var a = 0; a < _vertices.Count; a++)
        {
            var neighbors = 0;
            foreach (var edge in _edges!)
            {
                if (edge.A == a || edge.B == a)
                    neighbors++;
            }
            _neighbors_cache[a] = neighbors;
        }
    }

    public void RemoveVertex(Variable v)
    {
        var a = _vertices.IndexOf(v);
        _vertices.RemoveAt(a);
        _neighbors_cache.RemoveAt(a);
        _edges!.RemoveAll(e => e.A == a || e.B == a);

        _compute_neighbors_cache();
    }

    public void NaiveColor(int k)
    {
        Span<int> neighbors = stackalloc int[k];
        neighbors.Fill(-1);
        for (var a = 0; a < _vertices.Count; a++)
        {
            var n_index = 0;
            for (var b = 0; b < _vertices.Count; b++)
            {
                if (a == b || !_vertices[a].Overlap(_vertices[b]))
                    continue;
                neighbors[n_index++] = _vertices[b].Color;
            }

            // if no neighbors, early exit and use first color
            if (n_index == 0)
            {
                _vertices[a].Color = 0;
                continue;
            }

            var lowest = 0;
            for (; neighbors.Contains(lowest); lowest++)
                ;

            _vertices[a].Color = lowest;
            Debug.Assert(lowest < k);
        }
    }

    public int Neighbors(Variable v) => _neighbors_cache[_vertices.IndexOf(v)];
}
