/*
 * Definitions, structs and functions for Fast Food
*/

#ifndef FFLIB_FFDEFS_H_
#define FFLIB_FFDEFS_H_

#include <ckmem/List.h>

/// <summary>
/// The ID of a type in Fast Food.
/// </summary>
typedef enum FFType
{
	/// <summary>
	/// Used for operations that don't return any value.
	/// </summary>
	FF_TYPE_NONE,

	FF_TYPE_I8,
	FF_TYPE_U8,
	FF_TYPE_I16,
	FF_TYPE_U16,
	FF_TYPE_I32,
	FF_TYPE_U32,
	FF_TYPE_I64,
	FF_TYPE_U64,

	FF_TYPE_F16,
	FF_TYPE_F32,
	FF_TYPE_F64,

	/// <summary>
	/// A pointer. Always the largest integer being able to be
	/// stored for a given platform.
	/// </summary>
	FF_TYPE_PTR,

} FFType;

/// <summary>
/// The opcodes of the Fast Food instructions.
/// </summary>
typedef enum FFOpcode
{
	/// <summary>
	/// nop
	/// </summary>
	FF_OP_NOP,

	/// <summary>
	/// t1 = t2
	/// </summary>
	FF_OP_COPY,

	/// <summary>
	/// *t1 = t2
	/// </summary>
	FF_OP_STORE,

	/// <summary>
	/// t1 = *t2
	/// </summary>
	FF_OP_DEREFERENCE,

	/// <summary>
	/// t1 = -t2
	/// </summary>
	FF_OP_NEGATE,

	/// <summary>
	/// t1 = !t2
	/// </summary>
	FF_OP_LOGICAL_NOT,

	/// <summary>
	/// t1 = ~t2
	/// </summary>
	FF_OP_BITWISE_NOT,

	/// <summary>
	/// t1 = &t2
	/// </summary>
	FF_OP_ADDRESS_OF,

	/// <summary>
	/// t1 = t2 * t3
	/// </summary>
	FF_OP_MUL,

	/// <summary>
	/// t1 = t2 / t3
	/// </summary>
	FF_OP_DIV,

	/// <summary>
	/// t1 = t2 % t3
	/// </summary>
	FF_OP_REM,

	/// <summary>
	/// t1 = t2 + t3
	/// </summary>
	FF_OP_ADD,

	/// <summary>
	/// t1 = t2 - t3
	/// </summary>
	FF_OP_SUB,

	/// <summary>
	/// t1 = t2 << t3
	/// </summary>
	FF_OP_LSHIFT,

	/// <summary>
	/// t1 = t2 >> t3
	/// </summary>
	FF_OP_RSHIFT,

	/// <summary>
	/// t1 = t2 < t3
	/// </summary>
	FF_OP_LOWER,

	/// <summary>
	/// t1 = t2 <= t3
	/// </summary>
	FF_OP_LOWER_EQUAL,

	/// <summary>
	/// t1 = t2 > t3
	/// </summary>
	FF_OP_GREATER,

	/// <summary>
	/// t1 = t2 >= t3
	/// </summary>
	FF_OP_GREATER_EQUAL,

	/// <summary>
	/// t1 = t2 == t3
	/// </summary>
	FF_OP_EQUAL,

	/// <summary>
	/// t1 = t2 != t3
	/// </summary>
	FF_OP_NOT_EQUAL,

	/// <summary>
	/// t1 = t2 & t3
	/// </summary>
	FF_OP_BITWISE_AND,

	/// <summary>
	/// t1 = t2 ^ t3
	/// </summary>
	FF_OP_BITWISE_XOR,

	/// <summary>
	/// t1 = t2 | t3
	/// </summary>
	FF_OP_BITWISE_OR,

	/// <summary>
	/// t1 = t2 && t3
	/// </summary>
	FF_OP_LOGICAL_AND,

	/// <summary>
	/// t1 = t2 || t3
	/// </summary>
	FF_OP_LOGICAL_OR,

	/// <summary>
	/// goto L1
	/// </summary>
	FF_OP_GOTO,

	/// <summary>
	/// goto L1 : t1
	/// </summary>
	FF_OP_CGOTO,

	/// <summary>
	/// t1 = L1 call
	/// </summary>
	FF_OP_CALL_FUNC,

	/// <summary>
	/// proc L1
	/// </summary>
	FF_OP_CALL_PROC,

	/// <summary>
	/// return t1
	/// </summary>
	FF_OP_RETURN,

	/// <summary>
	/// aload t1
	/// </summary>
	FF_OP_LOAD_ARG,

	/// <summary>
	/// astore t1
	/// </summary>
	FF_OP_STORE_ARG,

} FFOpcode;

/// <summary>
/// A scope can contain functions and variables.
/// </summary>
typedef struct FFScope
{
	/// <summary>
	/// The parent scope to this one. NULL means it is the global scope.
	/// Global scopes can declare modules and interfaces, but cannot declare
	/// variables.
	/// </summary>
	struct FFScope *parent;

	/// <summary>
	/// The list of the modules. Leave empty if not the global scope.
	/// </summary>
	CkList moduleList;

	/// <summary>
	/// The list of functions.
	/// </summary>
	CkList functionList;

	/// <summary>
	/// The list of variables.
	/// </summary>
	CkList variableList;

	/// <summary>
	/// The children scopes that belong to this scope.
	/// </summary>
	CkList children;

} FFScope;

/// <summary>
/// A module is a collection of functions and data that can be reused
/// multiple times in other libraries or programs.
/// </summary>
typedef struct FFModule
{
	/// <summary>
	/// If true, this module's functions will be exported
	/// publicly.
	/// </summary>
	bool_t export;

	/// <summary>
	/// The name of the module.
	/// </summary>
	char *name;

	/// <summary>
	/// The functions of the module.
	/// </summary>
	CkList functions;

	/// <summary>
	/// The static data associated with
	/// the module (variables, etc)
	/// </summary>
	CkList staticData;

	/// <summary>
	/// The instructions inside the module.
	/// </summary>
	void *instructions;

} FFModule;

/// <summary>
/// A function inside a module.
/// </summary>
typedef struct FFFunction
{
	/// <summary>
	/// The name of the function symbol.
	/// </summary>
	char *name;

	/// <summary>
	/// The base index of the function.
	/// </summary>
	size_t base;

} FFFunction;

/// <summary>
/// A Fast Food instruction.
/// </summary>
typedef struct FFInstruction
{
	/// <summary>
	/// The opcode of the instruction.
	/// </summary>
	FFOpcode op;

} FFInstruction;

#endif
