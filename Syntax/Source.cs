using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace ck.Syntax;

/// <summary>
/// A source file.
/// </summary>
public sealed class Source
{
    /// <summary>
    /// The MD5 instance.
    /// </summary>
    private static readonly MD5 _md5 = MD5.Create();

    /// <summary>
    /// The source code.
    /// </summary>
    public readonly string Code;

    /// <summary>
    /// Stores a list containing the index beginning of all lines
    /// in the source.
    /// </summary>
    private readonly List<int> _line_beginnings = new();

    /// <summary>
    /// The path to the source code.
    /// </summary>
    public readonly string Path;

    /// <summary>
    /// MD5 checksum for this file (used as a headerguard)
    /// </summary>
    public readonly string MD5sum;

    /// <summary>
    /// Creates and reads a source file.
    /// </summary>
    /// <param name="path">The path to the source file.</param>
    public Source(string path)
    {
        Contract.Requires(File.Exists(path));
        _line_beginnings.Add(0);
        Code = File.ReadAllText(path);
        Path = path;

        // md5 checksum
        _md5.Initialize();
        MD5sum = Encoding.UTF8.GetString(_md5.ComputeHash(Encoding.UTF8.GetBytes(Code)));
    }

    /// <summary>
    /// The width of a horizontal tab (\t.)
    /// </summary>
    public const int TabWidth = 4;

    /// <summary>
    /// Transforms linear coordinates in this given source to a 2D set of
    /// column/row coordinates. Col/row coordinates will start at (1,1).
    /// </summary>
    /// <param name="position">The linear position in the file.</param>
    /// <returns>A tuple holding the col/row information requested.</returns>
    public (int Column, int Row) LinearToColumnRowCoordinates(int position)
    {
        Debug.Assert(position < Code.Length);

        var result = (Column: 1, Row: 1);

        if (position < _line_beginnings.Last())
        {
            for (var i = _line_beginnings.Count - 1; i >= 0; i--)
            {
                var line_start = _line_beginnings[i];
                if (line_start <= position)
                    return (i + 1, position - line_start + 1);
            }
        }
        else
        {
            for (var i = 0; i < position; i++)
            {
                switch (Code[i])
                {
                    case '\n': // Line feed
                        if (_line_beginnings.Last() < i)
                            _line_beginnings.Add(i);
                        result.Column++;
                        result.Row = 1;
                        continue;
                    case '\t': // Tab (4)
                        result.Row += TabWidth - result.Row % TabWidth;
                        continue;
                    case '\f':
                        if (_line_beginnings.Last() < i)
                            _line_beginnings.Add(i);
                        result.Column++;
                        continue;
                    case '\v':
                        if (_line_beginnings.Last() < i)
                            _line_beginnings.Add(i);
                        result.Column++;
                        result.Row++;
                        continue;
                    case '\r':
                        continue;

                    default:
                        result.Row++;
                        continue;
                }
            }
        }

        return result;
    }

    /// <summary>
    /// Gets a whole line of source code.
    /// </summary>
    /// <param name="line">The line index.</param>
    /// <returns><see cref="string.Empty"/> may be returned if <paramref name="line"/> is not a valid line number.</returns>
    public string GetLine(int line)
    {
        // Black magic to generate all line beginnings
        if (line + 1 >= _line_beginnings.Count)
            LinearToColumnRowCoordinates(Code.Length - 1);

        if (line < 0 || line >= _line_beginnings.Count)
            return string.Empty;

        return Code.Substring(
            _line_beginnings[line],
            (line + 1 < _line_beginnings.Count ? _line_beginnings[line + 1] : Code.Length) - _line_beginnings[line]);
    }
}
