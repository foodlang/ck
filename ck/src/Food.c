#include <include/Food.h>
#include <include/CDebug.h>

CkFoodType *CkFoodCreateTypeInstance(
	CkArenaFrame *arena,
	uint8_t id,
	uint8_t qualifiers,
	CkFoodType *child)
{
	CkFoodType *instance;

	CK_ARG_NON_NULL(arena)

	instance = CkArenaAllocate(arena, sizeof(CkFoodType));
	instance->id = id;
	instance->qualifiers = qualifiers;
	instance->child = child;
	return instance;
}

CkFoodType *CkFoodCopyTypeInstance(CkArenaFrame *arena, CkFoodType *instance)
{
	CK_ARG_NON_NULL(arena)
	CK_ARG_NON_NULL(instance)

	return CkFoodCreateTypeInstance(
		arena,
		instance->id,
		instance->qualifiers,
		instance->child ? CkFoodCopyTypeInstance(arena, instance->child) : NULL
	);
}
