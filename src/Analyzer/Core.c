#include <Analyzer.h>

extern size_t AnalyzeFunc( CkFunction *func, CkArenaFrame *allocator );

static size_t _AnalyzeLib( CkLibrary *lib, CkArenaFrame *allocator )
{
	size_t bindings = 0; // Binding count

	// Binding modules
	FOREACH( lib->moduleList, mod ) {
		CkModule *m = ITEM( CkModule *, mod );
		FOREACH( m->scope->functionList, func ) AnalyzeFunc( ITEM( CkFunction *, func ), alocator );
	}

	return bindings;
}

size_t CkAnalyze( CkList *libs/* elem type=CkLibrary* */, CkArenaFrame *allocator )
{
	size_t bindings = 0; // Binding count

	// Binding libraries
	FOREACH(libs, lib) bindings += _AnalyzeLib( ITEM(CkLibrary *, lib), allocator);

	return bindings;
}