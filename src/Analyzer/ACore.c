#include "Intern.h"

static size_t _AnalyzeLib(CkLibrary *lib, CkAnalyzeConfig *cfg)
{
	size_t bindings = 0; // Binding count

	// Binding modules
	FOREACH(lib->moduleList, mod) {
		CkModule *m = ITEM(CkModule *, mod);
		FOREACH(m->scope->functionList, func) bindings += AnalyzeFunc((CkFunction *)(func + 1), cfg);
	}

	// Binding top functions
	FOREACH(lib->scope->functionList, func) bindings += AnalyzeFunc((CkFunction *)(func + 1), cfg);

	/*
	* check: maybe verify variables?
	* shouldn't cause variables but yeah
	* - \e, 5/14/2023
	*/

	return bindings;
}

size_t CkAnalyze(CkList *libs/* elem type=CkLibrary* */, CkAnalyzeConfig *cfg)
{
	size_t bindings = 0; // Binding count

	// Binding libraries
	FOREACH(libs, lib) bindings += _AnalyzeLib(ITEM(CkLibrary *, lib), cfg);

	return bindings;
}
