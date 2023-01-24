#include <include/Food.h>
#include <malloc.h>

CkFoodType *CkFoodCreateTypeInstance(
	uint32_t id,
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
