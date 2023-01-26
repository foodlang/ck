#include <include/Food.h>
#include <malloc.h>

CkFoodType *CkFoodCreateTypeInstance(
	uint8_t id,
	uint8_t qualifiers,
	CkFoodType *child)
{
	CkFoodType *instance = malloc(sizeof(CkFoodType));
	instance->id = id;
	instance->qualifiers = qualifiers;
	instance->child = child;
	return instance;
}

void CkFoodDeleteTypeInstance(CkFoodType *instance)
{
	if (instance->child)
		CkFoodDeleteTypeInstance(instance->child);
	free(instance);
}

CkFoodType *CkFoodCopyTypeInstance(CkFoodType *instance)
{
	return CkFoodCreateTypeInstance(
		instance->id,
		instance->qualifiers,
		instance->child ? CkFoodCopyTypeInstance(instance->child) : NULL
	);
}
