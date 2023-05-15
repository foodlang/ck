#include "Intern.h"

// Analyzes a function.
size_t AnalyzeFunc(CkFunction *func, CkAnalyzeConfig *cfg)
{
	size_t bindings = 0;       // Count of bindings performed
	bool found_return = false; // Whether a return was found in the function

	// ----- Analyze statements -----

	// TODO: Detect unused variables & params

	if (func->body->stmt == CK_STMT_EXPRESSION)
		bindings = AnalyzeExpression(func->body->data.expression, cfg, func->funscope);
	else
		bindings = AnalyzeStatement(func->body, cfg, func->body->data.block.scope, &found_return);

	// ----- Epilogue -----

	// Found return check; TODO: Functions declared with [noreturn] should not be subject
	// to this check.
	if (func->body->stmt == CK_STMT_BLOCK && !found_return && func->signature->child->id != CK_FOOD_VOID) {
		CkDiagnosticThrow(cfg->p_dhi, &func->body->prim, CK_DIAG_SEVERITY_ERROR, "",
			"The function returns no value in any control path, but it is supposed to return a value.");
		return bindings;
	}

	return bindings;
}
