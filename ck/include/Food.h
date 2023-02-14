/*
 * Defines various structures, macros and functions
 * for the Food programming language implementation.
*/

#ifndef CK_FOOD_H_
#define CK_FOOD_H_

#include <ckmem/Types.h>
#include <ckmem/Arena.h>

/// <summary>
/// Allocates and creates a new type instance on the heap.
/// </summary>
/// <param name="id">The type identifier. Must be equal to one of the CK_FOOD_(typename) macros.</param>
/// <param name="qualifiers">A bit array storing the type qualifiers.</param>
/// <param name="child">(optional) The child type node. Set this arg to NULL if no child node is present. The child
/// is now owned by this node.</param>
/// <returns>A heap-allocated type instance.</returns>
CkFoodType *CkFoodCreateTypeInstance(
	CkArenaFrame *arena,
	FoodTypeID id,
	uint8_t qualifiers,
	CkFoodType *child);

/// <summary>
/// Duplicates a type instance.
/// </summary>
/// <param name="instance">The instance to duplicate.</param>
/// <returns></returns>
CkFoodType *CkFoodCopyTypeInstance(CkArenaFrame *arena, CkFoodType *instance);

#endif
