#include "Intern.h"
#include <Food.h>

#include <string.h>

CkFoodType *ResolveScopedSymType(CkExpression *path, CkAnalyzeConfig *cfg, CkScope *scope)
{
	// TODO
	return NULL;
}

CkFoodType *ResolveSymType(char *ident, CkAnalyzeConfig *cfg, CkScope *scope)
{
	// ----- Look in variables -----

	FOREACH(scope->variableList, node_var) {
		CkVariable *v = ITEMPTR(CkVariable, node_var);
		if (!strcmp(ident, v->name)) return CkFoodCopyTypeInstance(cfg->allocator, v->type);
	}

	// ----- Look in functions -----

	if (scope->supportsFunctions)
	FOREACH(scope->functionList, node_func) {
		CkFunction *f = ITEMPTR(CkFunction, node_func);
		if (!strcmp(ident, f->name)) return CkFoodCopyTypeInstance(cfg->allocator, f->signature);
	}

	// ----- Look in parent or return null -----
	if (scope->parent) return ResolveSymType(ident, cfg, scope->parent);
	return NULL;
}
