using ck.Diagnostics;
using ck.Lang;
using ck.Lang.Type;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using System.Diagnostics;

namespace ck.Syntax.Parsing;

public sealed partial class Parser
{
    /// <summary>
    /// Parses a variable.
    /// </summary>
    /// <param name="parent_node">The parent node.</param>
    /// <param name="var_init">[Optional] variable initialization statement.</param>
    public bool ParseVariable(ScopeTree parent_node, out Statement? var_init, AttributeClause attributes)
    {
        var_init = null;

        var c = Current();
        //if (c.Kind != TokenKind.Let && c.Kind != TokenKind.Const && c.Kind != TokenKind.Shared)
        //    return false;
        //var access
        //    = c.Kind == TokenKind.Let ? Symbol.VariableAccessType.Regular
        //    : c.Kind == TokenKind.Const ? Symbol.VariableAccessType.Constant
        //    : Symbol.VariableAccessType.Shared;

        var ident_name = Current();
        if (ident_name.Kind != TokenKind.Identifier)
            return false;
        Position++;
        FType? T = null;
        Expression? default_expression = null;

        // Explicit type declaration
        if (Current().Kind == TokenKind.Colon)
        {
            Position++;
            T = ParseType(parent_node);
            if (Current().Kind == TokenKind.Equal)
            {
                Position++;
                default_expression = ParseExpression_Conditional(parent_node); // no assign
            }
        }
        // implicit
        else if (Current().Kind == TokenKind.ColonEqual)
        {
            ExpectAndConsume(TokenKind.ColonEqual);
            default_expression = ParseExpression_Conditional(parent_node); // no assign
        }
        else
        {
            Position--;
            return false;
        }

        if (!parent_node.DeclareSymbol(ident_name, null, attributes, T, Symbol.ArgumentRefType.Value, out var sym))
        {
            Diagnostic.Throw(ident_name.Source, ident_name.Position, DiagnosticSeverity.Error, "",
                $"duplicate definition of symbol '{(string)ident_name.Value!}'");
        }
        if (default_expression != null)
        {
            default_expression
                = new BinaryExpression(ident_name, ExpressionKind.VariableInit,
                new ChildlessExpression(ident_name, ExpressionKind.Identifier),
                default_expression);
            var_init = new Statement(parent_node, StatementKind.VariableInit, c);
            var_init.AddExpression(default_expression);
        }
        ExpectAndConsume(TokenKind.Semicolon);
        return true;
    }

    /// <summary>
    /// Attempts to parse a function.
    /// </summary>
    /// <returns>Returns <c>null</c> if no function was parsed, and no token will be consumed.</returns>
    public FunctionNode? ParseFunction(ScopeTree parent_node, bool internal_specifier, AttributeClause attributes)
    {
        FunctionNode fx;
        Symbol fsym;
        Statement body;
        var dbg_token = Current();

        Symbol DeclareFunc(Token ident_name, FType? signature)
        {
            if (!parent_node.DeclareSymbol(
                ident_name, internal_specifier ? ident_name.Source : null, attributes, signature,
                Symbol.ArgumentRefType.Value,
                out var sym))
            {
                Diagnostic.Throw(ident_name.Source, ident_name.Position, DiagnosticSeverity.Error, "",
                    $"cannot declare function '{(string)ident_name.Value!}', as a symbol in this scope already bears this name");
            }
            return sym!;
        }

        var ident_name = Current();
        if (ident_name.Kind != TokenKind.Identifier)
            return null;

        var name = (string)ident_name.Value!;
        Position++;

        var current = Current();
        if (current.Kind != TokenKind.Cube)
        {
            Position--; /* unconsume ident */
            return null;
        }
        Position++;

        var functionT = ParseType(parent_node);
        if (functionT is null || functionT.SubjugateSignature is not FunctionSubjugateSignature)
        {
            Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "",
                $"function {(string)ident_name.Value!}: expected function type");
            functionT = FType.Function(FType.I32, new(Enumerable.Empty<FunctionArgumentSignature.Arg>(), null));
        }
        var signature = (FunctionSubjugateSignature)functionT.SubjugateSignature!;

        fsym = DeclareFunc(ident_name, functionT);
        fx = new FunctionNode(parent_node, name, fsym, null);

        // declaring arguments
        var success = signature.Params.Arguments.Where(
                a => fx.DeclareSymbol(a.Identifier, null, a.Attributes, a.Type, a.Ref, out _)
                ).Count();
        if (success != signature.Params.Arguments.Count())
            Debug.Fail(null);

        current = Current();
        if (current.Kind == TokenKind.ThickArrow)
        {
            Position++;
            body = new Statement(fx, StatementKind.Expression, dbg_token);
            var expr = ParseExpression(body);
            ExpectAndConsume(TokenKind.Semicolon);
            body.AddExpression(expr);
        }
        else if (current.Kind == TokenKind.LeftCurlyBracket)
        {
            _statement_index = 0;
            body = Statement_Block(fx);
        }
        else
        {
            Diagnostic.Throw(ident_name.Source, ident_name.Position, DiagnosticSeverity.Error, "", "Expected body");
            body = new Statement(null, StatementKind.Error, dbg_token);
        }

        fx.Body = body;
        fx.AttributeClause = attributes;
        
        return fx;
    }

    /// <summary>
    /// Attempts to parse a user type.
    /// </summary>
    /// <param name="parent">The parent scope.</param>
    /// <param name="internal_specifier">Whether the internal specifier has been added before the declaration.</param>
    /// <returns><c>true</c> if success; otherwise failure.</returns>
    public bool TryParseUserType(ScopeTree parent, bool internal_specifier, AttributeClause attributes)
    {
        FClass @class;
        var kw = Current();

        if (kw.Kind != TokenKind.Class)
            return false;

        Position++;

        if (Current().Kind != TokenKind.Identifier)
        {
            Diagnostic.Throw(kw.Source, kw.Position, DiagnosticSeverity.Error, "", "expected identifier after keyword");
            Position++;
            return true;
        }
        var ident = Current();
        var user_type_name = (string)ident.Value!;
        Position++;

        ExpectAndConsume(TokenKind.Cube);

        var T = ParseType(parent);
        if (T is null)
        {
            Diagnostic.Throw(kw.Source, kw.Position, DiagnosticSeverity.Error, "", "expected type after =");
            Position++;
            if (!parent.DeclareClass(ident, internal_specifier ? ident.Source : null, attributes, FType.VOID, out @class))
            {
                Diagnostic.Throw(kw.Source, kw.Position, DiagnosticSeverity.Error, "",
                    $"duplicate class declaration '{@class.Name}' (current={@class.Class}, attempt=???)");
            }
            ExpectAndConsume(TokenKind.Semicolon);
            return true;
        }

        if (T.SubjugateSignature is UserPlaceholderSubjugateSignature u
            && u.Name == user_type_name)
        {
            Diagnostic.Throw(kw.Source, kw.Position, DiagnosticSeverity.Error, "",
                $"circular dependency of class '{user_type_name}'");
            T = FType.VOID;
        }

        ExpectAndConsume(TokenKind.Semicolon);
        if (!parent.DeclareClass(ident, internal_specifier ? ident.Source : null, attributes, T, out @class))
        {
            Diagnostic.Throw(kw.Source, kw.Position, DiagnosticSeverity.Error, "",
                $"duplicate class declaration '{@class.Name}' (current={@class.Class}, attempt={T})");
        }
        return true;

    }
}
