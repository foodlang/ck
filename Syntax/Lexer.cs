using ck.Diagnostics;
using ck.Lang.Type;
using System.Numerics;
using System.Text;

namespace ck.Syntax;

/// <summary>
/// A lexer turns source code into tokens.
/// </summary>
public sealed class Lexer
{
    /// <summary>
    /// The source code the lexer is working on.
    /// </summary>
    public readonly Source Source;

    /// <summary>
    /// The current position of the lexer.
    /// </summary>
    public int Position { get; private set; } = 0;

    /// <summary>
    /// Creates a new lexer instance.
    /// </summary>
    /// <param name="src">The source file.</param>
    public Lexer(Source src)
    {
        Source = src;
    }

    /// <summary>
    /// Contains a list of keyword/token kind pairs that are used to figure out the keywords.
    /// </summary>
    private static readonly Dictionary<string, TokenKind> _keywords = new()
    {
        ["new"] = TokenKind.New,
        ["default" ] = TokenKind.Default,
        ["sizeof"] = TokenKind.SizeOf,
        ["nameof"] = TokenKind.NameOf,
        ["true"] = TokenKind.True,
        ["false"] = TokenKind.False,
        ["null"] = TokenKind.Null,
        ["typeof"] = TokenKind.TypeOf,
        ["lengthof"] = TokenKind.LengthOf,
        ["cast"] = TokenKind.Cast,

        ["void"] = TokenKind.Void,
        ["bool"] = TokenKind.Bool,
        ["i8"] = TokenKind.I8,
        ["u8"] = TokenKind.U8,
        ["i16"] = TokenKind.I16,
        ["u16"] = TokenKind.U16,
        ["f16"] = TokenKind.F16,
        ["i32"] = TokenKind.I32,
        ["u32"] = TokenKind.U32,
        ["f32"] = TokenKind.F32,
        ["i64"] = TokenKind.I64,
        ["u64"] = TokenKind.U64,
        ["f64"] = TokenKind.F64,
        ["isz"] = TokenKind.ISZ,
        ["usz"] = TokenKind.USZ,
        ["fmax"] = TokenKind.FMAX,
        ["string"] = TokenKind.String,

        ["struct"] = TokenKind.Struct,
        ["union"] = TokenKind.Union,
        ["enum"] = TokenKind.Enum,
        ["class"] = TokenKind.Class,

        ["let"] = TokenKind.Let,
        ["func"] = TokenKind.Func,
        ["varying"] = TokenKind.Varying,
        ["in"] = TokenKind.In,
        ["out"] = TokenKind.Out,
        ["internal"] = TokenKind.Internal,

        ["const"] = TokenKind.Const,
        ["atomic"] = TokenKind.Atomic,
        ["shared"] = TokenKind.Shared,

        ["import"] = TokenKind.Import,

        ["if"] = TokenKind.If,
        ["else"] = TokenKind.Else,
        ["while"] = TokenKind.While,
        ["do"] = TokenKind.Do,
        ["switch"] = TokenKind.Switch,
        ["case"] = TokenKind.Case,
        ["break"] = TokenKind.Break,
        ["continue"] = TokenKind.Continue,
        ["return"] = TokenKind.Return,
        ["for"] = TokenKind.For,
        ["goto"] = TokenKind.Goto,
        ["assert"] = TokenKind.Assert,
        ["sponge"] = TokenKind.Sponge,
        ["finally"] = TokenKind.Finally,
        [ "asm"] = TokenKind.Asm,
    };

    /// <summary>
    /// Gets the character at the current position with an
    /// optional offset.
    /// </summary>
    /// <param name="offset">The offset to the current position.</param>
    private char Seek(int offset = 0)
    {
        var total_position = Position + offset;
        return Source.Code.Length > total_position ? Source.Code[total_position] : '\0';
    }

    /// <summary>
    /// The current character.
    /// </summary>
    public char Current() => Seek();

    /// <summary>
    /// Skips the next whitespaces.
    /// </summary>
    private void SkipWhitespaces()
    {
        while (char.IsWhiteSpace(Current()))
            Position++;
    }

    /// <summary>
    /// Parses a binary integer.
    /// </summary>
    private BigInteger ParseBinaryInteger()
    {
        var accumulator = new BigInteger(0);
        var digit = Current();
        while (digit == '0' || digit == '1')
        {
            accumulator *= 2;
            accumulator += digit - '0';
            Position++;
            digit = Current();
        }
        return accumulator;
    }

