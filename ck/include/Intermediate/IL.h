/*
 * Defines structures and helper functions for Food's IL.
*/

#ifndef CK_IL_H_
#define CK_IL_H_

#include <ckmem/Types.h>
#include "../Food.h"
#include <ckmem/Queue.h>
#include <ckmem/List.h>

/// <summary>
/// The opcode of the instruction.
/// </summary>
typedef enum CkTACInstructionOpcode
{
	/// <summary>
	/// nop :: Does nothing.
	/// </summary>
	CK_TAC_NOP,

	/// <summary>
	/// t1 = t2 :: Copies data from a reference to another.
	/// </summary>
	CK_TAC_COPY,

	/// <summary>
	/// *t1 = t2 :: Copies data from a reference, and the destination reference is interpreted as a pointer.
	/// </summary>
	CK_TAC_STORE,

	/// <summary>
	/// t1 = -t2 :: Negates a value.
	/// </summary>
	CK_TAC_NEGATE,

	/// <summary>
	/// t1 = !t2 :: Stores the logical NOT of a reference.
	/// </summary>
	CK_TAC_LOGICAL_NOT,

	/// <summary>
	/// t1 = ~t2 :: Stores the bitwise NOT of a reference.
	/// </summary>
	CK_TAC_BITWISE_NOT,

	/// <summary>
	/// t1 = &t2 :: Stores the address of a reference (if can be referenced.)
	/// </summary>
	CK_TAC_ADDRESS_OF,

	/// <summary>
	/// t1 = *t2 :: Stores the dereferenced value at a reference (as an address.)
	/// </summary>
	CK_TAC_DEREFERENCE,

	/// <summary>
	/// t1 = t2 * t3 :: Multiplies two references and stores the result in another.
	/// </summary>
	CK_TAC_MUL,

	/// <summary>
	/// t1 = t2 / t3 :: Divides a reference by another and stores the result in another.
	/// </summary>
	CK_TAC_DIV,

	/// <summary>
	/// t1 = t2 % t3 :: Divides a reference by another and stores the result 
	/// </summary>
	CK_TAC_REM,

	/// <summary>
	/// t1 = t2 + t3 :: Adds two references and stores the result in another.
	/// </summary>
	CK_TAC_ADD,

	/// <summary>
	/// t1 = t2 - t3 :: Subtracts a reference from another and stores the result in another.
	/// </summary>
	CK_TAC_SUB,

	/// <summary>
	/// t1 = t2 << t3 :: Shifts a reference by another (left)
	/// </summary>
	CK_TAC_LSHIFT,

	/// <summary>
	/// t1 = t2 >> t3 :: Shifts a reference by another (right)
	/// </summary>
	CK_TAC_RSHIFT,

	/// <summary>
	/// t1 = t2 < t3
	/// </summary>
	CK_TAC_LOWER,

	/// <summary>
	/// t1 = t2 <= t3
	/// </summary>
	CK_TAC_LOWER_EQUAL,

	/// <summary>
	/// t1 = t2 > t3
	/// </summary>
	CK_TAC_GREATER,

	/// <summary>
	/// t1 = t2 >= t3
	/// </summary>
	CK_TAC_GREATER_EQUAL,

	/// <summary>
	/// t1 = t2 == t3
	/// </summary>
	CK_TAC_EQUAL,

	/// <summary>
	/// t1 = t2 != t3
	/// </summary>
	CK_TAC_NOT_EQUAL,

	/// <summary>
	/// t1 = t2 & t3
	/// </summary>
	CK_TAC_BITWISE_AND,

	/// <summary>
	/// t1 = t2 ^ t3
	/// </summary>
	CK_TAC_BITWISE_XOR,

	/// <summary>
	/// t1 = t2 | t3
	/// </summary>
	CK_TAC_BITWISE_OR,

	/// <summary>
	/// t1 = t2 && t3
	/// </summary>
	CK_TAC_LOGICAL_AND,

	/// <summary>
	/// t1 = t2 || t3
	/// </summary>
	CK_TAC_LOGICAL_OR,

	/// <summary>
	/// goto l1
	/// </summary>
	CK_TAC_GOTO,

	/// <summary>
	/// goto l1 : t1 (Conditional goto)
	/// </summary>
	CK_TAC_CGOTO,

	/// <summary>
	/// t1 = l1 call
	/// </summary>
	CK_TAC_CALL_FUNC,

	/// <summary>
	/// proc l1
	/// </summary>
	CK_TAC_CALL_PROC,

	/// <summary>
	/// return t1
	/// </summary>
	CK_TAC_RETURN,

	/// <summary>
	/// aload t1 (Loads an argument for a function call.)
	/// </summary>
	CK_TAC_LOAD_ARG,

} CkTACInstructionOpcode;

