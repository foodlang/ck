/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the internal analyzer symbols.
 *
 ***************************************************************************/

#ifndef CK_ANALYZER_INTERN_H_
#define CK_ANALYZER_INTERN_H_

#include <Analyzer.h>

size_t AnalyzeExpression(CkExpression *expr, CkAnalyzeConfig *cfg, CkScope *scope);
size_t AnalyzeStatement(CkStatement *stmt, CkAnalyzeConfig *cfg, CkScope *scope, bool *mut_return_found);
size_t AnalyzeFunc(CkFunction *func, CkAnalyzeConfig *cfg);
CkFoodType *ResolveSymType(char *ident, CkAnalyzeConfig *cfg, CkScope *scope);

#endif
