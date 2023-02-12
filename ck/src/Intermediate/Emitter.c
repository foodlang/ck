#include <include/Intermediate/Emitter.h>

#define DEST_SOURCE(x, y) void x(CkTACFunctionDecl *func, CkTACReference *dest, CkTACReference *source) \
{ \
	CkTACInstruction instruction; \
	instruction.opcode = y; \
	instruction.dest = dest; \
	instruction.op1 = source; \
	CkListAdd(&func->instructions, &instruction); \
}

#define DEST_SOURCE2(x, y) void x(CkTACFunctionDecl *func, CkTACReference *dest, CkTACReference *source1, CkTACReference *source2) \
{ \
	CkTACInstruction instruction; \
	instruction.opcode = y; \
	instruction.dest = dest; \
	instruction.op1 = source1; \
	instruction.op2 = source2; \
	CkListAdd(&func->instructions, &instruction); \
}

void CkEmitNOP(CkTACFunctionDecl *func)
{
	CkTACInstruction instruction;
	instruction.opcode = CK_TAC_NOP;
	CkListAdd(&func->instructions, &instruction);
}

DEST_SOURCE(CkEmitCopy, CK_TAC_COPY)
DEST_SOURCE(CkEmitStore, CK_TAC_STORE)
DEST_SOURCE(CkEmitNegate, CK_TAC_NEGATE)
DEST_SOURCE(CkEmitLogicalNot, CK_TAC_LOGICAL_NOT)
DEST_SOURCE(CkEmitBitwiseNot, CK_TAC_BITWISE_NOT)
DEST_SOURCE(CkEmitAddressOf, CK_TAC_ADDRESS_OF)
DEST_SOURCE(CkEmitDereference, CK_TAC_DEREFERENCE)

DEST_SOURCE2(CkEmitMul, CK_TAC_MUL)
DEST_SOURCE2(CkEmitDiv, CK_TAC_DIV)
DEST_SOURCE2(CkEmitRem, CK_TAC_REM)
DEST_SOURCE2(CkEmitAdd, CK_TAC_ADD)
DEST_SOURCE2(CkEmitSub, CK_TAC_SUB)
DEST_SOURCE2(CkEmitLShift, CK_TAC_LSHIFT)
DEST_SOURCE2(CkEmitRShift, CK_TAC_RSHIFT)
DEST_SOURCE2(CkEmitLower, CK_TAC_LOWER)
DEST_SOURCE2(CkEmitLowerEqual, CK_TAC_LOWER_EQUAL)
DEST_SOURCE2(CkEmitGreater, CK_TAC_GREATER)
DEST_SOURCE2(CkEmitGreaterEqual, CK_TAC_GREATER_EQUAL)
DEST_SOURCE2(CkEmitEqual, CK_TAC_EQUAL)
DEST_SOURCE2(CkEmitNotEqual, CK_TAC_NOT_EQUAL)
DEST_SOURCE2(CkEmitBitwiseAND, CK_TAC_BITWISE_AND)
DEST_SOURCE2(CkEmitBitwiseXOR, CK_TAC_BITWISE_XOR)
DEST_SOURCE2(CkEmitBitwiseOR, CK_TAC_BITWISE_OR)

void CkEmitGoto(
	CkTACFunctionDecl *func,
	CkTACReference *label)
{
	CkTACInstruction instruction;
	instruction.opcode = CK_TAC_GOTO;
	instruction.dest = label;
	CkListAdd(&func->instructions, &instruction);
}

void CkEmitConditionalGoto(
	CkTACFunctionDecl *func,
	CkTACReference *label,
	CkTACReference *condition)
{
	CkTACInstruction instruction;
	instruction.opcode = CK_TAC_CGOTO;
	instruction.dest = label;
	instruction.op1 = condition;
	CkListAdd(&func->instructions, &instruction);
}

void CkEmitCallFunc(
	CkTACFunctionDecl *func,
	CkTACReference *dest,
	CkTACReference *sym)
{
	CkTACInstruction instruction;
	instruction.opcode = CK_TAC_CALL_FUNC;
	instruction.dest = dest;
	instruction.op1 = sym;
	CkListAdd(&func->instructions, &instruction);
}

void CkEmitCallProc(
	CkTACFunctionDecl *func,
	CkTACReference *sym)
{
	CkTACInstruction instruction;
	instruction.opcode = CK_TAC_CALL_PROC;
	instruction.dest = sym;
	CkListAdd(&func->instructions, &instruction);
}

void CkEmitReturn(
	CkTACFunctionDecl *func,
	CkTACReference *returnRef)
{
	CkTACInstruction instruction;
	instruction.opcode = CK_TAC_RETURN;
	instruction.dest = returnRef;
	CkListAdd(&func->instructions, &instruction);
}

void CkEmitExpression(
	CkTACFunctionDecl *func,
	CkExpression *expression,
	CkTACReference *result)
{
	(void)func;
	(void)expression;
	(void)result;
}