/// <summary>
/// The kind of the reference.
/// </summary>
typedef enum CkTACReferenceKind
{
	/// <summary>
	/// An external symbol, to be linked later after the compiler
	/// has finished generating code.
	/// </summary>
	CK_TAC_EXTERNAL_SYMBOL,

	/// <summary>
	/// An external symbol that can be found in a library's public
	/// symbol table.
	/// </summary>
	CK_TAC_EXTERNAL_FOOD_SYMBOL,

	/// <summary>
	/// An internal symbol.
	/// </summary>
	CK_TAC_SYMBOL,

	/// <summary>
	/// A variable.
	/// </summary>
	CK_TAC_VARIABLE,

	/// <summary>
	/// A value that is only used temporarily.
	/// </summary>
	CK_TAC_TEMP,

	/// <summary>
	/// A constant value.
	/// </summary>
	CK_TAC_CONST,

} CkTACReferenceKind;

/// <summary>
/// The type of storage that contains the reference.
/// </summary>
typedef enum CkTACReferenceStorage
{
	/// <summary>
	/// The reference is stored in a module.
	/// </summary>
	CK_TAC_MODULE,

	/// <summary>
	/// The reference is stored in a function.
	/// This also applies to local functions.
	/// </summary>
	CK_TAC_FUNC,

} CkTACReferenceStorage;

/// <summary>
/// A module declaration.
/// </summary>
typedef struct CkTACModuleDecl
{
	/// <summary>
	/// An index to the mdoule table corresponding to
	/// the module definition.
	/// </summary>
	uint64_t name;

	/// <summary>
	/// If true, the function will be exposed to other libraries
	/// or programs.
	/// </summary>
	bool_t public;

	/// <summary>
	/// The functions of the module.
	/// </summary>
	CkList functions;

	/// <summary>
	/// The references in the module.
	/// </summary>
	CkList references;

} CkTACModuleDecl;

/// <summary>
/// A function declaration.
/// </summary>
typedef struct CkTACFunctionDecl
{
	/// <summary>
	/// A list storing all of the instructions.
	/// </summary>
	CkList instructions;

	/// <summary>
	/// An index to the symbol table corresponding to the
	/// function definition.
	/// </summary>
	uint64_t name;

	/// <summary>
	/// If true, the function will be exposed to other modules.
	/// </summary>
	bool_t public;

	/// <summary>
	/// Stores a list of all of the local references, including
	/// variables.
	/// </summary>
	CkList localReferences;

	/// <summary>
	/// Stores a list of all of the local labels. This exclude
	/// this function's call label.
	/// </summary>
	CkList localLabels;

} CkTACFunctionDecl;

/// <summary>
/// A reference to a symbol, a variable or an intermediate value.
/// </summary>
typedef struct CkTACReference
{
	/// <summary>
	/// The kind of the reference.
	/// </summary>
	CkTACReferenceKind kind;

	/// <summary>
	/// The type of the reference.
	/// </summary>
	CkFoodType *type;

	/// <summary>
	/// The name of the reference. If it is a symbol, this is an
	/// index to its entry in the function table.
	/// </summary>
	uint64_t name;

	/// <summary>
	/// The storage location of the reference. This is used to
	/// emit names properly.
	/// </summary>
	CkTACReferenceStorage storage;

	/// <summary>
	/// This union stores a pointer to where to the owner of the
	/// reference. If the storage is a module and the pointer is
	/// null, it is stored globally in the program.
	/// </summary>
	union {
		/// <summary>
		/// The owner as a function.
		/// </summary>
		CkTACFunctionDecl *func;
	} pOwner;

} CkTACReference;

/// <summary>
/// An instruction in TAC form.
/// </summary>
typedef struct CkTACInstruction
{
	/// <summary>
	/// The instruction.
	/// </summary>
	CkTACInstructionOpcode opcode;

	/// <summary>
	/// The destination reference.
	/// </summary>
	CkTACReference *dest;

	/// <summary>
	/// The first operand of the instruction.
	/// </summary>
	CkTACReference *op1;

	/// <summary>
	/// The second operand of the instruction.
	/// </summary>
	CkTACReference *op2;

} CkTACInstruction;

/// <summary>
/// Allocates a new function declaration.
/// </summary>
/// <param name="public">If this true, the function is public and can be exported to the API of the library.</param>
/// <param name="name">The name of the function.</param>
/// <param name="IQSize">The size (in instructions) of the instruction queue.</param>
/// <param name="LRSize"></param>
/// <param name="LLSize"></param>
/// <returns></returns>
CkTACFunctionDecl *CkTACAllocFunction(bool_t public, char *name, size_t IQSize, size_t LRSize, size_t LLSize);

/// <summary>
/// Allocates a local reference.
/// </summary>
/// <param name="func">The function scope responsible for this reference.</param>
/// <param name="usage">The purpose of the reference.</param>
/// <returns></returns>
CkTACReference *CkTACAllocLocalRef(CkTACFunctionDecl *func, CkTACReferenceKind usage, CkFoodType *type);

/// <summary>
/// Frees a local reference. This reference must be a temporary reference.
/// </summary>
/// <param name="func">The function scope responsible for this reference.</param>
/// <param name="ref">The temporary reference to free.</param>
void CkTACFreeLocalRef(CkTACFunctionDecl *func, CkTACReference *ref);

#endif
