#include <include/Intermediate/IL.h>

CkTACReference *CkTACAllocLocalRef(CkTACFunctionDecl *func, CkTACReferenceKind usage, CkFoodType *type)
{
	CkTACReference tempRefBuffer;
	tempRefBuffer.kind = usage;
	tempRefBuffer.type = type;
	tempRefBuffer.name = func->localReferences.elemCount;
	return NULL; // >>>SPONGE<<<
}
