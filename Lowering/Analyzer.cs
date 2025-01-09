using ck.Diagnostics;
using ck.Lang.Type;
using ck.Lang.Tree;
using ck.Lang.Tree.Expressions;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Statements;
using System.Diagnostics;
using System.Text;
using ck.Lang;

namespace ck.Lowering;

/// <summary>
/// Performs semantic analysis on a program tree.
/// </summary>
public sealed partial class Analyzer
{
    /// <summary>
    /// The program to analyze.
    /// </summary>
    public readonly FreeTree Program;

    /// <summary>
    /// Parses an inline assembly template.
    /// </summary>
    /// <returns>Processed template</returns>
    private string ParseInlineAsmTemplate(Statement stmt, out int errors)
    {
        errors = 0;

        var asm = (InlineAsmBlock)stmt.Objects[0];
        string template = asm.Template;
        var preserve_list = asm.PreserveList;
        var ignore_list = asm.IgnoreList;
        var copy_ignore_list = ignore_list.ToList();

        var result = new StringBuilder(template.Length);
        var position = 0;

        char Current() => position < template.Length ? template[position] : '\0';
        char Peek(int offset = 1) => (position + offset) < template.Length ? template[position + offset] : '\0';

        while (Current() != 0)
        {
            // %% means % and $$ means $
            if (Current() == '%' && Peek() == '%'
             || Current() == '$' && Peek() == '$')
            {
                result.Append(Current());
                position += 2;
                continue;
            }

            // regname
            if (Current() == '%')
            {
                var sb_regname = new StringBuilder(4);
                position++;
                while (char.IsLetterOrDigit(Current()) || Current() == '_')
                {
                    sb_regname.Append(Current());
                    position++;
                }

                var regname = sb_regname.ToString();

                if (string.IsNullOrEmpty(regname))
                {
                    Diagnostic.Throw(stmt.KwToken.Source!, stmt.KwToken.Position, DiagnosticSeverity.Error, "",
                        $"expected register name after % operator. use %% to specify literal percentage sign %");
                    errors++;
                }

                if (ignore_list.Contains(regname))
                    copy_ignore_list.Remove(regname);

                if (!ignore_list.Contains(regname) && !preserve_list.Contains(regname))
                    preserve_list.Add(regname);
                result.Append($"%{regname}");
                continue;
            }

            // symbol or label
            if (Current() == '$')
            {
                var sb_ident = new StringBuilder(6);
                position++;
                while (char.IsLetterOrDigit(Current()) || Current() == '_')
                {
                    sb_ident.Append(Current());
                    position++;
                }

                var ident = sb_ident.ToString();

                // label
                if (stmt.FindNearestFunction()!.IsLabelDeclared(ident))
                {
                    result.Append($"${ident}");
                    continue;
                }

                // sym
                if (!stmt.GetDeclaration<Symbol>(ident, stmt.KwToken.Source!, out var sym))
                {
                    Diagnostic.Throw(stmt.KwToken.Source!, stmt.KwToken.Position, DiagnosticSeverity.Error, "",
                        $"unresolved identifier ${ident} in inline asm");
                    errors++;
                    result.Append($"${ident}");
                    continue;
                }

                sym!.Referenced = true;
                asm.SymReferences.Add(sym);

                if (sym!.RefType != Symbol.ArgumentRefType.Value)
                {
                    Diagnostic.Throw(stmt.KwToken.Source!, stmt.KwToken.Position, DiagnosticSeverity.Error, "",
                        $"cannot use ref param in inline asm block");
                    errors++;
                }
                result.Append($"${ident}");
                continue;
            }

            result.Append(Current());
            position++;
        }

        if (copy_ignore_list.Count != 0)
            Diagnostic.Throw(stmt.KwToken.Source!, stmt.KwToken.Position, DiagnosticSeverity.Warning, "unused-ignored-registers",
                $"{copy_ignore_list.Count} register(s) present in the ignore list are not present in the inline asm block");

        return result.ToString();
    }

