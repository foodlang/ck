using ck.Diagnostics;
using ck.Lang.Tree;
using ck.Lang.Tree.Scopes;
using ck.Syntax;
using LLVMSharp.Interop;

namespace ck.Lang.Type;

/// <summary>
/// Descriptor for user (Food) type.
/// </summary>
public sealed class FType
{
    /// <summary>
    /// The size (in bytes) of a scalar object of this
    /// type. Not applicable for vector types.
    /// </summary>
    public nint Size { get; private set; }

    /// <summary>
    /// The traits of this type.
    /// </summary>
    public TypeTraits Traits { get; private set; }

    /// <summary>
    /// Subordinated data (optional)
    /// </summary>
    public ITypeSubjugateSignature? SubjugateSignature { get; private set; }

    /// <summary>
    /// The name of the type.
    /// </summary>
    public string Name { get; private set; }

    /// <summary>
    /// The named constants of the type.
    /// </summary>
    public IEnumerable<NamedConstant> NamedConstants;

    /// <summary>
    /// The parent syntax tree where the type was created.
    /// </summary>
    public readonly SyntaxTree? Parent;

    /// <summary>
    /// Contains all of the types registered in the language.
    /// </summary>
    private static readonly HashSet<FType> _types;

    /// <summary>
    /// Tries to get a type from <see cref="_types"/> using metadata.
    /// </summary>
    private static FType? TryGetType(string name, Source where, SyntaxTree tree)
    {
        foreach (var type in _types)
        {
            if (type.Name == name)
            {
                // declared at top-level
                if (type.Parent == null)
                    return type;

                // class
                if (tree.IsPartOf(type.Parent)
                    && ((ScopeTree)type.Parent).GetDeclaration<FClass>(name, where, out var @class))
                    return @class!.Class;
            }
        }

        return null;
    }

    /// <summary>
    /// Resolves the user types.
    /// </summary>
    public static bool ResolveUserTypes()
    {
        var fail = false;
        var types_to_resolve = _types;
        HashSet<FType> new_types_to_resolve;
        var previous_count = 0;
        while (true)
        {
            new_types_to_resolve = new HashSet<FType>();
            foreach (var type in types_to_resolve)
            {
                if (type.SubjugateSignature is UserPlaceholderSubjugateSignature placeholder)
                {
                    var new_type = TryGetType(placeholder.Name, placeholder.Where!, type.Parent!);

                    if (new_type is not null)
                    {
                        // type copy
                        type.SubjugateSignature = new_type.SubjugateSignature;
                        type.Size = new_type.Size;
                        type.Traits = new_type.Traits;
                        type.NamedConstants = new_type.NamedConstants;
                    }
                    else new_types_to_resolve.Add(type);
                }
            }
            types_to_resolve = new_types_to_resolve;

            // we still got work to do
            if (new_types_to_resolve.Count != 0)
            {
                if (new_types_to_resolve.Count == previous_count)
                    break; // show errors

                previous_count = new_types_to_resolve.Count;
                continue;
            }

            break;
        }

        foreach (var unresolved in new_types_to_resolve)
        {
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", $"unresolved classname '{unresolved.Name}' in program");
            fail = true;
        }
        return fail;
    }

    // using manual static constructor because of odd c# compiler behavior
    static FType()
    {
        _types = [];

        VOID = new FType(0, TypeTraits.Void, null, "void", [], null, LLVMTypeRef.Void);
        BOOL = new FType(1, TypeTraits.Integer, null, "bool", [], null, LLVMTypeRef.Int1);
        I8 = new FType(1, TypeTraits.Arithmetic | TypeTraits.Integer, null, "i8", [], null, LLVMTypeRef.Int8);
        U8 = new FType(1, TypeTraits.Arithmetic | TypeTraits.Integer | TypeTraits.Unsigned, null, "u8", [], null, LLVMTypeRef.Int8);
        I16 = new FType(2, TypeTraits.Arithmetic | TypeTraits.Integer, null, "i16", [], null, LLVMTypeRef.Int16);
        U16 = new FType(2, TypeTraits.Arithmetic | TypeTraits.Integer | TypeTraits.Unsigned, null, "u16", [], null, LLVMTypeRef.Int16);
        I32 = new FType(4, TypeTraits.Arithmetic | TypeTraits.Integer, null, "i32", [], null, LLVMTypeRef.Int32);
        U32 = new FType(4, TypeTraits.Arithmetic | TypeTraits.Integer | TypeTraits.Unsigned, null, "u32", [], null, LLVMTypeRef.Int32);
        I64 = new FType(8, TypeTraits.Arithmetic | TypeTraits.Integer, null, "i64", [], null, LLVMTypeRef.Int64);
        U64 = new FType(8, TypeTraits.Arithmetic | TypeTraits.Integer | TypeTraits.Unsigned, null, "u64", [], null, LLVMTypeRef.Int64);
        ISZ = I64;
        USZ = U64;
        F32 = new FType(4, TypeTraits.Arithmetic | TypeTraits.Floating, null, "f32", [], null, LLVMTypeRef.Float);
        F64 = new FType(8, TypeTraits.Arithmetic | TypeTraits.Floating, null, "f64", [], null, LLVMTypeRef.Double);
        FMAX = F64;
        STRING = Pointer(U8);
        VPTR = Pointer(VOID);
    }

