using ck.Diagnostics;
using ck.Lang.Tree;
using ck.Lang.Tree.Scopes;
using System.Data;

namespace ck.Syntax.Parsing;

/// <summary>
/// Parses source code into abstract syntax trees.
/// </summary>
public sealed partial class Parser
{
    /// <summary>
    /// The tokens of the program.
    /// </summary>
    private Token[] _program_tokens;

    /// <summary>
    /// The position in the token list.
    /// </summary>
    public int Position { get; private set; } = 0;

    /// <summary>
    /// Seeks a token in the token list.
    /// </summary>
    /// <param name="offset">The offset from the current position.</param>
    /// <returns>Returns a terminator token if out of bounds.</returns>
    public Token Seek(int offset = 0)
    {
        var total_position = Position + offset;
        return 
            total_position < _program_tokens.Length
            ? _program_tokens[total_position]
            : new Token(total_position, TokenKind.Terminator);
    }

    /// <summary>
    /// Returns the current token.
    /// </summary>
    /// <returns>Returns a terminator token if out of bounds.</returns>
    public Token Current() => Seek(0);

    /// <summary>
    /// Creates a new parser. You must pass the "main source" of
    /// the program.
    /// </summary>
    /// <param name="main_source">The main source file of the program.</param>
    public Parser(Source main_source)
    {
        var lexer = new Lexer(main_source);
        var tokens = new List<Token>();
        while (true)
        {
            lexer.NextToken(out var next_token);
            if (next_token.Kind == TokenKind.Terminator)
                break;
            tokens.Add(next_token);
        }
        _program_tokens = tokens.ToArray();
        _source_checksums.Add(main_source.MD5sum);

        // cklib
        var di = new DirectoryInfo("./cklib");
        if (di.Exists)
            foreach (var file in di.EnumerateFiles("*.*", SearchOption.AllDirectories))
                Include(file.FullName);
    }

    /// <summary>
    /// Expects a token, and consumes it regardless.
    /// </summary>
    /// <param name="kind">The kind of token to expect.</param>
    /// <returns>False if the token expected was not found. A diagnostic is already thrown.</returns>
    private bool ExpectAndConsume(TokenKind kind)
    {
        var current = Current();
        Position++;
        if (current.Kind != kind)
        {
            Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", $"Expected token of type `{kind}`");
            return false;
        }
        return true;
    }

    /// <summary>
    /// Expects a token, and consumes it regardless.
    /// </summary>
    /// <param name="kinds">The kinds of token to expect.</param>
    /// <returns>False if the token expected was not found. A diagnostic is already thrown.</returns>
    private bool ExpectAndConsumeAny(params TokenKind[] kinds)
    {
        var current = Current();
        Position++;
        if (!kinds.Contains(current.Kind))
        {
            Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "",
                $"Expected token to be one of these types:" +
                $"[{kinds.Select(x => x.ToString()).Aggregate((x, y) => $"{x}, {y}")}]");
            return false;
        }
        return true;
    }

    /// <summary>
    /// Parses a whole program.
    /// </summary>
    /// <returns>The program tree.</returns>
    public FreeTree ParseProgram()
    {
        var program_tree = new ScopeTree(null);

        while (true)
        {
            var current = Current();
            if (current.Kind == TokenKind.Terminator)
                break;

            var internal_specifier = false;
            if (current.Kind == TokenKind.Internal)
            {
                internal_specifier = true;
                Position++;
            }

            var attributes = ParseAttributeClause();

            if (TryParseUserType(program_tree, internal_specifier, attributes))
                // continue is necessary here for the specifiers
                continue;

            var fx = ParseFunction(program_tree, internal_specifier, attributes);
            if (fx != null)
            {
                program_tree.Add(fx);
                continue;
            }
            ParseDirective();
        }

        return program_tree;
    }
}
