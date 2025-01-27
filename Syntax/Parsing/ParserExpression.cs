using ck.Diagnostics;
using ck.Lang.Type;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using System.Diagnostics.Contracts;
using System.Reflection;
using System.Net.Security;

namespace ck.Syntax.Parsing;

public sealed partial class Parser
{
    /// <summary>
    /// Binary operator precedence table
    /// </summary>
    private static readonly Dictionary<TokenKind, (int Precedence, ExpressionKind Kind)> _binary_precedence = new()
    {
        { TokenKind.Star, (11, ExpressionKind.Multiply) },
        { TokenKind.Slash, (11, ExpressionKind.Divide) },
        { TokenKind.Percentage, (11, ExpressionKind.Modulo) },

        { TokenKind.Plus, (10, ExpressionKind.Add) },
        { TokenKind.Minus, (10, ExpressionKind.Subtract) },

        { TokenKind.LeftShift, (9, ExpressionKind.LeftShift) },
        { TokenKind.RightShift, (9, ExpressionKind.RightShift) },

        { TokenKind.Lower, (8, ExpressionKind.Lower) },
        { TokenKind.LowerEqual, (8, ExpressionKind.LowerEqual) },
        { TokenKind.Greater, (8, ExpressionKind.Greater) },
        { TokenKind.GreaterEqual, (8, ExpressionKind.GreaterEqual) },

        { TokenKind.EqualDot, (7, ExpressionKind.MemberOf) },
        { TokenKind.EqualDotDot, (7, ExpressionKind.AllMembersOf) },
        { TokenKind.EqualQuestionDot, (7, ExpressionKind.AnyMembersOf) },

        { TokenKind.ExclamationEqual, (6, ExpressionKind.NotEqual) },
        { TokenKind.EqualEqual, (6, ExpressionKind.Equal) },

        { TokenKind.And, (5, ExpressionKind.BitwiseAnd) },

        { TokenKind.Caret, (4, ExpressionKind.BitwiseXOr) },

        { TokenKind.Pipe, (3, ExpressionKind.BitwiseOr) },

        { TokenKind.AndAnd, (2, ExpressionKind.LogicalAnd) },

        { TokenKind.PipePipe, (1, ExpressionKind.LogicalOr) },
    };

    /// <summary>
    /// Gets the precedence of a given operator (using <see cref="TokenKind"/>.)
    /// </summary>
    private int GetBinaryPrecedence(TokenKind kind)
        => _binary_precedence.GetValueOrDefault(kind, (0, ExpressionKind.Error)).Item1;

    /// <summary>
    /// Gets the expression kind for a given <see cref="TokenKind"/>.
    /// </summary>
    private ExpressionKind GetBinaryExpressionKind(TokenKind kind)
        => _binary_precedence.GetValueOrDefault(kind, (0, ExpressionKind.Error)).Item2;

    private IEnumerable<(string Name, Expression E)> ParseInitializerList(ScopeTree? parent)
    {
        var initializer_list = new List<(string, Expression)>();
        ExpectAndConsume(TokenKind.LeftCurlyBracket);
        while (true)
        {
            if (Current().Kind == TokenKind.RightCurlyBracket)
                break;
            if (Current().Kind != TokenKind.Identifier)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                    "expected member name in initializer list");
                Position++;
                continue;
            }
            var member_name = Current();
            Position++;
            ExpectAndConsume(TokenKind.Colon);
            var expr = ParseExpression_Conditional(parent);
            initializer_list.Add(((string)member_name.Value!, expr));