    /// <summary>
    /// Creates a new type descriptor.
    /// </summary>
    /// <param name="size">The size of the type.</param>
    /// <param name="traits">The traits of the type.</param>
    /// <param name="subjugate_signature">The subjugate signature of the type.</param>
    /// <param name="name">The name of the type.</param>
    private FType(nint size, TypeTraits traits, ITypeSubjugateSignature? subjugate_signature, string name, IEnumerable<NamedConstant> named_constants, SyntaxTree? parent, LLVMTypeRef llvm_type_ref)
    {
        Size = size;
        Traits = traits;
        SubjugateSignature = subjugate_signature;
        Name = name;
        Parent = parent;
        NamedConstants = named_constants;
        LLVMTypeReference = llvm_type_ref;
        _types.Add(this);
    }

    public readonly static FType VOID;

    public readonly static FType BOOL;

    public readonly static FType I8;
    public readonly static FType U8;
    public readonly static FType I16;
    public readonly static FType U16;
    public readonly static FType I32;
    public readonly static FType U32;
    public readonly static FType I64;
    public readonly static FType U64;
    public readonly static FType ISZ;
    public readonly static FType USZ;

    public readonly static FType F32;
    public readonly static FType F64;
    public readonly static FType FMAX;
    public readonly static FType STRING;
    public readonly static FType VPTR;

    /// <summary>
    /// Creates a new pointer type descriptor.
    /// </summary>
    /// <param name="subjugate">The subjugate type.</param>
    public static FType Pointer(FType subjugate)
        => new FType(
            USZ.Size, TypeTraits.Integer | TypeTraits.Unsigned | TypeTraits.Pointer | TypeTraits.Arithmetic,
            new PointerSubjugateSignature(subjugate), $"*{subjugate.Name}", [], null, LLVMTypeRef.CreatePointer(subjugate.LLVMTypeReference, 0));

    /// <summary>
    /// Creates a new array type descriptor.
    /// </summary>
    /// <param name="subjugate">The subjugate type.</param>
    /// <param name="len">The size of the array. If set to 0, variable-length array (VLA.)</param>
    public static FType Array(FType subjugate, nint len)
        => new FType(
            USZ.Size, TypeTraits.Integer | TypeTraits.Unsigned | TypeTraits.Pointer | TypeTraits.Array,
            new ArraySubjugateSignature(subjugate, len), $"[{(len != 0 ? len : "")}]{subjugate.Name}", [], null, 
            len != 0 ? LLVMTypeRef.CreateArray(subjugate.LLVMTypeReference, (uint)len)
            : LLVMTypeRef.CreatePointer(subjugate.LLVMTypeReference, 0));

