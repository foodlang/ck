using ck.Diagnostics;
using ck.Lang;
using ck.Lang.Tree;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using ck.Util;
using System.Net.Http.Headers;
using System.Text;

namespace ck.Syntax.Parsing;

public sealed partial class Parser
{
    private int _statement_index = 0;

    /// <summary>
    /// Parses a block statement.
    /// </summary>
    private Statement Statement_Block(SyntaxTree? parent)
    {
        var statement_block = new Statement(parent, StatementKind.Block, Current());
        ExpectAndConsume(TokenKind.LeftCurlyBracket);
        while (true)
        {
            var c = Current();
            if (c.Kind == TokenKind.RightCurlyBracket || c.Kind == TokenKind.Terminator)
            {
                Position++;
                break;
            }

        LAcceptLabelOrStmt:

            // case labels
            if (c.Kind == TokenKind.Case)
            {
                if (!statement_block.CaseAllowed)
                {
                    Diagnostic.Throw(c.Source, c.Position, DiagnosticSeverity.Error, "",
                        "case label not allowed outside of a switch");
                    Position++;
                    continue;
                }
                Position++;
                var constexpr = ParseExpression_Conditional(null);
                ExpectAndConsume(TokenKind.Colon);
                var case_stmt = new Statement(statement_block, StatementKind.SwitchCase, c);
                case_stmt.AddExpression(constexpr);
                statement_block.AddStatement(case_stmt);
                c = Current();
                goto LAcceptLabelOrStmt;
            }
            
            if (c.Kind == TokenKind.Default)
            {
                if (!statement_block.CaseAllowed)
                {
                    Diagnostic.Throw(c.Source, c.Position, DiagnosticSeverity.Error, "",
                        "default label not allowed outside of a switch");
                    Position++;
                    continue;
                }
                Position++;
                ExpectAndConsume(TokenKind.Colon);
                var default_stmt = new Statement(statement_block, StatementKind.SwitchDefault, c);
                statement_block.AddStatement(default_stmt);
                c = Current();
                goto LAcceptLabelOrStmt;
            }

            // a label
            if (c.Kind == TokenKind.Pound)
            {
                Position++;
                var identifier = Current();
                var label_name = (string)identifier.Value!;
                Position++;
                c = Current();
                if (c.Kind == TokenKind.Colon)
                {
                    Position++;
                    var fx = statement_block.FindNearestFunction()!;
                    if (fx.IsLabelDeclared(label_name))
                    {
                        Diagnostic.Throw(identifier.Source, identifier.Position, DiagnosticSeverity.Error, "",
                            $"duplicate definition of label '{label_name}'");
                    }
                    else
                    {
                        fx.Labels.Add(new(label_name, _statement_index));
                        var label_stmt = new Statement(statement_block, StatementKind.Label, identifier);
                        label_stmt.Objects.Add(label_name);
                        statement_block.AddStatement(label_stmt);
                    }
                    c = Current();
                    goto LAcceptLabelOrStmt;
                }
                Diagnostic.Throw(identifier.Source, identifier.Position, DiagnosticSeverity.Error, "",
                    $"expected label after pound symbol #");
            }

            statement_block.AddStatement(ParseStatement(statement_block));
        }
        return statement_block;
    }
    
    /// <summary>
    /// Parses an if statement.
    /// </summary>
    private Statement Statement_If(SyntaxTree? parent)
    {
        var if_stmt = new Statement(parent, StatementKind.If, Current());
        ExpectAndConsume(TokenKind.If);

        // Condition
        ExpectAndConsume(TokenKind.LeftBracket);
        if_stmt.AddExpression(ParseExpression_Conditional(parent!.FindScopeTree()!));
        ExpectAndConsume(TokenKind.RightBracket);

        // Then
        if_stmt.AddStatement(ParseStatement(if_stmt));

        // Else
        var c = Current();
        if (c.Kind == TokenKind.Else)
        {
            Position++;
            if_stmt.AddStatement(ParseStatement(if_stmt));
        }
        return if_stmt;
    }

