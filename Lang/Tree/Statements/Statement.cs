using ck.Lang.Tree;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Statements;

/// <summary>
/// A statement.
/// </summary>
public sealed class Statement : ScopeTree
{
    public readonly List<Expression> Expressions = new();
    public readonly List<Statement> Statements = new();
    public readonly List<object> Objects = new();

    /// <summary>
    /// Keyword token. Used for debugging and such.
    /// </summary>
    public readonly Token KwToken;

    private bool _break_allowed = false;
    private bool _continue_allowed = false;
    private bool _case_allowed = false;

    public bool SwitchDecorated = false;

    /// <summary>
    /// If true, the break statement is allowed to be inserted in a child statement.
    /// </summary>
    public bool BreakAllowed
    {
        get
        {
            if (_break_allowed)
                return true;
            return Parent is Statement ? ((Statement)Parent).BreakAllowed : false;
        }
        set
        {
            _break_allowed = value;
        }
    }

    /// <summary>
    /// If true, the continue statement is allowed to be inserted in a child statement.
    /// </summary>
    public bool ContinueAllowed
    {
        get
        {
            if (_continue_allowed)
                return true;
            return Parent is Statement ? ((Statement)Parent).ContinueAllowed : false;
        }
        set
        {
            _continue_allowed = value;
        }
    }

    /// <summary>
    /// If true, <c>case</c> labels are allowed to be inserted in a child statement.
    /// This is for switch statements.
    /// </summary>
    public bool CaseAllowed
    {
        get
        {
            if (_case_allowed)
                return true;
            return Parent is Statement ? ((Statement)Parent).CaseAllowed : false;
        }
        set
        {
            _case_allowed = value;
        }
    }

    public Statement? FindNearestBreakableStatement()
        => BreakAllowed
        && (Kind == StatementKind.While || Kind == StatementKind.DoWhile
        || Kind == StatementKind.For || Kind == StatementKind.Switch)
        ?
        this : Parent is Statement ps && Parent is not null ? ps.FindNearestBreakableStatement()! : null;

    public Statement? FindNearestSwitch()
        => Kind == StatementKind.Switch ? this : Parent is Statement ps && Parent is not null ? ps.FindNearestSwitch()! : null;

    public StatementKind Kind;

    public Statement(SyntaxTree? parent, StatementKind kind, Token kw_token) : base(parent)
    {
        Kind = kind;
        KwToken = kw_token;
    }

    /// <summary>
    /// The expressions stored in all substatements and this statement.
    /// </summary>
    public IEnumerable<Expression> RecursiveExpressions
        => Statements.SelectMany(s => s.RecursiveExpressions).Concat(Expressions.SelectMany(e => e.GetRecursiveChildrenExpressions()));

    public void AddExpression(Expression e)
    {
        Expressions.Add(e);
        e.Parent = this;
    }

    public void AddStatement(Statement s)
    {
        Statements.Add(s);
        s.Parent = this;
    }

    public Expression? GetDecoratedExpression(string decoration)
        => Expressions.Where(e => e.Decorations.Contains(decoration)).FirstOrDefault();

    public override IEnumerable<SyntaxTree> GetChildren()
    {
        return Expressions.Concat<SyntaxTree>(Statements);
    }

    public override string PrintElement(int indent_level = 0) => $"{Kind}";
}
