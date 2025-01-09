namespace ck.Lang.Type;

/// <summary>
/// Traits for Food types.
/// </summary>
[Flags]
public enum TypeTraits : uint
{
    /// <summary>
    /// Void type
    /// </summary>
    Void = 1 << 0,

    /// <summary>
    /// Arithmetic type (arithmetic operations are allowed on it)
    /// </summary>
    Arithmetic = 1 << 1,

    /// <summary>
    /// An integer type
    /// </summary>
    Integer = 1 << 2,

    /// <summary>
    /// Unsigned type (lowest is 0)
    /// </summary>
    Unsigned = 1 << 3,

    /// <summary>
    /// Can be dereferenced (e.g. pointers) and points to a valid type
    /// </summary>
    Pointer = 1 << 4,

    /// <summary>
    /// Cannot be mutated (e.g. strings, records)
    /// </summary>
    Immutable = 1 << 5,

    /// <summary>
    /// Can be indexed and may have a specific length
    /// </summary>
    Array = 1 << 6,

    /// <summary>
    /// Has members (e.g. structs, unions)
    /// </summary>
    Members = 1 << 7,

    /// <summary>
    /// Whether the type is a function and can be invoked
    /// </summary>
    Function = 1 << 8,

    /// <summary>
    /// Whether the type is a float. Only compatible with <see cref="Array"/>, <see cref="Members"/> and <see cref="Arithmetic"/>.
    /// </summary>
    Floating = 1 << 9,

    /// <summary>
    /// User placeholder trait
    /// </summary>
    UserPlaceholder = 1 << 11,
}
