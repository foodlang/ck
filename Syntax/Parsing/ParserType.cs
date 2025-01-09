using ck.Diagnostics;
using ck.Lang.Type;
using ck.Lang.Tree.Scopes;
using ck.Lang.Tree.Expressions;
using ck.Lang;
using ck.Lowering;

namespace ck.Syntax.Parsing;

public sealed partial class Parser
{
    /// <summary>
    /// Parses a type.
    /// </summary>
    public FType? ParseType(ScopeTree? parent)
    {
        FType? t_acc = null;
        var current = Current();
        Position++;

        switch (current.Kind)
        {
        case TokenKind.Void: t_acc = FType.VOID; goto L_AttemptParseDecorators;
        case TokenKind.Bool: t_acc = FType.BOOL; goto L_AttemptParseDecorators;
        case TokenKind.I8: t_acc = FType.I8; goto L_AttemptParseDecorators;
        case TokenKind.U8: t_acc = FType.U8; goto L_AttemptParseDecorators;
        case TokenKind.I16: t_acc = FType.I16; goto L_AttemptParseDecorators;
        case TokenKind.U16: t_acc = FType.U16; goto L_AttemptParseDecorators;
        case TokenKind.I32: t_acc = FType.I32; goto L_AttemptParseDecorators;
        case TokenKind.U32: t_acc = FType.U32; goto L_AttemptParseDecorators;
        case TokenKind.F32: t_acc = FType.F32; goto L_AttemptParseDecorators;
        case TokenKind.I64: t_acc = FType.I64; goto L_AttemptParseDecorators;
        case TokenKind.U64: t_acc = FType.U64; goto L_AttemptParseDecorators;
        case TokenKind.F64: t_acc = FType.F64; goto L_AttemptParseDecorators;
        case TokenKind.ISZ: t_acc = FType.ISZ; goto L_AttemptParseDecorators;
        case TokenKind.USZ: t_acc = FType.USZ; goto L_AttemptParseDecorators;
        case TokenKind.FMAX: t_acc = FType.FMAX; goto L_AttemptParseDecorators;
        case TokenKind.String: t_acc = FType.STRING; goto L_AttemptParseDecorators;

        case TokenKind.F16:
            // TODO: change this if expanding to more architectures
            Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "",
                "bfloat16 (internal implementation of the f16 type) is natively unsupported for the amd64 architecture");
            t_acc = FType.F32; // avoid summoning dragons
            goto L_AttemptParseDecorators;

        case TokenKind.Identifier:
        {
            if (parent is null)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                    "context does not permit user-defined types");
                return null;
            }

            var ident = (string)current.Value!;