    /// <summary>
    /// Parses a while statement.
    /// </summary>
    private Statement Statement_While(SyntaxTree? parent)
    {
        var while_stmt = new Statement(parent, StatementKind.While, Current());
        while_stmt.BreakAllowed = true;
        while_stmt.ContinueAllowed = true;
        ExpectAndConsume(TokenKind.While);

        // Condition
        ExpectAndConsume(TokenKind.LeftBracket);
        while_stmt.AddExpression(ParseExpression_Conditional(parent!.FindScopeTree()!));
        ExpectAndConsume(TokenKind.RightBracket);

        // Then
        while_stmt.AddStatement(ParseStatement(while_stmt));
        return while_stmt;
    }

    /// <summary>
    /// Parses a do/while statement.
    /// </summary>
    private Statement Statement_DoWhile(SyntaxTree? parent)
    {
        var do_while_stmt = new Statement(parent, StatementKind.DoWhile, Current());
        do_while_stmt.BreakAllowed = true;
        do_while_stmt.ContinueAllowed = true;
        ExpectAndConsume(TokenKind.Do);

        do_while_stmt.AddStatement(ParseStatement(do_while_stmt));

        // Condition
        ExpectAndConsume(TokenKind.While);
        ExpectAndConsume(TokenKind.LeftBracket);
        do_while_stmt.AddExpression(ParseExpression_Conditional(parent!.FindScopeTree()!));
        ExpectAndConsume(TokenKind.RightBracket);
        ExpectAndConsume(TokenKind.Semicolon);
        return do_while_stmt;
    }

    private Statement Statement_For(SyntaxTree? parent)
    {
        var for_stmt = new Statement(parent, StatementKind.For, Current());
        for_stmt.BreakAllowed = true;
        for_stmt.ContinueAllowed = true;
        ExpectAndConsume(TokenKind.For);

        // clause
        ExpectAndConsume(TokenKind.LeftBracket);

        // initialize
        if (Current().Kind == TokenKind.Semicolon)
        {
            ExpectAndConsume(TokenKind.Semicolon);
            goto LSkipInit;
        }
        var attributes = ParseAttributeClause();
        if (!ParseVariable(for_stmt, out var initialize_stmt, attributes))
            initialize_stmt = Statement_Expression(for_stmt);
        for_stmt.AddStatement(initialize_stmt!);
    LSkipInit:

        // condition
        if (Current().Kind == TokenKind.Semicolon)
            goto LSkipCondition;
        var cond = ParseExpression(for_stmt);
        cond.Decorations.Add("for-condition");
        for_stmt.AddExpression(cond);
    LSkipCondition:
        ExpectAndConsume(TokenKind.Semicolon);

        // iterate
        if (Current().Kind == TokenKind.RightBracket) goto LSkipIterate;
        var iteration = ParseExpression(for_stmt);
        iteration.Decorations.Add("for-iteration");
        for_stmt.AddExpression(iteration);

    // end clause
    LSkipIterate:
        ExpectAndConsume(TokenKind.RightBracket);

        // body
        for_stmt.AddStatement(ParseStatement(for_stmt));
        return for_stmt;
    }

    /// <summary>
    /// Parses a break statement.
    /// </summary>
    private Statement Statement_Break(SyntaxTree? parent)
    {
        var c = Current();
        ExpectAndConsume(TokenKind.Break);
        ExpectAndConsume(TokenKind.Semicolon);
        if (!(parent is Statement && ((Statement)parent).BreakAllowed))
            Diagnostic.Throw(c.Source, c.Position, DiagnosticSeverity.Error, "", "`break` is not allowed in the current context");
        return new Statement(parent, StatementKind.Break, Current());
    }

    /// <summary>
    /// Parses a continue statement.
    /// </summary>
    private Statement Statement_Continue(SyntaxTree? parent)
    {
        var c = Current();
        ExpectAndConsume(TokenKind.Continue);
        ExpectAndConsume(TokenKind.Semicolon);
        if (!(parent is Statement && ((Statement)parent).ContinueAllowed))
            Diagnostic.Throw(c.Source, c.Position, DiagnosticSeverity.Error, "", "`continue` is not allowed in the current context");
        return new Statement(parent, StatementKind.Continue, Current());
    }

