using ck.Lang.Type;
using ck.Syntax;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree.Scopes;

/// <summary>
/// A table storing a group of <see cref="NamedObject"/>s bound to the same
/// scope.
/// </summary>
public sealed class ObjectTable
{
    /// <summary>
    /// The parent scope.
    /// </summary>
    public readonly ScopeTree? Parent;

    /// <summary>
    /// The symbol table (rw)
    /// </summary>
    private readonly Dictionary<string, Symbol> _symbols = new();

    /// <summary>
    /// The classes (rw)
    /// </summary>
    private readonly Dictionary<string, FClass> _classes = new();

    /// <summary>
    /// The enums (rw)
    /// </summary>
    private readonly Dictionary<string, FEnum> _enums = new();

    /// <summary>
    /// Enumerates the symbols of the table.
    /// </summary>
    public IEnumerable<Symbol> GetSymbols() => _symbols.Values;

    /// <summary>
    /// Enumerates the classes of the table.
    /// </summary>
    public IEnumerable<FClass> GetClasses() => _classes.Values;

    /// <summary>
    /// Enumerates the enums of the table.
    /// </summary>
    /// <returns></returns>
    public IEnumerable<FEnum> GetEnums() => _enums.Values;

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
        if (typeof(T) == typeof(Symbol)
            && _symbols.TryGetValue(name, out var sym)
            && (sym.InternalTo != null ? sym.InternalTo == src : true))
        {
            decl = sym as T;
            return true;
        }

        if (typeof(T) == typeof(FClass)
            && _classes.TryGetValue(name, out var @class)
            && (@class.InternalTo != null ? @class.InternalTo == src : true))
        {
            decl = @class as T;
            return true;
        }

        if (typeof(T) == typeof(FEnum)
            && _enums.TryGetValue(name, out var @enum)
            && (@enum.InternalTo != null ? @enum.InternalTo == src : true))
        {
            decl = @enum as T;
            return true;
        }

        decl = null;
        return false;
    }

    /// <summary>
    /// Attempts to declare a symbol.
    /// </summary>
    /// <param name="identifier">The identifier token. Also used for diagnostics.</param>
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

        sym = new Symbol(this, name, internal_to, attributes, identifier, T, ref_type);
        _symbols.Add(name, sym);
        return true;
    }

    /// <summary>
    /// Attempts to declare a class.
    /// </summary>
    /// <param name="identifier">The identifier. Also used for diagnostic purposes.</param>
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

        @class = new FClass(this, name, internal_to, attributes, identifier, T);
        _classes.Add(name, @class);
        return true;
    }

    /// <summary>
    /// Attempts to declare an enum. Does not declare an enum's class, this must be done
    /// with two separate declarations.
    /// </summary>
    /// <param name="identifier">The identifier. Also used for diagnostic purposes.</param>
    /// <param name="attributes">The attributes of the enum.</param>
    /// <param name="named_constants">The named constants of the enum.</param>
    /// <param name="enum">(Out) The enum. If the function fails, this returns the enum bearing the same ident.</param>
    /// <returns>Returns false if the class is already declared.</returns>
    public bool DeclareEnum(Token identifier, Source? internal_to, AttributeClause attributes, (string Name, nint Const)[] named_constants,
        out FEnum @enum)
    {
        var name = (string)identifier.Value!;

        // Check if the enum is already declared
        if (GetDeclaration(name, internal_to, out @enum!))
            return false;

        @enum = new FEnum(this, name, internal_to, attributes, identifier, named_constants);
        _enums.Add(name, @enum);
        return true;
    }

    /// <summary>
    /// Adds another table to this one.
    /// </summary>
    /// <param name="other"></param>
    public void Merge(ObjectTable other)
    {
        foreach (var sym in other._symbols)
            _symbols.Add(sym.Key, sym.Value);
        foreach (var @class in other._classes)
            _classes.Add(@class.Key, @class.Value);
        foreach (var @enum in other._enums)
            _enums.Add(@enum.Key, @enum.Value);
    }

    public ObjectTable(ScopeTree? parent)
    {
        Parent = parent;
    }
}
