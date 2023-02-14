/*
 * Basic typedefs.
*/

#ifndef CK_TYPES_H_
#define CK_TYPES_H_

#include <stdint.h>
#include <math.h>

typedef signed char bool_t;

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef NULL
	#define NULL (void *)0
#endif

static inline bool_t FloatEqual(double a, double b)
{
	const double diff = fmax(a, b) - fmin(a, b);

	return diff < 0.00005 || diff > 0.00005;
}

typedef struct m256_t
{
	uint64_t low;
	uint64_t high;

} __declspec(align(1)) m256_t;

#define CK_QUALIFIER_CONST_BIT    1
#define CK_QUALIFIER_VOLATILE_BIT 2
#define CK_QUALIFIER_RESTRICT_BIT 4
#define CK_QUALIFIER_ATOMIC_BIT   8

/// <summary>
/// Represents a token, an indivisible bit of text that is used to
/// represent the syntax of the source code.
/// </summary>
typedef struct CkToken
{

	/// <summary>
	/// Where the token is located.
	/// </summary>
	size_t position;

	/// <summary>
	/// The kind of the token.
	/// </summary>
	uint64_t kind;

	/// <summary>
	/// An additional value stored with the token.
	/// </summary>
	union {
		bool_t boolean;
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		float f32;
		double f64;
		char *cstr;
		void *ptr;
	} value;

} CkToken;

typedef enum FoodTypeID
{
	CK_FOOD_VOID = 1,
	CK_FOOD_BOOL,
	CK_FOOD_I8,
	CK_FOOD_U8,
	CK_FOOD_I16,
	CK_FOOD_U16,
	CK_FOOD_F16,
	CK_FOOD_I32,
	CK_FOOD_U32,
	CK_FOOD_ENUM,
	CK_FOOD_F32,
	CK_FOOD_I64,
	CK_FOOD_U64,
	CK_FOOD_F64,

	CK_FOOD_FUNCPOINTER,
	CK_FOOD_POINTER,
	CK_FOOD_REFERENCE,

	CK_FOOD_ARRAY,

	CK_FOOD_STRUCT,
	CK_FOOD_UNION,
	CK_FOOD_STRING,
} FoodTypeID;

/// <summary>
/// Represents a type in the Food programming language.
/// Use CkFoodCreateTypeInstance() to create,
/// and CkFoodDeleteTypeInstance() to delete.
/// </summary>
typedef struct CkFoodType
{
	/// <summary>
	/// The type identifier. Must be equal to one of the
	/// CK_FOOD_(typename) macros.
	/// </summary>
	FoodTypeID id;

	/// <summary>
	/// A bit array storing the type qualifiers.
	/// </summary>
	uint8_t  qualifiers;

	/// <summary>
	/// Types can have subtypes. This points to this
	/// type's direct subtype.
	/// </summary>
	struct CkFoodType *child;

} CkFoodType;

/// <summary>
/// The kind of an expression (its operator.)
/// </summary>
typedef enum CkExpressionKind
{

	/// <summary>
	/// Misc. unknown expression identifier
	/// </summary>
	CK_EXPRESSION_DUMMY,

	CK_EXPRESSION_COMPOUND_LITERAL,
	CK_EXPRESSION_INTEGER_LITERAL,
	CK_EXPRESSION_FLOAT_LITERAL,
	CK_EXPRESSION_STRING_LITERAL,
	CK_EXPRESSION_BOOL_LITERAL,
	CK_EXPRESSION_TYPE,
	CK_EXPRESSION_SIZEOF,
	CK_EXPRESSION_ALIGNOF,
	CK_EXPRESSION_NAMEOF,
	CK_EXPRESSION_TYPEOF,

	CK_EXPRESSION_POSTFIX_INC,
	CK_EXPRESSION_POSTFIX_DEC,
	CK_EXPRESSION_FUNCCALL,
	CK_EXPRESSION_SUBSCRIPT,
	CK_EXPRESSION_MEMBER_ACCESS,
	CK_EXPRESSION_POINTER_MEMBER_ACCESS,

	CK_EXPRESSION_PREFIX_INC,
	CK_EXPRESSION_PREFIX_DEC,
	CK_EXPRESSION_UNARY_PLUS,
	CK_EXPRESSION_UNARY_MINUS,
	CK_EXPRESSION_LOGICAL_NOT,
	CK_EXPRESSION_BITWISE_NOT,
	CK_EXPRESSION_C_CAST,
	CK_EXPRESSION_DEREFERENCE,
	CK_EXPRESSION_ADDRESS_OF,

	CK_EXPRESSION_MUL,
	CK_EXPRESSION_DIV,
	CK_EXPRESSION_MOD,

	CK_EXPRESSION_ADD,
	CK_EXPRESSION_SUB,

	CK_EXPRESSION_LEFT_SHIFT,
	CK_EXPRESSION_RIGHT_SHIFT,

	CK_EXPRESSION_LOWER,
	CK_EXPRESSION_LOWER_EQUAL,
	CK_EXPRESSION_GREATER,
	CK_EXPRESSION_GREATER_EQUAL,

	CK_EXPRESSION_EQUAL,
	CK_EXPRESSION_NOT_EQUAL,

	CK_EXPRESSION_BITWISE_AND,

	CK_EXPRESSION_BITWISE_XOR,

	CK_EXPRESSION_BITWISE_OR,

	CK_EXPRESSION_LOGICAL_AND,

	CK_EXPRESSION_LOGICAL_OR,

	CK_EXPRESSION_FOOD_CAST,

	CK_EXPRESSION_CONDITIONAL,

	CK_EXPRESSION_ASSIGN,
	CK_EXPRESSION_ASSIGN_SUM,
	CK_EXPRESSION_ASSIGN_DIFF,
	CK_EXPRESSION_ASSIGN_PRODUCT,
	CK_EXPRESSION_ASSIGN_QUOTIENT,
	CK_EXPRESSION_ASSIGN_REMAINDER,
	CK_EXPRESSION_ASSIGN_LEFT_SHIFT,
	CK_EXPRESSION_ASSIGN_RIGHT_SHIFT,
	CK_EXPRESSION_ASSIGN_AND,
	CK_EXPRESSION_ASSIGN_XOR,
	CK_EXPRESSION_ASSIGN_OR,

	CK_EXPRESSION_COMPOUND,

} CkExpressionKind;

/// <summary>
/// A parser expression.
/// </summary>
typedef struct CkExpression
{
	/// <summary>
	/// The main token of the expression.
	/// Usually an operator, or the literal
	/// value token with literal expressions.
	/// </summary>
	CkToken token;

	/// <summary>
	/// The kind of the expression (its operator.)
	/// This cannot be stored with the token, cause the
	/// main expression token of two operators can be the same.
	/// (e.g. both x++ and ++x have ++ has their main token.)
	/// </summary>
	CkExpressionKind kind;

	/// <summary>
	/// The type of the expression. This is a passed
	/// pointer. Ideally, this should be allocated on 
	/// the same arena as the expression to prevent
	/// object lifetime issues.
	/// </summary>
	CkFoodType *type;

	/// <summary>
	/// The left child node.
	/// </summary>
	struct CkExpression *left;

	/// <summary>
	/// The right child node.
	/// </summary>
	struct CkExpression *right;

	/// <summary>
	/// This child node is used to store the third
	/// member of ternary expressions.
	/// </summary>
	struct CkExpression *extra;

	/// <summary>
	/// If true, this expression can be referenced.
	/// </summary>
	bool_t isLValue;

} CkExpression;

#endif
