#include <Food.h>
#include <CDebug.h>

CkFoodType *CkFoodCreateTypeInstance(
	CkArenaFrame *arena,
	FoodTypeID id,
	uint8_t qualifiers,
	CkFoodType *child )
{
	CkFoodType *instance;

	CK_ARG_NON_NULL( arena );

	instance = CkArenaAllocate( arena, sizeof( CkFoodType ) );
	instance->id = id;
	instance->qualifiers = qualifiers;
	instance->child = child;
	return instance;
}

CkFoodType *CkFoodCopyTypeInstance( CkArenaFrame *arena, CkFoodType *instance )
{
	CkFoodType *yield;
	CK_ARG_NON_NULL( arena );
	CK_ARG_NON_NULL( instance );

	yield = CkFoodCreateTypeInstance(
		arena,
		instance->id,
		instance->qualifiers,
		instance->child ? CkFoodCopyTypeInstance( arena, instance->child ) : NULL
	);
	yield->extra = instance->extra;
	return yield;
}
