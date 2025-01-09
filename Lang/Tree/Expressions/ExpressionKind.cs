namespace ck.Lang.Tree.Expressions;

/// <summary>
/// The kind of an expression.
/// </summary>
public enum ExpressionKind
{
    /// <summary>
    /// An error. Used as a placeholder to avoid
    /// internal errors.
    /// </summary>
    Error,

    /// <summary>
    /// A literal value is a compile-time constant
    /// (e.g. <c>null</c>, <c>123</c>, <c>"Hello, World!"</c>, etc.)
    /// </summary>
    Literal,

    /// <summary>
    /// Initializes an array, typically by putting it on the stack or
    /// in the data segment.
    /// </summary>
    CollectionInitialization,

    /// <summary>
    /// The <c>sizeof()</c> operator. Should not 
    /// exist at codegen level.
    /// </summary>
    SizeOf,

    /// <summary>
    /// The <c>nameof()</c> operator. Should not 
    /// exist at codegen level.
    /// </summary>
    NameOf,

    /// <summary>
    /// Gets the default value of types.
    /// </summary>
    Default,

    /// <summary>
    /// Constructs a new value for an aggregrate types.
    /// </summary>
    New,

    /// <summary>
    /// A symbol reference.
    /// </summary>
    Identifier,

    /// <summary>
    /// Casting syntax is <c>cast&lt;T&gt;(x)</c>.
    /// </summary>
    Cast,

    /// <summary>
    /// Allows to access symbols inside of modules and namespaces.
    /// </summary>
    ScopeResolution,

    /// <summary>
    /// (TODO) Gets the length of an array or a string.
    /// </summary>
    LengthOf,

    PostfixIncrement,
    PostfixDecrement,
    FunctionCall,
    ArraySubscript,
    MemberAccess,

    PrefixIncrement,
    PrefixDecrement,
    UnaryPlus,
    UnaryMinus,
    LogicalNot,
    BitwiseNot,
    Dereference,
    AddressOf,
    /// <summary>
    /// Same as <see cref="AddressOf"/>, but will always return an opaque pointer (<c>*void</c>)
    /// </summary>
    OpaqueAddressOf,

    Multiply,
    Divide,
    Modulo,

    Add,
    Subtract,

    LeftShift,
    RightShift,

    Lower,
    LowerEqual,
    Greater,
    GreaterEqual,

    MemberOf,     // =.
    AllMembersOf, // =..
    AnyMembersOf, // =?.

    Equal,
    NotEqual,

    BitwiseAnd,

    BitwiseXOr,

    BitwiseOr,

    LogicalAnd,

    LogicalOr,

    /// <summary>
    /// Ternary conditional expression <c>B ? x : y</c> where <c>typeof(B).traits & TRAIT_INTEGER</c>
    /// </summary>
    Conditional,

    /// <summary>
    /// Variable initialization
    /// </summary>
    VariableInit,

    Assign,
    AssignSum,
    AssignDiff,
    AssignProduct,
    AssignQuotient,
    AssignRemainder,
    AssignBitwiseAnd,
    AssignBitwiseXOr,
    AssignBitwiseOr,
    AssignLeftShift,
    AssignRightShift,
}
