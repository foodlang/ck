using ck.Lang.Type;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Syntax;

/// <summary>
/// A token is the smallest defined unit of code.
/// It's akin to a word.
/// </summary>
public readonly struct Token
{
    /// <summary>
    /// The source code of the token.
    /// </summary>
    public readonly Source? Source;

    /// <summary>
    /// The position of the token in the file.
    /// </summary>
    public readonly int Position;

    /// <summary>
    /// The kind of the token.
    /// </summary>
    public readonly TokenKind Kind;

    /// <summary>
    /// The type of <see cref="Value"/>.
    /// </summary>
    public readonly FType? ValueType;

    /// <summary>
    /// The value of the token.
    /// </summary>
    public readonly object? Value;

    public Token(Source source, int position, TokenKind kind, FType? value_type = null, object? value = null)
    {
        Source = source;
        Position = position;
        Kind = kind;
        Value = value;
        ValueType = value_type;
    }

    public Token(int position, TokenKind kind, FType? value_type = null, object? value = null)
    {
        Source = null;
        Position = position;
        Kind = kind;
        Value = value;
        ValueType = value_type;
    }

    public override string ToString() => $"{Position}:{Kind}[{Value}]";
}
