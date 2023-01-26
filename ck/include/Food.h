/*
 * Defines various structures, macros and functions
 * for the Food programming language implementation.
*/

#ifndef CK_FOOD_H_
#define CK_FOOD_H_

#include "Types.h"

#define CK_QUALIFIER_CONST_BIT    1
#define CK_QUALIFIER_VOLATILE_BIT 2
#define CK_QUALIFIER_RESTRICT_BIT 4
#define CK_QUALIFIER_ATOMIC_BIT   8

enum
{
	CK_FOOD_VOID = 1,
	CK_FOOD_BOOL,
	CK_FOOD_I8,
	CK_FOOD_U8,
	CK_FOOD_I16,
	CK_FOOD_U16,
	CK_FOOD_F16,
	CK_FOOD_I32,
	CK_FOOD_U32,
	CK_FOOD_F32,
	CK_FOOD_I64,
	CK_FOOD_U64,
	CK_FOOD_F64,

	CK_FOOD_POINTER,
	CK_FOOD_FUNCPOINTER,
	CK_FOOD_REFERENCE,

	CK_FOOD_STRUCT,
	CK_FOOD_UNION,
	CK_FOOD_ENUM,
	CK_FOOD_ARRAY,

	CK_FOOD_STRING,
};

/// <summary>
/// Represents a type in the Food programming language.
/// Use CkFoodCreateTypeInstance() to create,
/// and CkFoodDeleteTypeInstance() to delete.
/// </summary>
typedef struct CkFoodType
{
	/// <summary>
	/// The type identifier. Must be equal to one of the
	/// CK_FOOD_(typename) macros.
	/// </summary>
	uint8_t id;

	/// <summary>
	/// A bit array storing the type qualifiers.
	/// </summary>
	uint8_t  qualifiers;

	/// <summary>
	/// Types can have subtypes. This points to this
	/// type's direct subtype.
	/// </summary>
	struct CkFoodType *child;

} CkFoodType;

/// <summary>
/// Allocates and creates a new type instance on the heap.
/// </summary>
/// <param name="id">The type identifier. Must be equal to one of the CK_FOOD_(typename) macros.</param>
/// <param name="qualifiers">A bit array storing the type qualifiers.</param>
/// <param name="child">(optional) The child type node. Set this arg to NULL if no child node is present. The child
/// is now owned by this node.</param>
/// <returns>A heap-allocated type instance.</returns>
CkFoodType *CkFoodCreateTypeInstance(
	uint8_t id,
	uint8_t qualifiers,
	CkFoodType *child);

/// <summary>
/// Deletes and frees a type instance on the heap.
/// </summary>
/// <param name="instance">The instance to delete.</param>
void CkFoodDeleteTypeInstance(CkFoodType *instance);

/// <summary>
/// Duplicates a type instance.
/// </summary>
/// <param name="instance">The instance to duplicate.</param>
/// <returns></returns>
CkFoodType *CkFoodCopyTypeInstance(CkFoodType *instance);

#endif