    /// <summary>
    /// Parses a hexadecimal integer.
    /// </summary>
    private BigInteger ParseHexInteger()
    {
        var accumulator = new BigInteger(0);
        var digit = Current();
        while (char.IsDigit(digit) || digit >= 'a' && digit <= 'f')
        {
            accumulator *= 16;
            if (char.IsDigit(digit))
                accumulator += digit - '0';
            else accumulator += digit - 'a' + 10;
            Position++;
            digit = Current();
        }
        return accumulator;
    }

    /// <summary>
    /// Parses an octal integer.
    /// </summary>
    private BigInteger ParseOctalInteger()
    {
        var accumulator = new BigInteger(0);
        var digit = Current();
        while (digit >= '0' && digit <= '7')
        {
            accumulator *= 8;
            accumulator += digit - '0';
            Position++;
            digit = Current();
        }
        return accumulator;
    }

    /// <summary>
    /// Parses a decimal integer.
    /// </summary>
    private BigInteger ParseDecimalInteger()
    {
        var accumulator = new BigInteger(0);
        var digit = Current();
        while (char.IsDigit(digit))
        {
            accumulator *= 10;
            accumulator += digit - '0';
            Position++;
            digit = Current();
        }
        return accumulator;
    }

    /// <summary>
    /// Parses the fractional part of a fractional number
    /// and builds the final fractional number with the
    /// integer part too.
    /// </summary>
    private decimal ParseFractionalNumber(decimal integer_part)
    {
        var accumulator = integer_part;
        var scale = 1m;
        var digit = Current();
        while (char.IsDigit(digit))
        {
            scale *= 0.1m;
            accumulator += (digit - '0') * scale;
            Position++;
            digit = Current();
        }
        if (digit == 'e' || digit == 'E')
        {
            var sign = 1;
            Position++;
            if (Current() == '+')
                Position++;
            else if (Current() == '-')
            {
                sign = -1;
                Position++;
            }
            var power = sign * (double)ParseDecimalInteger();
            accumulator *= (decimal)Math.Pow(10.0F, power);
        }
        return accumulator;
    }

    /// <summary>
    /// Parses a number (integer or decimal)
    /// </summary>
    /// <param name="number">The output number.</param>
    /// <param name="is_fractional">Whether the number has a fractional part or not.</param>
    /// <returns>True if a number was successfully parsed; false if it was not.</returns>
    private bool ParseNumber(out decimal number, out FType type)
    {
        number = 0;
        type = FType.VOID;

        if (!char.IsDigit(Current()))
            return false;

        // Prefixed integers
        if (Current() == '0')
        {
            var base_char = char.ToLower(Seek(1));
            if (base_char == 'b')
            {
                Position += 2;
                number = (decimal)ParseBinaryInteger();
                goto LTypeSuffix;
            }
            else if (base_char == 'x')
            {
                Position += 2;
                number = (decimal)ParseHexInteger();
                goto LTypeSuffix;
            }
            else if (char.IsDigit(base_char))
            {
                Position++;
                number = (decimal)ParseOctalInteger();
                goto LTypeSuffix;
            }
        }

        number = (decimal)ParseDecimalInteger();
        if (Current() == '.')
        {
            Position++;
            if (!char.IsDigit(Current()))
            {
                Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "Expected fractional part after `.` in float constant");
                return true;
            }
            number = ParseFractionalNumber(number);
        }
    // fallthrough

    LTypeSuffix:
        if (Current() == '_')
        {
            Position++;
            var t_radical = char.ToLower(Current()) switch
            {
                'i' => 0,
                'u' => 1,
                'f' => 2,
                _ => 3
            };
            Position++;

            if (t_radical == 3)
            {
                Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "Unknown literal suffix type radical. Must be either i/I, u/U or f/F.");
                return true;
            }

            var t_size = 0;
            while (char.IsDigit(Current()) || Current() == 'x' || Current() == 'X')
            {
                if (Current() == 'x' || Current() == 'X')
                {
                    Position++;
                    t_size = 0;
                    break;
                }

                t_size *= 10;
                t_size += Current() - '0';
                Position++;
            }