    /// <summary>
    /// Parses a return statement.
    /// </summary>
    private Statement Statement_Return(SyntaxTree? parent)
    {
        var return_statement = new Statement(parent, StatementKind.Return, Current());
        ExpectAndConsume(TokenKind.Return);
        if (Current().Kind == TokenKind.Semicolon)
            ExpectAndConsume(TokenKind.Semicolon);
        else
        {
            return_statement.AddExpression(ParseExpression_Conditional(parent!.FindScopeTree()!));
            ExpectAndConsume(TokenKind.Semicolon);
        }
        return return_statement;
    }

    private Statement Statement_Empty(SyntaxTree? parent) { Position++; return new Statement(parent, StatementKind.Empty, Current()); }
    private Statement Statement_Expression(SyntaxTree? parent)
    {
        var stmt = new Statement(parent, StatementKind.Expression, Current());
        stmt.AddExpression(ParseExpression(parent!.FindScopeTree()!));
        ExpectAndConsume(TokenKind.Semicolon);
        return stmt;
    }

    private Statement Statement_Goto(SyntaxTree? parent)
    {
        var stmt = new Statement(parent, StatementKind.Goto, Current());
        ExpectAndConsume(TokenKind.Goto);
        // computed goto
        if (Current().Kind == TokenKind.LeftBracket)
        {
            Position++;
            stmt.AddExpression(ParseExpression_Conditional(parent!.FindScopeTree()!));
            ExpectAndConsume(TokenKind.RightBracket);
        }
        else
        {
            var identifier = Current();
            var label_name = (string)identifier.Value!;
            Position++;
            if (identifier.Kind != TokenKind.Identifier)
            {
                Diagnostic.Throw(identifier.Source, identifier.Position, DiagnosticSeverity.Error, "",
                    "expected label name in non-conditional goto statement");
                return new Statement(parent, StatementKind.Error, Current());
            }
            stmt.Objects.Add(label_name);
        }
        ExpectAndConsume(TokenKind.Semicolon);
        return stmt;
    }

    private Statement Statement_Switch(SyntaxTree? parent)
    {
        var stmt = new Statement(parent, StatementKind.Switch, Current());
        stmt.CaseAllowed = true;
        stmt.BreakAllowed = true;

        ExpectAndConsume(TokenKind.Switch);

        // evaluated
        ExpectAndConsume(TokenKind.LeftBracket);
        stmt.AddExpression(ParseExpression_Conditional(parent!.FindScopeTree()!));
        ExpectAndConsume(TokenKind.RightBracket);

        // body
        stmt.AddStatement(ParseStatement(stmt));

        return stmt;
    }

    private Statement Statement_Finally(SyntaxTree? parent)
    {
        var stmt = new Statement(parent, StatementKind.Finally, Current());
        var c = Current();
        ExpectAndConsume(TokenKind.Finally);
        stmt.AddStatement(ParseStatement(stmt));
        if (parent!.FindNearestFunction()!.HasFinally)
        {
            Diagnostic.Throw(c.Source!, c.Position, DiagnosticSeverity.Error, "",
                "duplicate finally block in function");
        }
        if (parent is Statement s)
        {
            if (s.Kind != StatementKind.Block || s.Parent != s.FindNearestFunction())
                Diagnostic.Throw(c.Source!, c.Position, DiagnosticSeverity.Error, "",
                    "finally block must be in function block body");
        }
        else throw new NotImplementedException();
        parent!.FindNearestFunction()!.HasFinally = true;
        return stmt;
    }

