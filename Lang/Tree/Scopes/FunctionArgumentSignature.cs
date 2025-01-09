using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// A function's argument signature.
/// </summary>
public sealed class FunctionArgumentSignature
{
    public sealed class Arg
    {
        public readonly string Name;
        public readonly Token Identifier;
        public readonly FType Type;
        public readonly Symbol.ArgumentRefType Ref;
        public readonly object? DefaultValue;
        public readonly AttributeClause Attributes;

        public Arg(Token identifier, FType type, Symbol.ArgumentRefType @ref, AttributeClause attributes, object? default_value)
        {
            Name = (string)identifier.Value!;
            Identifier = identifier;
            Type = type;
            Ref = @ref;
            Attributes = attributes;
            DefaultValue = default_value;
        }

        /// <summary>
        /// Gets the symbol associated with the argument. This function will die and catch fire if <paramref name="n"/>
        /// does not bear an argument of the same name. Bugs may still occur if both functions have the same param name,
        /// but a different definition.
        /// </summary>
        /// <param name="n">The parent function.</param>
        public Symbol GetSymbol(FunctionNode n)
        {
            if (!n.GetDeclaration<Symbol>(Name, Identifier.Source, out var ret))
                Debug.Fail(null);
            return ret!;
        }
    }

    public readonly IEnumerable<Arg> Arguments;
    public readonly FType? VariadicType; /* void for untyped variadic */

    public FunctionArgumentSignature(IEnumerable<Arg> arguments, FType? variadic_type)
    {
        Arguments = arguments;
        VariadicType = variadic_type;
    }
}
