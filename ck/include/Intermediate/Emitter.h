/*
 * Provides functions to emit Food IL.
*/

#ifndef CK_EMITTER_H_
#define CK_EMITTER_H_

#include "../Syntax/Expression.h"
#include "IL.h"

/// <summary>
/// Emits a NOP instruction.
/// </summary>
/// <param name="func"></param>
void CkEmitNOP(CkTACFunctionDecl *func);

/// <summary>
/// Emits a copy instruction (dest = source)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source"></param>
void CkEmitCopy(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits a store instruction (*dest = source)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source"></param>
void CkEmitStore(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits a negate instruction (t1 = -t2)
/// </summary>
/// <param name="func"></param>
/// <param name="op"></param>
void CkEmitNegate(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits a logical not instruction (t1 = !t2)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source"></param>
void CkEmitLogicalNot(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits a bitwise not instruction (t1 = ~t2)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source"></param>
void CkEmitBitwiseNot(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits an Address-Of instruction (t1 = &t2).
/// t2 must be an l-value.
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source"></param>
void CkEmitAddressOf(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits a dereference instruction (t1 = *t2)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source"></param>
void CkEmitDereference(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source);

/// <summary>
/// Emits a multiplication (t1 = t2 * t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitMul(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a division (t1 = t2 / t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitDiv(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a division, but stores the remainder (t1 = t2 % t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitRem(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits an add instruction (t1 = t2 + t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitAdd(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a subtraction instruction (t1 = t2 - t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitSub(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a left shift instruction (t1 = t2 << t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitLShift(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a right shift instruction (t1 = t2 >> t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitRShift(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a lower comparison instruction (t1 = t2 < t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitLower(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a lower equal comparison instruction (t1 = t2 <= t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitLowerEqual(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a greater comparison instruction (t1 = t2 > t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitGreater(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a greater equal comparison instruction (t1 = t2 >= t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitGreaterEqual(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits an equal comparison instruction (t1 = t2 == t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitEqual(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a not equal comparison instruction (t1 = t2 != t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitNotEqual(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a bitwise AND instruction (t1 = t2 & t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitBitwiseAND(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a bitwise XOR instruction (t1 = t2 ^ t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitBitwiseXOR(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a bitwise OR instruction (t1 = t2 | t3)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="source1"></param>
/// <param name="source2"></param>
void CkEmitBitwiseOR(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *source1,
	CkTACReference *source2);

/// <summary>
/// Emits a goto instruction (goto l1)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="label"></param>
void CkEmitGoto(
	CkTACFunctionDecl *func,
	CkTACReference *label);

/// <summary>
/// Emits a conditional goto instruction (goto l1 : t1)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="label"></param>
/// <param name="condition"></param>
void CkEmitConditionalGoto(
	CkTACFunctionDecl *func,
	CkTACReference *label,
	CkTACReference *condition);

/// <summary>
/// Emits an instruction to call a function (t1 = l1 call)
/// </summary>
/// <param name="func"></param>
/// <param name="dest"></param>
/// <param name="sym"></param>
void CkEmitCallFunc(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *sym);

/// <summary>
/// Emits an instruction to call a procedure (proc l1). Procedures
/// are functions without return results.
/// </summary>
/// <param name="func"></param>
/// <param name="sym"></param>
void CkEmitCallProc(
	CkTACFunctionDecl *func,
	CkTACReference *sym);

/// <summary>
/// Emits a return instruction (return t1)
/// </summary>
/// <param name="func"></param>
/// <param name="returnRef"></param>
void CkEmitReturn(
	CkTACFunctionDecl *func,
	CkTACReference *returnRef);

/// <summary>
/// Emits an expression.
/// </summary>
/// <param name="expression">The expression to emit.</param>
/// <param name="result">A pointer to the reference descriptor. This is where the result of the expression is stored.</param>
void CkEmitExpression(
	CkTACFunctionDecl *func,
	CkExpression *expression,
	CkTACReference *result);

#endif
