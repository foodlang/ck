/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the semantic analyzer module.
 *
 ***************************************************************************/

#ifndef CK_ANALYZER_H_
#define CK_ANALYZER_H_

#include "Types.h"
#include "IL/FFStruct.h"

// Analyzes a library. Returns the number of type bindings performed. If 0, done binding.
size_t CkAnalyze( CkList *libs/* elem type=CkLibrary* */, CkArenaFrame *allocator );

#endif