            if (Current().Kind == TokenKind.Comma)
            {
                Position++;
                continue;
            }
            if (Current().Kind == TokenKind.RightCurlyBracket)
                break;
            Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                "expected comma , or right curly bracket }");
        }
        Position++; // }
        return initializer_list.ToArray();
    }

    /// <summary>
    /// Parses a primary expression.
    /// </summary>
    private Expression ParseExpression_Primary(ScopeTree? parent)
    {
        var current = Current();

        switch (current.Kind)
        {
        case TokenKind.NumberLiteral:
            Position++;
            return new ChildlessExpression(current, ExpressionKind.Literal) { ConstExpr = true };

        case TokenKind.StringLiteral:
            Position++;
            // concat (e.g. "Hello, " "World!" -> "Hello, World!"
            while (Current().Kind == TokenKind.StringLiteral)
            {
                current =
                    new Token(current.Position, current.Kind, FType.STRING, (string)current.Value! + (string)Current().Value!);
                Position++;
            }
            return new ChildlessExpression(current, ExpressionKind.Literal);

        // collection initialization
        case TokenKind.LeftSquareBracket:
        {
            if (parent == null)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                    "invalid context");
                return new ChildlessExpression(current, ExpressionKind.Error);
            }
            var values = new List<Expression>();
            Position++;
            while (true)
            {
                var initializer = ParseExpression_Conditional(parent);
                values.Add(initializer);
                if (Current().Kind == TokenKind.Comma)
                {
                    ExpectAndConsume(TokenKind.Comma);
                    continue;
                }

                if (Current().Kind == TokenKind.Terminator || Current().Kind == TokenKind.RightSquareBracket)
                {
                    ExpectAndConsumeAny(TokenKind.Terminator, TokenKind.RightSquareBracket);
                    break;
                }
            }
            return new AggregateExpression(current, ExpressionKind.CollectionInitialization, values);
        }

        case TokenKind.Identifier:
            Position++;
            return new ChildlessExpression(current, ExpressionKind.Identifier) { ConstExpr = false };

        case TokenKind.True:
            Position++;
            return new ChildlessExpression(new Token(current.Source!, current.Position, TokenKind.True, FType.BOOL, (decimal)1), ExpressionKind.Literal) { ConstExpr = true };

        case TokenKind.False:
            Position++;
            return new ChildlessExpression(new Token(current.Source!, current.Position, TokenKind.False, FType.BOOL, (decimal)0), ExpressionKind.Literal) { ConstExpr = true };

        case TokenKind.Null:
            Position++;
            return new ChildlessExpression(new Token(current.Source!, current.Position, TokenKind.Null, FType.VPTR, (decimal)0), ExpressionKind.Literal) { ConstExpr = true };

        case TokenKind.Default:
        {
            Position++;
            ExpectAndConsume(TokenKind.LeftBracket);
            var t = ParseType(parent);
            ExpectAndConsume(TokenKind.RightBracket);
            return new ChildlessExpression(current, ExpressionKind.Default) { Type = t };
        }

        case TokenKind.SizeOf:
        {
            Position++;
            ExpectAndConsume(TokenKind.LeftBracket);
            var t = ParseType(parent);
            ExpectAndConsume(TokenKind.RightBracket);
            // sizeof() gets computed at semantic analysis
            return new ChildlessExpression(current, ExpressionKind.SizeOf) { Type = t/*, ConstExpr = true*/ };
        }

        case TokenKind.LengthOf:
        {
            if (parent == null)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                    "invalid context");
                return new ChildlessExpression(current, ExpressionKind.Error);
            }
            Position++;
            ExpectAndConsume(TokenKind.LeftBracket);
            var e = ParseExpression_Conditional(parent);
            ExpectAndConsume(TokenKind.RightBracket);
            // lengthof() may be computed at semantic analysis
            return new UnaryExpression(current, ExpressionKind.LengthOf, e) { Type = FType.USZ };
        }

        case TokenKind.TypeOf: // TODO
            throw new NotImplementedException();

        case TokenKind.New:
        {
            if (parent == null)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                    "invalid context");
                return new ChildlessExpression(current, ExpressionKind.Error);
            }
            Position++;
            var T = ParseType(parent);
            if (Current().Kind != TokenKind.LeftCurlyBracket)
                return new AggregateExpression(current, ExpressionKind.New, Enumerable.Empty<Expression>()) { Type = T };

            var initializer_list = ParseInitializerList(parent);
            return new AggregateExpression(current, ExpressionKind.New,
                initializer_list.Select(x => x.Item2) /* expressions */,
                initializer_list.Select(x => x.Item1) /*    names    */)
            {
                Type = T
            };
        }

        case TokenKind.NameOf:
            throw new NotImplementedException();

        case TokenKind.LeftBracket:
        {
            Position++;
            var expr = ParseExpression_Conditional(parent);
            ExpectAndConsume(TokenKind.RightBracket);
            return expr;
        }

        case TokenKind.Cast:
        {
            Position++;
            ExpectAndConsume(TokenKind.LeftBracket);
            var t = ParseType(parent);
            if (t is null)
                Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", "expected resulting type being specified");
            ExpectAndConsume(TokenKind.RightBracket);
            var expr = ParseExpression_Postfix(parent);
            return new UnaryExpression(current, ExpressionKind.Cast, expr) { Type = t };
        }

        default:
            Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", "expected expression");
            return new ChildlessExpression(current, ExpressionKind.Error);
        }
    }

    /// <summary>
    /// Postfix op expression parsing
    /// </summary>
    private Expression ParseExpression_Postfix(ScopeTree? parent)
    {
        var acc = ParseExpression_Primary(parent);

        while (true)
        {
            var current = Current();

            switch (current.Kind)
            {
            case TokenKind.PlusPlus:
            case TokenKind.MinusMinus:
                Position++;
                acc = new UnaryExpression(current, current.Kind == TokenKind.PlusPlus ? ExpressionKind.PostfixIncrement : ExpressionKind.PostfixDecrement, acc);
                continue;

                // fcall
            case TokenKind.LeftBracket:
            {
                Position++;
                var op = current;
                var f_signature = new List<Expression>
                {
                    acc
                };
                var f_reftypes = new List<Symbol.ArgumentRefType>
                {
                    Symbol.ArgumentRefType.Value
                };
                current = Current();
                if (current.Kind == TokenKind.RightBracket)
                {
                    Position++;
                    goto LSkipArgs;
                }
                while (true)
                {
                    var ref_type = Current().Kind switch
                    {
                        TokenKind.In => Symbol.ArgumentRefType.In,
                        TokenKind.Out => Symbol.ArgumentRefType.Out,
                        TokenKind.Varying => Symbol.ArgumentRefType.Varying,
                        _ => Symbol.ArgumentRefType.Value
                    };
                    if (ref_type != Symbol.ArgumentRefType.Value)
                        Position++;

                    var expr = ParseExpression_Conditional(parent); // assign not allowed
                    current = Current();
                    f_signature.Add(expr);
                    f_reftypes.Add(ref_type);
                    if (current.Kind == TokenKind.Comma)
                    {
                        Position++;
                        continue;
                    }
                    ExpectAndConsumeAny(TokenKind.Terminator, TokenKind.RightBracket);
                    break;
                }
            LSkipArgs:
                acc = new AggregateExpression(op, ExpressionKind.FunctionCall, f_signature, null, f_reftypes);
                continue;
            }
            case TokenKind.LeftSquareBracket:
            {
                Position++;
                var indexer = ParseExpression_Conditional(parent); // assign not allowed
                ExpectAndConsume(TokenKind.RightSquareBracket);
                acc = new BinaryExpression(current, ExpressionKind.ArraySubscript, acc, indexer);
                continue;
            }

            case TokenKind.Cube: // scope resolution/type constants
            {
                // TODO: namespaces
                if (acc.Kind != ExpressionKind.Identifier)
                {
                    Diagnostic.Throw(acc.Token.Source, acc.Token.Position, DiagnosticSeverity.Error, "",
                        "expected identifier as left-hand of scope resolution operator");
                }
                Position++;
                var default_or_ident = Current();
                if (default_or_ident.Kind != TokenKind.Identifier
                 && default_or_ident.Kind != TokenKind.Default)
                {
                    Diagnostic.Throw(default_or_ident.Source, default_or_ident.Position, DiagnosticSeverity.Error, "",
                        "expected identifier or `default` keyword as right-hand of scope resolution operator");
                    continue;
                }
                Position++;
                acc = new BinaryExpression(current, ExpressionKind.ScopeResolution, acc,
                    new ChildlessExpression(default_or_ident, ExpressionKind.Identifier) /* this is ok */);
                continue;
            }

            case TokenKind.Dot: // member access
            {
                Position++;
                var ident = Current();
                if (ident.Kind != TokenKind.Identifier)
                {
                    Diagnostic.Throw(ident.Source, ident.Position, DiagnosticSeverity.Error, "",
                        "expected identifier as right-hand of member access operator");
                    continue;
                }
                Position++;
                acc = new BinaryExpression(current, ExpressionKind.MemberAccess, acc,
                    new ChildlessExpression(ident, ExpressionKind.Identifier) /* this is ok too */);
                continue;
            }

            default:
                goto LLeave;
            }
        }

    LLeave:
        return acc;
    }

    /// <summary>
    /// Prefix op expression parsing
    /// </summary>
    private Expression ParseExpression_Prefix(ScopeTree? parent)
    {
        var current = Current();
        var ekind = current.Kind switch
        {
            TokenKind.PlusPlus => ExpressionKind.PrefixIncrement,
            TokenKind.MinusMinus => ExpressionKind.PrefixDecrement,
            TokenKind.Plus => ExpressionKind.UnaryPlus,
            TokenKind.Minus => ExpressionKind.UnaryMinus,
            TokenKind.ExclamationEqual => ExpressionKind.LogicalNot,
            TokenKind.Tilde => ExpressionKind.BitwiseNot,
            TokenKind.Star => ExpressionKind.Dereference,
            TokenKind.And => ExpressionKind.AddressOf,
            TokenKind.AndAnd => ExpressionKind.OpaqueAddressOf,
            _ => ExpressionKind.Error
        };

        if (ekind != ExpressionKind.Error)
        {
            Position++;
            return new UnaryExpression(current, ekind, ParseExpression_Prefix(parent));
        }

        return ParseExpression_Postfix(parent);
    }

    /// <summary>
    /// Binary op expression parsing
    /// </summary>
    /// <param name="parent_precedence">Previous precedence. Should be set to 0 if called by higher parsing func</param>
    private Expression ParseExpression_Binary(ScopeTree? scope_parent, int parent_precedence = 0)
    {
        var left = ParseExpression_Prefix(scope_parent);

        while (true)
        {
            var op = Current();
            var precedence = GetBinaryPrecedence(op.Kind);
            if (precedence == 0 || precedence <= parent_precedence)
                break;
            Position++;
            var right = ParseExpression_Binary(scope_parent, parent_precedence);
            left = new BinaryExpression(op, GetBinaryExpressionKind(op.Kind), left, right);
        }

        return left;
    }

    /// <summary>
    /// Ternary conditional op expression parsing
    /// </summary>
    private Expression ParseExpression_Conditional(ScopeTree? parent)
    {
        var condition = ParseExpression_Binary(parent);
        var op = Current();
        if (op.Kind != TokenKind.QuestionMark)
            return condition;
        Position++;

        var left = ParseExpression_Conditional(parent);
        ExpectAndConsume(TokenKind.Colon);
        var right = ParseExpression_Conditional(parent);
        return new TernaryExpression(op, ExpressionKind.Conditional, condition, left, right);
    }

    /// <summary>
    /// Assign op expression parsing
    /// </summary>
    private Expression ParseExpression_Assign(ScopeTree parent)
    {
        var left = ParseExpression_Conditional(parent);
        var op = Current();
        var expr_kind = op.Kind switch
        {
            TokenKind.Equal => ExpressionKind.Assign,
            TokenKind.PlusEqual => ExpressionKind.AssignSum,
            TokenKind.MinusEqual => ExpressionKind.AssignDiff,
            TokenKind.StarEqual => ExpressionKind.AssignProduct,
            TokenKind.SlashEqual => ExpressionKind.AssignQuotient,
            TokenKind.PercentageEqual => ExpressionKind.AssignRemainder,
            TokenKind.AndEqual => ExpressionKind.AssignBitwiseAnd,
            TokenKind.CaretEqual => ExpressionKind.AssignBitwiseXOr,
            TokenKind.PipeEqual => ExpressionKind.AssignBitwiseOr,
            TokenKind.LeftShiftEqual => ExpressionKind.AssignLeftShift,
            TokenKind.RightShiftEqual => ExpressionKind.AssignRightShift,
            _ => ExpressionKind.Error,
        };

        if (expr_kind == ExpressionKind.Error)
            return left;

        Position++;
        return new BinaryExpression(op, expr_kind, left, ParseExpression_Conditional(parent));
    }

    /// <summary>
    /// Parses an expression.
    /// </summary>
    public Expression ParseExpression(ScopeTree parent)
    {
        return ParseExpression_Assign(parent);
    }
}
