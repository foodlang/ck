/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares various functions concerning Food-related types.
 * These types are used to represent some of the front-end & back-end syntax
 * (such as FoodType.)
 *
 ***************************************************************************/

#ifndef CK_FOOD_H_
#define CK_FOOD_H_

#include <Types.h>
#include <Memory/Arena.h>
#include <Memory/List.h>

// Allocates and creates a new type instance on the heap.
CkFoodType *CkFoodCreateTypeInstance(
	CkArenaFrame *arena,
	FoodTypeID id,
	uint8_t qualifiers,
	CkFoodType *child);

// Duplicates a type instance.
CkFoodType *CkFoodCopyTypeInstance(CkArenaFrame *arena, CkFoodType *instance);

// A function's signature.
typedef struct CkFuncSignature
{
	// The return type.
	CkFoodType *tReturn;

	// The arguments' types. Element type = CkFoodType*
	CkList *args;

} CkFuncSignature;

#endif