    /// <summary>
    /// Analyzes a statement.
    /// </summary>
    /// <param name="stmt">The statement to analyze.</param>
    private int AnalyzeStatement(Statement stmt)
    {
        var summed = 0;
        var children = stmt.Expressions;
        var new_expressions = new Expression[children.Count()];
        var new_expression_index = 0;
        foreach (var child in children)
        {
            new_expressions[new_expression_index] = child;
            summed += AnalyzeExpression(ref new_expressions[new_expression_index++]);
        }
        stmt.Expressions.Clear();
        stmt.Expressions.AddRange(new_expressions);
        summed += stmt.Statements.Sum(AnalyzeStatement);

        if (summed != 0)
            return summed;

        // Statement special cases
        switch (stmt.Kind)
        {

        case StatementKind.Return:
        {
            var func = stmt.FindNearestFunction()!;
            var fx = (FunctionSubjugateSignature)func.FuncSym.Type!.SubjugateSignature!;
            if (stmt.Expressions.Count == 0) // `return;`
            {
                if (fx.ReturnType != FType.VOID)
                {
                    Diagnostic.Throw(stmt.KwToken.Source!, stmt.KwToken.Position, DiagnosticSeverity.Error, "",
                        $"void return found in non-void returning function");
                    return 1;
                }
                return 0;
            }

            // `return <expr>;`
            stmt.Expressions[0] = ImplicitConversionAvailableTU(fx.ReturnType, stmt.Expressions[0]);
            var return_expr = stmt.Expressions[0];
            if (return_expr.Type! != fx.ReturnType)
            {
                Diagnostic.Throw(return_expr.Token.Source!, return_expr.Token.Position,
                    DiagnosticSeverity.Error, "",
                    "return type mismatch between expression and function signature");
                return 1;
            }
            return 0;
        }

        case StatementKind.If:
        case StatementKind.While:
        case StatementKind.DoWhile:
        case StatementKind.For:
            if (!stmt.Expressions[0].Type!.Traits.HasFlag(TypeTraits.Integer))
            {
                Diagnostic.Throw(stmt.Expressions[0].Token.Source!, stmt.Expressions[0].Token.Position,
                    DiagnosticSeverity.Error, "",
                    "conditional statement (if, while, do/while) condition is expected to be of integer or boolean type");
                return 1;
            }
            return 0;

        case StatementKind.Goto:
            if (stmt.Expressions.Count > 0 && stmt.Expressions[0].Type! != FType.VPTR)
            {
                Diagnostic.Throw(stmt.Expressions[0].Token.Source!, stmt.Expressions[0].Token.Position,
                    DiagnosticSeverity.Error, "",
                    "conditional goto statement expects expression of type *void");
                return 1;
            }
            // direct
            if (stmt.Objects.Count == 1)
            {
                var label = (string)stmt.Objects[0];
                var fx = stmt.FindNearestFunction()!;
                if (!fx.IsLabelDeclared(label))
                {
                    Diagnostic.Throw(stmt.KwToken.Source!, stmt.KwToken.Position, DiagnosticSeverity.Error, "", $"undeclared label {label}");
                    return 1;
                }
            }
            return 0;

        case StatementKind.SwitchCase:
        {
            var constexpr = stmt.Expressions[0];
            if (constexpr.Kind != ExpressionKind.Literal)
            {
                Diagnostic.Throw(constexpr.Token.Source!, constexpr.Token.Position,
                    DiagnosticSeverity.Error, "",
                    "switch case value must be constexpr");
                return 1;
            }

            var evaluated = stmt.FindNearestSwitch()!.Expressions[0];
            var evaluation_type = evaluated.Type!;
            Debug.Assert(evaluation_type is not null);
            constexpr = ImplicitConversionAvailableTU(evaluation_type, constexpr);
            if (constexpr.Type! != evaluation_type)
            {
                Diagnostic.Throw(constexpr.Token.Source!, constexpr.Token.Position,
                    DiagnosticSeverity.Error, "",
                    "switch case value must be compatible with evaluated value type");
                return 1;
            }
            return 0;
        }

        case StatementKind.Asm:
        {
            var template = ParseInlineAsmTemplate(stmt, out var inline_asm_errors);
            ((InlineAsmBlock)stmt.Objects[0]).Template = template;
            return inline_asm_errors;
        }

        }

        return 0;
    }

