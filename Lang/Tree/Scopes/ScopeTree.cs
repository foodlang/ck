using ck.Lang.Type;
using ck.Syntax;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// A syntax tree that contains a scope.
/// </summary>
public class ScopeTree : FreeTree
{
    /// <summary>
    /// The table holding all of the declarations in the scope.
    /// </summary>
    public readonly ObjectTable Table;

    public ScopeTree(SyntaxTree? parent) : base(parent)
    {
        Table = new(this);
    }

    /// <summary>
    /// Attempts to get a declaration.
    /// </summary>
    /// <param name="name">The name of the declaration.</param>
    /// <param name="src">The current file.</param>
    /// <param name="decl">(Out) The declaration bearing the name <paramref name="name"/>. <c>null</c> if not found.</param>
    /// <returns>True if declared.</returns>
    public bool GetDeclaration<T>(string name, Source? src, out T? decl)
        where T : NamedObject
    {
        decl = null; // failure
        
        // lookup current table
        if (Table.GetDeclaration(name, src, out decl))
            return true;

        // lookup parent table
        var parent_scope = FindNearestParentScope();
        if (parent_scope is null)
            return false;
        if (parent_scope.GetDeclaration(name, src, out decl))
            return true;

        // not found :(
        return false;
    }
    
    /// <summary>
    /// Gets all of the symbols in the scope.
    /// </summary>
    public IEnumerable<Symbol> GetAllSymbols()
        =>
        Table.GetSymbols()
        .Concat(
            // POSSIBLE BUG: can't reach into nodes separated by non-ScopeTrees
            GetChildren().Where(c => c is ScopeTree).Cast<ScopeTree>().SelectMany(s => s.GetAllSymbols())
            );

    /// <summary>
    /// Gets all of the used symbols in this scope.
    /// </summary>
    public IEnumerable<Symbol> GetAllUsedSymbols()
        => Table
        .GetSymbols()
        .Where(s => s.Referenced || ((FunctionSubjugateSignature)FindNearestFunction()!.FuncSym.Type!.SubjugateSignature!)
                        .Params.Arguments.Select(p => p.Name).Contains(s.Name)) // SPONGE
        .Concat(
            // POSSIBLE BUG: can't reach into nodes separated by non-ScopeTrees
            GetChildren()
            .Where(c => c is ScopeTree)
            .Cast<ScopeTree>()
            .SelectMany(s => s.GetAllUsedSymbols())
            );

    /// <summary>
    /// Gets all of the unused symbols in this scope.
    /// </summary>
    public IEnumerable<Symbol> GetAllUnusedSymbols()
        => Table
        .GetSymbols()
        .Where(s => !s.Referenced)
        .Concat(
            // POSSIBLE BUG: can't reach into nodes separated by non-ScopeTrees
            GetChildren()
            .Where(c => c is ScopeTree)
            .Cast<ScopeTree>()
            .SelectMany(s => s.GetAllUnusedSymbols())
            );

    /// <summary>
    /// Attempts to declare a symbol.
    /// </summary>
    /// <param name="attributes">The attributes of the symbol.</param>
    /// <param name="T">The type of the symbol. May be set to <c>null</c> if the type is not determined at declaration.</param>
    /// <param name="ref_type">(Param only) The type of reference used, if using references.</param>
    /// <param name="sym">(Out) The symbol. If the function fails, this returns the symbol bearing the same ident.</param>
    /// <returns>Returns false if the symbol is already declared.</returns>
    public bool DeclareSymbol(Token identifier, Source? internal_to, AttributeClause attributes, FType? T,
        Symbol.ArgumentRefType ref_type,
        out Symbol sym)
    {
        var name = (string)identifier.Value!;

        // Checks if the symbol is already declared
        if (GetDeclaration(name, internal_to, out sym!))
            return false;
        return Table.DeclareSymbol(identifier, internal_to, attributes, T, ref_type, out sym);
    }

    /// <summary>
    /// Attempts to declare a class.
    /// </summary>
    /// <param name="attributes">The attributes of the class.</param>
    /// <param name="T">The type this class points to.</param>
    /// <param name="class">(Out) The class. If the function fails, this returns the class bearing the same ident.</param>
    /// <returns>Returns false if the class is already declared.</returns>
    public bool DeclareClass(Token identifier, Source? internal_to, AttributeClause attributes, FType T,
        out FClass @class)
    {
        var name = (string)identifier.Value!;

        // Checks if the class is already declared
        if (GetDeclaration(name, internal_to, out @class!))
            return false;
        return Table.DeclareClass(identifier, internal_to, attributes, T, out @class);
    }

    /// <summary>
    /// Attempts to declare an enum. Does not declare an enum's class, this must be done
    /// with two separate declarations.
    /// </summary>
    /// <param name="attributes">The attributes of the enum.</param>
    /// <param name="named_constants">The named constants of the enum.</param>
    /// <returns>Returns false if the class is already declared.</returns>
    public bool DeclareEnum(Token identifier, Source? internal_to, AttributeClause attributes, (string Name, nint Const)[] named_constants,
        out FEnum @enum)
    {
        var name = (string)identifier.Value!;

        // Checks if the symbol is already declared
        if (GetDeclaration(name, internal_to, out @enum!))
            return false;
        return Table.DeclareEnum(identifier, internal_to, attributes, named_constants, out @enum);
    }
}