            type = t_radical switch
            {
                0 => t_size switch
                {
                    0 => FType.ISZ,
                    8 => FType.I8,
                    16 => FType.I16,
                    32 => FType.I32,
                    64 => FType.I64,
                    _ => FType.VOID,
                },

                1 => t_size switch
                {
                    0 => FType.USZ,
                    8 => FType.U8,
                    16 => FType.U16,
                    32 => FType.U32,
                    64 => FType.U64,
                    _ => FType.VOID,
                },

                2 => t_size switch
                {
                    0 => FType.FMAX,
                    32 => FType.F32,
                    64 => FType.F64,
                    _ => FType.VOID,
                },

                _ => FType.VOID,
            };

            if (type == FType.VOID)
            {
                Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "",
                    $"Unknown literal type `{(t_radical == 0 ? 'i' : t_radical == 1 ? 'u' : t_radical == 2 ? 'f' : '?')}{t_size}`");
                return true;
            }
        }
        else
        {
            // integer
            if ((nint)number == number)
            {
                type = number switch
                {
                    < int.MaxValue and > int.MinValue => FType.I32,
                    < uint.MaxValue and > 0 => FType.U32,
                    < long.MaxValue and > long.MinValue => FType.I64,
                    _ => FType.U64,
                };
            }
            else type = FType.F64;
        }
        return true;
    }

    /// <summary>
    /// Parses either an identifier or a keyword.
    /// </summary>
    /// <param name="kind">The output kind for the token (if it is a keyword, this will be the keyword id.)</param>
    /// <param name="identifier">The identifier/keyword text.</param>
    /// <returns>True if success in parsing; false if failure.</returns>
    private bool ParseIdentifierOrKeyword(out TokenKind kind, out string identifier)
    {
        var base_position = Position;
        var c = Current();
        while (char.IsLetterOrDigit(c) || c == '_')
        {
            Position++;
            c = Current();
        }
        identifier = Source.Code.Substring(base_position, Position - base_position);
        kind = _keywords.GetValueOrDefault(identifier, TokenKind.Identifier);
        return Position - base_position != 0;
    }

    /// <summary>
    /// Parses the next escape sequence, excluding the backslash character.
    /// No unicode is supported C:
    /// </summary>
    private char ParseEscapeSequence()
    {
        BigInteger value; // for integer literals in escape sequences
        var c = Current();
        Position++;
        var result = c switch
        {
            'a' or 'A' => '\x07',
            'b' or 'B' => '\x08',
            'e' or 'E' => '\x1B',
            'f' or 'F' => '\x0C',
            'n' or 'N' => '\x0A',
            'r' or 'R' => '\x0D',
            't' or 'T' => '\x09',
            'v' or 'V' => '\x0B',
            '\\' => '\x5C',
            '\'' => '\x27',
            '\"' => '\x22',
            '?' => '\x3F',
            _ => '\0',
        };

        if (result != '\0')
            return result;

        if (c is >= '0' and <= '7')
        {
            Position--;
            value = ParseOctalInteger();
            if (value > byte.MaxValue)
            {
                Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", $"Octal literal escape sequence overflow (max=255,x={value})");
                return (char)0x00;
            }
            return (char)value;
        }
        else if (c == 'x')
        {
            Position--;
            value = ParseHexInteger();
            if (value > byte.MaxValue)
            {
                Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", $"Hex literal escape sequence overflow (max=255,x={value})");
                return (char)0x00;
            }
            return (char)value;
        }

        Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", $"Unknown escape sequence `\\{c}`");
        Position--;
        return (char)0x00;
    }

    /// <summary>
    /// Parses a character literal.
    /// </summary>
    /// <param name="value">The output integer value.</param>
    private bool ParseCharacterLiteral(out BigInteger value)
    {
        var shift = 0;
        value = 0;

        if (Current() != '\'')
            return false;
        Position++;
        var c = Current();
        while (c != '\'' && c != '\0')
        {
            var cval = (BigInteger)c;
            Position++;
            if (c == '\\')
                cval = ParseEscapeSequence();
            value |= cval << shift++;
            c = Current();
        }
        if (Current() == '\0')
        {
            Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "Unexpected end of file in character literal");
            return true;
        }
        Position++;

        return true;
    }

    /// <summary>
    /// Parses a string literal.
    /// </summary>
    /// <param name="value">The output string value.</param>
    private bool ParseStringLiteral(out string value)
    {
        value = string.Empty;
        if (Current() != '\"')
            return false;
        Position++;
        var sb = new StringBuilder(16);
        var c = Current();
        while (c != '\"' && c != '\0')
        {
            var cval = c;
            Position++;
            if (c == '\\')
                cval = ParseEscapeSequence();
            sb.Append(cval);
            c = Current();
        }
        if (Current() == '\n' || Current() == '\r')
        {
            Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "unexpected line feed/carriage return in string literal");
            return true;
        }
        if (Current() == '\0')
        {
            Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "unexpected end of file in string literal");
            return true;
        }

        value = sb.ToString();

        Position++;
        return true;
    }

    /// <summary>
    /// Parses an operator.
    /// </summary>
    /// <param name="kind">The output token kind.</param>
    private bool ParseOperators(out TokenKind kind)
    {
        switch (Current())
        {
            case '(': kind = TokenKind.LeftBracket; Position++; return true;
            case ')': kind = TokenKind.RightBracket; Position++; return true;
            case '[': kind = TokenKind.LeftSquareBracket; Position++; return true;
            case ']': kind = TokenKind.RightSquareBracket; Position++; return true;
            case '{': kind = TokenKind.LeftCurlyBracket; Position++; return true;
            case '}': kind = TokenKind.RightCurlyBracket; Position++; return true;
            case '~': kind = TokenKind.Tilde; Position++; return true;
            case '?': kind = TokenKind.QuestionMark; Position++; return true;
            case ';': kind = TokenKind.Semicolon; Position++; return true;
            case ',': kind = TokenKind.Comma; Position++; return true;
            case '#': kind = TokenKind.Pound; Position++; return true;

            case '+':
                Position++;
                if (Current() == '+')
                {
                    kind = TokenKind.PlusPlus;
                    Position++;
                }
                else if (Current() == '=')
                {
                    kind = TokenKind.PlusEqual;
                    Position++;
                }
                else kind = TokenKind.Plus;
                return true;

            case '&':
                Position++;
                if (Current() == '&')
                {
                    kind = TokenKind.AndAnd;
                    Position++;
                }
                else if (Current() == '=')
                {
                    kind = TokenKind.AndEqual;
                    Position++;
                }
                else kind = TokenKind.And;
                return true;

            case '|':
                Position++;
                if (Current() == '|')
                {
                    kind = TokenKind.PipePipe;
                    Position++;
                }
                else if (Current() == '=')
                {
                    kind = TokenKind.PipeEqual;
                    Position++;
                }
                else kind = TokenKind.Pipe;
                return true;

            case '-':
                Position++;
                if (Current() == '-')
                {
                    kind = TokenKind.MinusMinus;
                    Position++;
                }
                else if (Current() == '=')
                {
                    kind = TokenKind.MinusEqual;
                    Position++;
                }
                else if (Current() == '>')
                {
                    kind = TokenKind.ThinArrow;
                    Position++;
                }
                else kind = TokenKind.Minus;
                return true;

            case '=':
                Position++;
                if (Current() == '=')
                {
                    kind = TokenKind.EqualEqual;
                    Position++;
                }
                else if (Current() == '>')
                {
                    kind = TokenKind.ThickArrow;
                    Position++;
                }
                else if (Current() == '.')
                {
                    Position++;
                    if (Current() == '.')
                    {
                        kind = TokenKind.EqualDotDot;
                        Position++;
                    }
                    else kind = TokenKind.EqualDot;
                }
                else if (Current() == '?')
                {
                    Position++;
                    if (Current() == '.')
                    {
                        kind = TokenKind.EqualQuestionDot;
                        Position++;
                    }
                    else
                    {
                        kind = TokenKind.EqualDot;
                        Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "=? is not a valid token, but =?. is");
                        Position++;
                    }
                }
                else kind = TokenKind.Equal;
                return true;

            case '!':
                Position++;
                if (Current() == '=')
                {
                    kind = TokenKind.ExclamationEqual;
                    Position++;
                }
                else kind = TokenKind.ExclamationMark;
                return true;

            case '*':
                Position++;
                if (Current() == '=')
                {
                    kind = TokenKind.StarEqual;
                    Position++;
                }
                else kind = TokenKind.Star;
                return true;

            case '/':
                Position++;
                if (Current() == '=')
                {
                    kind = TokenKind.SlashEqual;
                    Position++;
                }
                else kind = TokenKind.Slash;
                return true;

            case '%':
                Position++;
                if (Current() == '=')
                {
                    kind = TokenKind.PercentageEqual;
                    Position++;
                }
                else kind = TokenKind.Percentage;
                return true;

            case '^':
                Position++;
                if (Current() == '=')
                {
                    kind = TokenKind.CaretEqual;
                    Position++;
                }
                else kind = TokenKind.Caret;
                return true;

            case ':':
                Position++;
                if (Current() == ':')
                {
                    kind = TokenKind.Cube;
                    Position++;
                }
                else if (Current() == '=')
                {
                    kind = TokenKind.ColonEqual;
                    Position++;
                }
                else kind = TokenKind.Colon;
                return true;


            case '.':
                Position++;
                if (Current() == '.')
                {
                    Position++;
                    if (Current() == '.')
                    {
                        kind = TokenKind.DotDotDot;
                        Position++;
                    }
                    else kind = TokenKind.DotDot;
                }
                else kind = TokenKind.Dot;
                return true;

            case '<':
                Position++;
                if (Current() == '<')
                {
                    Position++;
                    if (Current() == '=')
                    {
                        kind = TokenKind.LeftShiftEqual;
                        Position++;
                    }
                    else kind = TokenKind.LeftShift;
                }
                else if (Current() == '=')
                {
                    Position++;
                    kind = TokenKind.LowerEqual;
                }
                else kind = TokenKind.Lower;
                return true;

            case '>':
                Position++;
                if (Current() == '>')
                {
                    Position++;
                    if (Current() == '=')
                    {
                        kind = TokenKind.RightShiftEqual;
                        Position++;
                    }
                    else kind = TokenKind.RightShift;
                }
                else if (Current() == '=')
                {
                    Position++;
                    kind = TokenKind.GreaterEqual;
                }
                else kind = TokenKind.Greater;
                return true;

            default:
                kind = TokenKind.Unknown;
                return false;
        }
    }

    /// <summary>
    /// Constructs a token struct.
    /// </summary>
    /// <param name="token">The output token.</param>
    /// <param name="position">The position of the token.</param>
    /// <param name="kind">The kind of the token.</param>
    /// <param name="value">The value of the token.</param>
    private void ConstructToken(out Token token, int position, TokenKind kind, object? value, FType? vtype)
        => token = new(Source, position, kind, vtype, value);

    /// <summary>
    /// Gets the next token.
    /// </summary>
    /// <param name="token">The resulting token.</param>
    public void NextToken(out Token token)
    {
        SkipWhitespaces();
        var pos = Position;

        // Null check
        if (Current() == '\0')
        {
            ConstructToken(out token, pos, TokenKind.Terminator, null, null);
            return;
        }

        // C++ comment skip
        if (Current() == '/' && Seek(1) == '/')
        {
            Position += 2;
            while (Current() != '\n' && Current() != '\0')
                Position++;

            // Tail call
            NextToken(out token);
            return;
        }

        // C comment skip
        if (Current() == '/' && Seek(1) == '*')
        {
            Position++;
            while (!(Current() == '*' && Seek(1) == '/') && Current() != '\0')
                Position++;
            Position += 2; // skip '*/'
            if (Current() == '\0')
                Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", "Unterminated C-style comment");

            // Tail call
            NextToken(out token);
            return;
        }

        // Check for numbers
        if (ParseNumber(out var number, out var ntype))
        {
            ConstructToken(out token, pos, TokenKind.NumberLiteral, number, ntype);
            return;
        }

        // Check for keywords or identifiers
        if (ParseIdentifierOrKeyword(out var kind, out var identifier))
        {
            ConstructToken(out token, pos, kind, identifier, null);
            return;
        }

        // Check for character literal
        if (ParseCharacterLiteral(out var big_int_value))
        {
            ConstructToken(out token, pos, TokenKind.NumberLiteral, (decimal)big_int_value, FType.I8);
            return;
        }

        // Check for string literal
        if (ParseStringLiteral(out var strvalue))
        {
            ConstructToken(out token, pos, TokenKind.StringLiteral, strvalue, FType.STRING);
            return;
        }

        // Check for operator kind
        if (ParseOperators(out var op_kind))
        {
            ConstructToken(out token, pos, op_kind, null, null);
            return;
        }

        Diagnostic.Throw(Source, Position, DiagnosticSeverity.Error, "", $"unknown token '{Current()}'");
        ConstructToken(out token, pos, TokenKind.Unknown, null, null);
        return;
    }

    /// <summary>
    /// Gets all the tokens from the lexer.
    /// </summary>
    public Token[] GetAllTokens()
    {
        Position = 0;
        var tokens = new List<Token>();
        while (true)
        {
            NextToken(out var token);
            if (token.Kind == TokenKind.Terminator)
                break;
            tokens.Add(token);
        }
        return tokens.ToArray();
    }
}