    private Statement Statement_Asm(SyntaxTree? parent)
    {
        List<string> ReadRegisterList(Token dbg)
        {
            var list = new List<string>();
            ExpectAndConsume(TokenKind.LeftSquareBracket);
            while (true)
            {
                var tok_preserve_lstr = Current();
                Position++;

                if (tok_preserve_lstr.Kind == TokenKind.RightSquareBracket)
                    break;

                if (tok_preserve_lstr.Kind != TokenKind.StringLiteral)
                {
                    Diagnostic.Throw(dbg.Source!, dbg.Position, DiagnosticSeverity.Error, "",
                        "expected register name as string literal or closing square bracket ]");
                    if (tok_preserve_lstr.Kind == TokenKind.Terminator)
                        break;
                    continue;
                }
                list.Add((string)tok_preserve_lstr.Value!);

                var ahead = Current();
                if (ahead.Kind != TokenKind.Comma && ahead.Kind != TokenKind.RightSquareBracket)
                    Diagnostic.Throw(dbg.Source!, dbg.Position, DiagnosticSeverity.Error, "",
                        "expected comma or closing square bracket");

                if (ahead.Kind == TokenKind.Comma)
                    Position++;
            }
            return list;
        }

        // asm ![preserve_list] &[ignore_list] asm_template;

        var stmt = new Statement(parent, StatementKind.Asm, Current());
        var dbg = Current();
        ExpectAndConsume(TokenKind.Asm);

        // asm_template is a string literal
        // ![preserve_list] and &[ignore_list] are both optional
        // ignore_list is a list of registers that don't require preserve
        // both preserve_list and ignore_list are a list of literal strings
        List<string>? preserve_list = null;
        List<string>? ignore_list = null;

    LReadPreserveIgnore:
        if (Current().Kind == TokenKind.ExclamationMark)
        {
            Position++;
            if (preserve_list is not null)
                Diagnostic.Throw(dbg.Source!, dbg.Position, DiagnosticSeverity.Error, "",
                    "duplicate preserve register list");
            preserve_list = ReadRegisterList(dbg);
            goto LReadPreserveIgnore;
        }
        else if (Current().Kind == TokenKind.And)
        {
            Position++;
            if (ignore_list is not null)
                Diagnostic.Throw(dbg.Source!, dbg.Position, DiagnosticSeverity.Error, "",
                    "duplicate ignore register list");
            ignore_list = ReadRegisterList(dbg);
            goto LReadPreserveIgnore;
        }

        preserve_list ??= new List<string>();
        ignore_list ??= new List<string>();
        /* fallthrough */

        // template
        var full_template = new StringBuilder(128);
        while (true)
        {
            var tok_asm_template = Current();
            if (tok_asm_template.Kind != TokenKind.StringLiteral)
            {
                Diagnostic.Throw(dbg.Source!, dbg.Position, DiagnosticSeverity.Error, "",
                    "expected inline asm template as string literal");
            }
            Position++;
            var asm_template = tok_asm_template.Kind == TokenKind.StringLiteral ? (string)tok_asm_template.Value! : string.Empty;
            full_template.AppendLine(asm_template);

            if (Current().Kind != TokenKind.Comma)
                break;
            Position++; // comma
        }

        ExpectAndConsume(TokenKind.Semicolon);

        var collision = preserve_list.Intersect(ignore_list).Count();
        if (collision != 0)
        {
            Diagnostic.Throw(dbg.Source!, dbg.Position, DiagnosticSeverity.Error, "",
                $"{collision} register(s) in the ignore list are also specified in the preserve list");
        }

        // generating stmt
        stmt.Objects.Add(new InlineAsmBlock(full_template.ToString(), preserve_list, ignore_list));
        return stmt;
    }

    /// <summary>
    /// Parses a statement.
    /// </summary>
    public Statement ParseStatement(SyntaxTree? parent)
    {
        var scope = parent is ScopeTree s ? s : parent!.FindNearestParentScope();
        var attr_clause = ParseAttributeClause();
#if DEBUG
        if (scope == null)
            throw new Exception("null scope");
#endif
        if (ParseVariable(scope!, out var init_var, attr_clause))
        {
            _statement_index++;
            if (init_var != null)
                return init_var;
            return new Statement(parent, StatementKind.Empty, Current());
        }
        var ret = Current().Kind switch
        {
            TokenKind.Semicolon => Statement_Empty(parent),
            TokenKind.LeftCurlyBracket => Statement_Block(parent),
            TokenKind.If => Statement_If(parent),
            TokenKind.While => Statement_While(parent),
            TokenKind.Do => Statement_DoWhile(parent),
            TokenKind.Break => Statement_Break(parent),
            TokenKind.Continue => Statement_Continue(parent),
            TokenKind.Return => Statement_Return(parent),
            TokenKind.Goto => Statement_Goto(parent),
            TokenKind.For => Statement_For(parent),
            TokenKind.Switch => Statement_Switch(parent),
            TokenKind.Finally => Statement_Finally(parent),
            TokenKind.Asm => Statement_Asm(parent),
            _ => Statement_Expression(parent)
        };
        ret.AttributeClause = attr_clause;
        _statement_index++;
        return ret;
    }
}
