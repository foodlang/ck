using ck.Diagnostics;
using ck.Lang;
using System;
using System.Collections.Generic;
using System.Diagnostics.Metrics;
using System.Linq;
using System.Net.Http.Headers;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ck.Syntax.Parsing;

public sealed partial class Parser
{
    /// <summary>
    /// The include directories that are currently registered.
    /// </summary>
    private readonly static List<string> _include_directories = new()
    {
        Environment.CurrentDirectory,
        new FileInfo(Environment.ProcessPath!).Directory!.FullName,
        Path.Join(new FileInfo(Environment.ProcessPath!).Directory!.FullName, "lib")
    };

    /// <summary>
    /// Appends a group of include directories to the parser's include directory list.
    /// </summary>
    /// <param name="includes">The include directories.</param>
    public void AppendIncludeDirectories(string[] includes) => _include_directories.AddRange(includes.Select(Path.GetFullPath));

    /// <summary>
    /// List of checksums for the already included sources
    /// </summary>
    private readonly HashSet<string> _source_checksums = new();

    /// <summary>
    /// Includes a source file.
    /// </summary>
    /// <param name="path">The path to include.</param>
    private void Include(string path)
    {
        var imported_source = new Source(path);
        if (_source_checksums.Contains(imported_source.MD5sum))
            return;
        _source_checksums.Add(imported_source.MD5sum);
        var imported_lexer = new Lexer(imported_source);
        _program_tokens = _program_tokens.Concat(imported_lexer.GetAllTokens()).ToArray();
    }

    /// <summary>
    /// Parses an import.
    /// </summary>
    private void ParseImport()
    {

        var import_keyword = Current();
        Position++;
        var path_token = Current();
        ExpectAndConsume(TokenKind.StringLiteral);
        ExpectAndConsume(TokenKind.Semicolon);

        var short_path = (string)path_token.Value!;
        var import_directory = new FileInfo(path_token.Source!.Path).Directory!.FullName;
        var import_path = Path.Join(import_directory, short_path);
        if (File.Exists(import_path))
        {
            Include(import_path);
            return;
        }

        foreach (var include_directory in _include_directories)
        {
            import_path = Path.Join(include_directory, short_path);
            if (File.Exists(import_path))
            {
                Include(import_path);
                return;
            }
        }

        Diagnostic.Throw(import_keyword.Source, path_token.Position, DiagnosticSeverity.Error, "", $"'{short_path}' is not a valid include path");
    }

    /// <summary>
    /// Parses an attribute clause.
    /// </summary>
    private AttributeClause ParseAttributeClause()
    {
        var current = Current();
        var clause = new AttributeClause();

        if (current.Kind != TokenKind.Pound)
            return clause;
        Position++;

        ExpectAndConsume(TokenKind.LeftCurlyBracket);
        while (true)
        {
            if (Current().Kind == TokenKind.RightCurlyBracket)
            {
                Position++;
                break;
            }

            if (Current().Kind != TokenKind.Identifier)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                    "expected attribute name");
                Position++;
            }
            var attribute_name = Current();
            var attribute_args = new List<object>();
            Position++;
            if (Current().Kind == TokenKind.LeftBracket)
            {
                Position++;
                while (true)
                {
                    if (Current().Kind == TokenKind.RightBracket)
                    {
                        Position++;
                        break;
                    }

                    // should be a valid constexpr
                    var attribute_param_value = ParseExpression_Conditional(null);
                    attribute_args.Add(attribute_param_value);

                    if (Current().Kind == TokenKind.Comma)
                    {
                        Position++;
                        continue;
                    }

                    ExpectAndConsumeAny(TokenKind.RightBracket, TokenKind.Terminator);
                    break;
                }
            }

            clause.AddAttribute(attribute_name, new((string)attribute_name.Value!, attribute_args, current));
            if (Current().Kind == TokenKind.Comma)
            {
                Position++;
                continue;
            }

            ExpectAndConsumeAny(TokenKind.RightCurlyBracket, TokenKind.Terminator);
            break;
        }
        return clause;
    }

    /// <summary>
    /// Parses a directive. Directives are similar to 
    /// statements, they tell the compiler how to behave.
    /// For example, `import` is a directive.
    /// </summary>
    public void ParseDirective()
    {
        var current = Current();

        if (current.Kind == TokenKind.Import)
            ParseImport();
        else
        {
            Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", "Unknown syntax element");
            Position++;
        }
    }
}
