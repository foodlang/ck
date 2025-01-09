namespace ck.Syntax;

/// <summary>
/// The kind of a token.
/// </summary>
public enum TokenKind
{
    /// <summary>
    /// Terminates a token buffer.
    /// </summary>
    Terminator = 0,

    /// <summary>
    /// Error token
    /// </summary>
    Unknown,

    NumberLiteral,
    StringLiteral,

    Identifier,

    LeftBracket,        // (
    RightBracket,       // )
    LeftSquareBracket,  // [
    RightSquareBracket, // ]
    LeftCurlyBracket,   // {
    RightCurlyBracket,  // }
    Tilde,              // ~
    QuestionMark,       // ?
    Semicolon,          // ;
    Comma,              // ,
    Pound,              // #
    Plus,               // +
    PlusPlus,           // ++
    PlusEqual,          // +=
    And,                // &
    AndAnd,             // &&
    AndEqual,           // &=
    Pipe,               // |
    PipePipe,           // ||
    PipeEqual,          // |=
    Minus,              // -
    MinusMinus,         // --
    MinusEqual,         // -=
    ThinArrow,          // ->
    Equal,              // =
    EqualEqual,         // ==
    ThickArrow,         // =>
    EqualDot,           // =.
    EqualDotDot,        // =..
    EqualQuestionDot,   // =?.
    ExclamationMark,    // !
    ExclamationEqual,   // !=
    Star,               // *
    StarEqual,          // *=
    Slash,              // /
    SlashEqual,         // /=
    Percentage,         // %
    PercentageEqual,    // %=
    Caret,              // ^
    CaretEqual,         // ^=
    Colon,              // :
    Cube,               // ::
    ColonEqual,         // :=
    Dot,                // .
    DotDot,             // ..
    DotDotDot,          // ...
    Lower,              // <
    LowerEqual,         // <=
    LeftShift,          // <<
    LeftShiftEqual,     // <<=
    Greater,            // >
    GreaterEqual,       // >=
    RightShift,         // >>
    RightShiftEqual,    // >>=

    New,
    Default,
    True,
    False,
    Null,
    SizeOf,
    LengthOf,
    NameOf,
    TypeOf,
    Cast,

    Struct,
    Union,
    Enum,
    Class,

    Void,
    Bool,
    I8,
    U8,
    I16,
    U16,
    F16,
    I32,
    U32,
    F32,
    I64,
    U64,
    F64,
    ISZ,
    USZ,
    FMAX,
    String,

    Let,
    Func,
    Varying,
    In,
    Out,
    Internal,

    Const,
    Atomic,
    Shared,

    Import,

    If,
    Else,
    While,
    Do,
    Switch,
    Case,
    Break,
    Continue,
    Return,
    For,
    Goto,
    Assert,
    Sponge,
    Finally,

    Asm,
}