    // TODO: in/var/out
    /// <summary>
    /// Creates a new function type descriptor. 
    /// </summary>
    /// <param name="return_type">The return type of the function.</param>
    /// <param name="argsignature">An object containing all the function parameters and more.</param>
    public static FType Function(FType return_type, FunctionArgumentSignature argsignature)
        => new FType(
            USZ.Size, TypeTraits.Integer | TypeTraits.Unsigned | TypeTraits.Pointer | TypeTraits.Function,
            new FunctionSubjugateSignature(return_type, argsignature),
            $"{return_type.Name}({argsignature.Arguments.Aggregate(
                seed: string.Empty,
                (a, b) =>
                $"{a}, {(b.Ref != Symbol.ArgumentRefType.Value ? b.Ref.ToString().ToLower() : string.Empty)} {b.Name}: {b.Type.Name}").TrimStart([',', ' '])})",
            [],
            null,
            LLVMTypeRef.CreateFunction(return_type.LLVMTypeReference,
                argsignature.Arguments.Select(a => a.Type.LLVMTypeReference).ToArray(),
                argsignature.VariadicType is not null));

    /// <summary>
    /// Creates a new structure type descriptor.
    /// </summary>
    /// <param name="members">The members of the structure type.</param>
    /// <param name="union">Whether the structure type is a union.</param>
    public static FType Struct(StructMember[] members, NamedConstant[] named_constants, bool union, SyntaxTree? parent)
        => new FType(
            0, TypeTraits.Members,
            new StructSubjugateSignature(members, union),
            $"struct{{{members.Aggregate(seed: string.Empty, (a, x) => $"{a}{x.Name}: {x.Type.Name};")}}}"
            , named_constants, parent,
            union ?
                members.MaxBy(m => m.Type.SizeOf())?.Type.LLVMTypeReference ?? LLVMTypeRef.Void
            : LLVMTypeRef.CreateStruct(members.Select(m => m.Type.LLVMTypeReference).ToArray(), false /* TODO: packed */));

    public static FType Enum(NamedConstant[] named_constants, FType underlying, SyntaxTree? parent)
        => new FType(
            underlying.Size, underlying.Traits, new EnumSubjugateSignature(underlying), $"enum:{underlying.Name}", named_constants, parent,
            underlying.LLVMTypeReference);

    /// <summary>
    /// Creates a new user placeholder type descriptor.
    /// </summary>
    /// <param name="ident">The identifier.</param>
    /// <param name="where">Where the identifier was emitted from.</param>
    public static FType UserPlaceholder(string ident, Source? where, SyntaxTree? parent)
        => new FType(0, TypeTraits.UserPlaceholder, new UserPlaceholderSubjugateSignature(ident, where), ident, [], parent,
            LLVMTypeRef.Void);

    // EnumType() was removed because enum types dont make any sense,
    // internal types should be used instead

    public static bool operator ==(FType? a, FType? b)
    {
        if (a is null && b is null)
            return true;

        // satisfying c# compiler because of compiler bug,
        // could've been implemented with
        // (aV && bu) || (au && bV)-like conditions instead
        if (a is null) return false;
        if (b is null) return false;

        //////////////////// null out of the way

        if (!(a.Traits == b.Traits && a.Size == b.Size))
            return false;

        if (a.SubjugateSignature != null && b.SubjugateSignature != null)
            return
                a.SubjugateSignature.GetType() != b.SubjugateSignature.GetType()
                ? false
                : a.SubjugateSignature.EqualTo(b.SubjugateSignature);

        return true;
    }

    public static bool operator !=(FType? a, FType? b) => !(a == b); // sufficient

    public override bool Equals(object? obj)
    {
        if (ReferenceEquals(this, obj))
            return true;

        if (ReferenceEquals(obj, null))
            return false;

        return this == (FType)obj;
    }

    public override int GetHashCode()
        => (int)
        ((long)Size.GetHashCode() * Traits.GetHashCode() * 20);

    /// <summary>
    /// Computes the size of a type.
    /// </summary>
    public nint SizeOf()
    {
        if (Traits.HasFlag(TypeTraits.Array))
        {
            var a = (ArraySubjugateSignature)SubjugateSignature!;
            return a.Length * a.ObjectType.SizeOf();
        }

        if (SubjugateSignature is StructSubjugateSignature struct_signature)
        {
            var accumulator = (nint)0;
            if (struct_signature.IsUnion)
            {
                foreach (var member in struct_signature.Members)
                {
                    var mem_size = member.Type.SizeOf();
                    var aligned = mem_size;
                    accumulator = Math.Max(accumulator, aligned);
                }
            }
            else
            {
                foreach (var member in struct_signature.Members)
                {
                    var mem_size = member.Type.SizeOf();
                    var aligned = (mem_size + (CompilerRunner.AlignmentRequired - 1)) & ~(CompilerRunner.AlignmentRequired - 1);
                    accumulator += aligned;
                }
            }
            return accumulator;
        }

        // final alignment
        return Size;
    }

    /// <summary>
    /// lengthof() operator for arrays and strings.
    /// </summary>
    public nint LengthOf() => SubjugateSignature is not null ? SubjugateSignature.LengthOf() : 0;

    /// <summary>
    /// The LLVM type reference for the given type.
    /// </summary>
    public readonly LLVMTypeRef LLVMTypeReference;
}
