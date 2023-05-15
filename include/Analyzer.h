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
#include "Diagnostics.h"
#include "IL/FFStruct.h"

// Analysis config
typedef struct CkAnalyzeConfig
{
	CkArenaFrame            *allocator; // The arena
	CkDiagnosticHandlerInstance *p_dhi; // The diagnostics handler

} CkAnalyzeConfig;

// Analyzes a library. Returns the number of type bindings performed. If 0, done binding.
size_t CkAnalyze( CkList *libs/* elem type=CkLibrary* */, CkAnalyzeConfig *cfg );

static inline void CkAnalyzeFull(CkList *libs, CkAnalyzeConfig *cfg)
{
	while (true) {
		size_t n = 0;
		n = CkAnalyze(libs, cfg);
		if (!n) break;
	}
}

#endif