    /// <summary>
    /// Analyzes a program.
    /// </summary>
    /// <returns>The amount of partially type bound/unknown typed objects.</returns>
    private int AnalyzeWave(List<Symbol> unused_symbols)
    {
        var errors = 0;
        foreach (var child in Program.GetChildren())
        {
            if (child is not FunctionNode fx)
                continue;

            var syms = fx.GetAllSymbols();
            var unused_syms = syms.Where(s => !s.Referenced);

            var Tfunc = (FunctionSubjugateSignature)fx.FuncSym.Type!.SubjugateSignature!;
            var invalid_ref_params = Tfunc.Params.Arguments.Where(
                    a => (a.Type.Traits.HasFlag(TypeTraits.Members) || a.Type.Traits.HasFlag(TypeTraits.Array))
                         && a.Ref == Symbol.ArgumentRefType.Value).Select(a => new { });

            // checking for ref param errors
            var invalid_arguments = new StringBuilder();
            for (var i = 0; i < invalid_ref_params.Count(); i++)
            {
                var arg = Tfunc.Params.Arguments.ElementAt(i);
                var pname = arg.Name;
                var pTname = arg.Type.Name;
                invalid_arguments.AppendLine($"  param {pname} of type {pTname} must be passed by reference");
                errors++;
            }

            if (invalid_arguments.Length > 0)
                Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "",
                    $"in function {fx.Name}{(fx.FuncSym.InternalTo != null ? $" ({fx.FuncSym.InternalTo.Path})" : string.Empty)} ->\n" +
                    invalid_arguments.ToString());
            /////////////////////////////////

            errors += AnalyzeStatement(fx.Body!);
            var param_list = Tfunc.Params.Arguments.Select(p => p.Name);
            foreach (var sym in unused_syms)
            {
                if (sym.Attributes.HasAttribute("maybe_unused", out _)
                    || unused_symbols.Contains(sym)) // prevent duplicates
                    continue;
                Diagnostic.Throw(sym.DiagToken.Source, sym.DiagToken.Position, DiagnosticSeverity.Warning, "unused-symbol",
                    param_list.Contains(sym.Name) ? $"unused param {sym.Name}" : $"unused local symbol {sym.Name}");
                unused_symbols.Add(sym);
            }
            if (fx.FuncSym.InternalTo != null)
            {
                if (fx.FuncSym.Attributes.HasAttribute("maybe_unused", out _)
                    || unused_symbols.Contains(fx.FuncSym)) // prevent duplicates
                    continue;
                Diagnostic.Throw(fx.FuncSym.DiagToken.Source, fx.FuncSym.DiagToken.Position, DiagnosticSeverity.Warning, "unused-symbol",
                    $"unused internal function {fx.FuncSym.Name}");
                unused_symbols.Add(fx.FuncSym);
            }
        }
        return errors;
    }

    /// <summary>
    /// Performs analysis.
    /// </summary>
    public bool Analyze()
    {
        var prev_w_result = 0;
        bool fail;

        // first, process user types
        if (FType.ResolveUserTypes())
            return true;

        var prev_restoration_point = Diagnostic.CreateRestorationPoint();
        var unusued_symbols = new List<Symbol>();
        while (true)
        {
            var w_result = AnalyzeWave(unusued_symbols);
            if (w_result == 0)
            {
                fail = false;
                break;
            }

            if (prev_w_result == w_result)
            {
                if (prev_restoration_point.N != 0)
                    Diagnostic.LoadRestorationPoint(prev_restoration_point);
                fail = true;
                break;
            }

            prev_restoration_point = Diagnostic.CreateRestorationPoint();
            prev_w_result = w_result;
        }

        return fail;
    }

    public Analyzer(FreeTree program_tree)
    {
        Program = program_tree;
    }
}
