/***************************************************************************
 * 
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 * 
 * This header provides basic typedefs and constants used by the Food compiler.
 * It also declares global macros.
 * 
 ***************************************************************************/

#ifndef CK_TYPES_H_
#define CK_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <math.h>

#ifdef __GNUC__
#define ALIGN(x) __attribute__(( __aligned__(x) ))
#define PACKED __attribute__((packed))
#else
#define ALIGN(x) __declspec(align(x))
#define PACKED __declspec(align(1))
#endif

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

// Compares two floats for approximal equality.
static inline bool_t FloatEqual(double a, double b)
{
	const double diff = fmax(a, b) - fmin(a, b);

	return diff < 0.00005 || diff > 0.00005;
}

// 256-bit integers
typedef struct m256_t
{
	uint64_t low;
	uint64_t high;

} PACKED m256_t;

#define CK_QUALIFIER_CONST_BIT    1
#define CK_QUALIFIER_VOLATILE_BIT 2
#define CK_QUALIFIER_RESTRICT_BIT 4
#define CK_QUALIFIER_ATOMIC_BIT   8

// Dynamic string
typedef struct CkStrBuilder
{
	// The base of the string.
	char *base;

	// The length of the string.
	size_t length;

	// The current allocated length of the string.
	size_t capacity;

	// The size of allocations/reallocations.
	size_t blksize;

} CkStrBuilder;

// Represents a token, an indivisible bit of text that is used to
// represent the syntax of the source code.
typedef struct CkToken
{

	// Where the token is located.
	size_t position;

	// The kind of the token.
	uint64_t kind;

	// An additional value stored with the token.
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
	CK_FOOD_USER,
} FoodTypeID;

// Represents a type in the Food programming language.
// Use CkFoodCreateTypeInstance() to create,
// and CkFoodDeleteTypeInstance() to delete.
typedef struct CkFoodType
{
	// The type identifier. Must be equal to one of the
	// CK_FOOD_(typename) macros.
	FoodTypeID id;

	// A bit array storing the type qualifiers.
	uint8_t  qualifiers;

	// Types can have subtypes. This points to this
	// type's direct subtype.
	struct CkFoodType *child;

	// Extra type data (for example, function signature)
	void *extra;

} CkFoodType;

// The kind of an expression (its operator.)
typedef enum CkExpressionKind
{

	// Misc. unknown expression identifier
	CK_EXPRESSION_DUMMY,

	CK_EXPRESSION_IDENTIFIER,
	CK_EXPRESSION_SCOPED_REFERENCE, // A::B::C...
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
	CK_EXPRESSION_OPAQUE_ADDRESS_OF,
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

// A parser expression.
typedef struct CkExpression
{
	// The main token of the expression.
	// Usually an operator, or the literal
	// value token with literal expressions.
	CkToken token;

	// The kind of the expression (its operator.)
	// This cannot be stored with the token, cause the
	// main expression token of two operators can be the same.
	// (e.g. both x++ and ++x have ++ has their main token.)
	CkExpressionKind kind;

	// The type of the expression. This is a passed
	// pointer. Ideally, this should be allocated on 
	// the same arena as the expression to prevent
	// object lifetime issues.
	CkFoodType *type;

	// The left child node.
	struct CkExpression *left;

	// The right child node.
	struct CkExpression *right;

	// This child node is used to store the third
	// member of ternary expressions.
	struct CkExpression *extra;

	// If true, this expression can be referenced.
	bool_t isLValue;

	// Whether the expression is constant or not.
	bool_t isConstant;

} CkExpression;

#define CK_TYPE_CLASSED_INT(x) ((x) == CK_FOOD_I8 || (x) == CK_FOOD_U8 || (x) == CK_FOOD_I16 || (x) == CK_FOOD_U16 || \
							 (x) == CK_FOOD_I32 || (x) == CK_FOOD_U32 || (x) == CK_FOOD_I64 || (x) == CK_FOOD_U64 || \
							 (x) == CK_FOOD_ENUM)

#define CK_TYPE_CLASSED_FLOAT(x) ((x) == CK_FOOD_F16 || (x) == CK_FOOD_F32 || (x) == CK_FOOD_F64)

#define CK_TYPE_CLASSED_INTFLOAT(x) ((x) >= CK_FOOD_I8 && (x) <= CK_FOOD_F64)

#define CK_TYPE_CLASSED_POINTER(x) ((x) == CK_FOOD_POINTER || (x) == CK_FOOD_FUNCPOINTER || (x) == CK_FOOD_ARRAY)
#define CK_TYPE_CLASSED_POINTER_ARITHM(x) ((x) == CK_FOOD_POINTER || (x) == CK_FOOD_ARRAY)

// Creates a new string builder ready to be used.
// blksize = initial capacity, number of bytes to allocate
// every time the max. capacity is reached.
void CkStrBuilderCreate( CkStrBuilder *dest, size_t blksize );

// Appends a single character to the string builder.
void CkStrBuilderAppendChar( CkStrBuilder *sb, char c );

// Appends a string to the string builder.
void CkStrBuilderAppendString( CkStrBuilder *sb, char* s );

// Disposes of a string builder. These objects don't use arenas,
// so it is important to dispose of them.
void CkStrBuilderDispose( CkStrBuilder *sb );

#endif