            // type was declared before reaching type parsing
            t_acc = parent.GetDeclaration<FClass>(ident, current.Source!, out var @class)
                ? @class!.Class
                : FType.UserPlaceholder((string)current.Value!, current.Source, parent);
            goto L_AttemptParseDecorators;
        }

        case TokenKind.Enum:
        {
            var underlying = FType.I32;
            if (Current().Kind == TokenKind.LeftBracket)
            {
                Position++;
                underlying = ParseType(parent);
                if (underlying is null)
                {
                    Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                        "expected underlying type in enum declaration (specifier : present)");
                    return null;
                }
                ExpectAndConsume(TokenKind.RightBracket);
            }

            ExpectAndConsume(TokenKind.LeftCurlyBracket);
            nint current_const = 0;
            var consts = new List<NamedConstant>();
            while (true)
            {
                if (Current().Kind != TokenKind.Identifier && Current().Kind != TokenKind.Default)
                {
                    Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", "expected constant name as identifier or default keyword");
                    Position++;
                    continue;
                }
                var member_ident = Current();
                var member_name = (string?)member_ident.Value;
                Position++;

                if (Current().Kind == TokenKind.Equal)
                {
                    Position++;
                    if (Current().Kind != TokenKind.NumberLiteral)
                    {
                        Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", "expected enum const as literal integer");
                        Position++;
                        continue;
                    }

                    var literal = Current();
                    current_const = (nint)(decimal)literal.Value!;
                    Position++;
                }

                if (consts.Where(nc => nc.Name == member_name).Any())
                {
                    Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "",
                        $"duplicate named constant {member_name ?? "default"}");
                    continue;
                }

                consts.Add(
                    new(member_name,
                        new ChildlessExpression(
                            new(Current().Position, TokenKind.NumberLiteral, underlying, current_const),
                            ExpressionKind.Literal)
                            { Type = underlying },
                        null));

                // dont mind the finnicky sh*t around here
                if (Current().Kind == TokenKind.RightCurlyBracket || Current().Kind == TokenKind.Terminator)
                {
                    Position++;
                    break;
                }

                ExpectAndConsume(TokenKind.Comma);

                if (Current().Kind == TokenKind.RightCurlyBracket || Current().Kind == TokenKind.Terminator)
                {
                    Position++;
                    break;
                }
            }

            if (!consts.Where(nc => nc.Name is null).Any())
                consts.Add(new(null, new ChildlessExpression(new(Current().Position,
                    TokenKind.NumberLiteral, underlying, (decimal)0), ExpressionKind.Literal) { Type = underlying }, null));

            return FType.Enum(consts.ToArray(), underlying, parent);
        }

        case TokenKind.Struct:
        case TokenKind.Union:
        {
            ExpectAndConsume(TokenKind.LeftCurlyBracket);

            var l_structmems = new List<StructMember>();
            var l_namedconstants = new List<NamedConstant>();
            var switched_to_named_constant_decl_mode = false;
            while (true)
            {
                if (Current().Kind == TokenKind.RightCurlyBracket)
                    break;
                var ident = Current();
                if (ident.Kind != TokenKind.Identifier && ident.Kind != TokenKind.Default)
                {
                    Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error,
                        "", "expected ident of structure member");
                    break;
                }
                Position++;
                /* struct member declaration */
                /* ident: type; */
                if (Current().Kind == TokenKind.Colon)
                {
                    if (ident.Kind == TokenKind.Default)
                    {
                        Diagnostic.Throw(ident.Source, ident.Position, DiagnosticSeverity.Error,
                            "", "default is not allowed in struct member declarations, use :: for named constants");
                        break;
                    }
                    Position++;

                    var member_T = ParseType(parent);
                    if (member_T is null)
                    {
                        Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error,
                            "", "expected type of structure member");
                        break;
                    }

                    if (l_structmems.Where(mem => mem.Name == (string)ident.Value!).Any())
                    {
                        Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error,
                            "", $"duplicate struct member {(string)ident.Value!}");
                        break;
                    }
                    
                    l_structmems.Add(new((string)ident.Value!, member_T));

                    if (switched_to_named_constant_decl_mode)
                    {
                        Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error,
                            "", "declaring a struct member after a named constant is invalid");
                    }
                }
                /* named constant declaration */
                /* ident :: other-named-constant OR initializer-list; */
                else if (Current().Kind == TokenKind.Cube)
                {
                    switched_to_named_constant_decl_mode = true;
                    t_acc = FType.Struct(l_structmems.ToArray(), [], current.Kind == TokenKind.Union, parent);

                    var cube = Current();
                    Position++;

                    /* default will be automatically null */
                    var new_nc_name = (string?)ident.Value;
                    if (l_namedconstants.Where(nc => nc.Name == new_nc_name).Count() != 0)
                    {
                        Diagnostic.Throw(cube.Source, cube.Position, DiagnosticSeverity.Error,
                            "", $"duplicate named constant {new_nc_name ?? "default"} in this struct/union");
                        break;
                    }

                    /* alias with ident */
                    if (Current().Kind == TokenKind.Identifier || Current().Kind == TokenKind.Default)
                    {
                        var aliasing_ident = Current();
                        if (aliasing_ident.Kind != TokenKind.Identifier && aliasing_ident.Kind != TokenKind.Default)
                        {
                            Diagnostic.Throw(cube.Source, cube.Position, DiagnosticSeverity.Error,
                                "", "expected identifier or default keyword");
                            break;
                        }
                        var aliasing_name = (string?)aliasing_ident.Value!;
                        Position++;
                        var aliasing_candidates = l_namedconstants.Where(nc => nc.Name == aliasing_name);
                        if (aliasing_candidates.Count() == 0)
                        {
                            Diagnostic.Throw(cube.Source, cube.Position, DiagnosticSeverity.Error,
                                "", $"named constant {aliasing_name} not declared before in this struct/union");
                            break;
                        }

                        l_namedconstants.Add(new(new_nc_name, null, aliasing_candidates.First()));
                    }
                    /* initializer-list */
                    else
                    {
                        var initializer_list = ParseInitializerList(null);
                        foreach (var initializer in initializer_list)
                        {
                            var member_candidate = l_structmems.Where(m => m.Name == initializer.Name);
                            if (!member_candidate.Any())
                            {
                                Diagnostic.Throw(cube.Source, cube.Position, DiagnosticSeverity.Error,
                                    "", $"unknown struct member {initializer.Name}");
                                break;
                            }

                            var member = member_candidate.First();
                            var expr = initializer.E;
                            if (Analyzer.AnalyzeExpression(ref expr) != 0)
                                break;
                            expr = Analyzer.ImplicitConversionAvailableTU(member.Type, expr);
                            if (expr.Type != member.Type)
                            {
                                Diagnostic.Throw(cube.Source, cube.Position, DiagnosticSeverity.Error,
                                    "", $"type mismatch for {initializer.Name}");
                                break;
                            }
                        }
                        var constant_value = new AggregateExpression(cube, ExpressionKind.New, initializer_list.Select(i => i.E), initializer_list.Select(i => i.Name)) { Type = t_acc };
                        l_namedconstants.Add(new(new_nc_name, constant_value, null));
                    }
                }
                ExpectAndConsume(TokenKind.Semicolon);
            }

            var is_union = current.Kind == TokenKind.Union;
            if (is_union && l_structmems.Count == 0)
            {
                Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Warning, "empty-union",
                    $"empty unions are discouraged and treated as empty structs");
                is_union = false;
            }

            if (t_acc is null)
                t_acc = FType.Struct(l_structmems.ToArray(), [], is_union, parent);
            if (!l_namedconstants.Where(nc => nc.Name is null).Any())
                l_namedconstants.Add(new(null, new AggregateExpression(current, ExpressionKind.New, Enumerable.Empty<Expression>()) { Type = t_acc }, null));

            ExpectAndConsume(TokenKind.RightCurlyBracket);
            t_acc.NamedConstants = l_namedconstants;
            goto L_AttemptParseDecorators;
        }

        }

        Position--;
        return null;

    L_AttemptParseDecorators:
        current = Current();

        /* pointer */
        if (current.Kind == TokenKind.Star)
        {
            Position++;
            t_acc = FType.Pointer(t_acc);
            goto L_AttemptParseDecorators;
        }
        else if (current.Kind == TokenKind.LeftSquareBracket)
        {
            Position++;
            var size = (nint)0;
            if (Current().Kind == TokenKind.RightSquareBracket)
                Position++;
            else if (Current().Kind == TokenKind.NumberLiteral)
            {
                size = (nint)(decimal)Current().Value!;
                Position++;
                ExpectAndConsume(TokenKind.RightSquareBracket);
            }
            else Diagnostic.Throw(current.Source, current.Position, DiagnosticSeverity.Error, "", "expected `]` or integer literal");

             t_acc = FType.Array(t_acc, size);
            goto L_AttemptParseDecorators;
        }
        else if (current.Kind != TokenKind.LeftBracket)
            return t_acc;

        Position++;
        var args = new List<FunctionArgumentSignature.Arg>();
        while (true)
        {
            if (Current().Kind == TokenKind.RightBracket)
            {
                Position++;
                break;
            }

            var attributes = ParseAttributeClause();

            var ref_type = Current().Kind switch
            {
                TokenKind.In => Symbol.ArgumentRefType.In,
                TokenKind.Out => Symbol.ArgumentRefType.Out,
                TokenKind.Varying => Symbol.ArgumentRefType.Varying,
                _ => Symbol.ArgumentRefType.Value
            };

            if (ref_type != Symbol.ArgumentRefType.Value)
                Position++;

            var ident = Current();
            if (ident.Kind != TokenKind.Identifier)
            {
                Diagnostic.Throw(ident.Source, ident.Position, DiagnosticSeverity.Error,
                    "", "expected identifier in func param");
                break;
            }
            Position++;

            ExpectAndConsume(TokenKind.Colon);

            var arg = ParseType(parent);
            if (arg is null)
            {
                Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                "expected argument type");
                Position++;
                continue;
            }
            args.Add(new(ident, arg, ref_type, attributes, null));
            if (Current().Kind == TokenKind.Comma)
            {
                Position++;
                continue;
            }
            else if (Current().Kind == TokenKind.RightBracket)
            {
                Position++;
                break;
            }
            Diagnostic.Throw(Current().Source, Current().Position, DiagnosticSeverity.Error, "",
                "expected comma , on right bracket }");
            Position++;
        }
        t_acc = FType.Function(t_acc, new(args, null));
        goto L_AttemptParseDecorators;
    }
}
