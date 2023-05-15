#include "Intern.h"

#include <CDebug.h>

size_t AnalyzeStatement(CkStatement *stmt, CkAnalyzeConfig *cfg, CkScope *scope, bool *mut_return_found)
{
	static const void *stmt_dispatch_table[] = {
		&&LEmpty, &&LExpression, &&LBlock, &&LIf, &&LWhile,
		&&LDoWhile, &&LFor, &&LSwitch, &&LBreak, &&LContinue,
		&&LGoto, &&LAssert, &&LSponge, &&LReturn,
	};
	size_t bindings = 0;

	CK_ASSERT(stmt->stmt <= CK_STMT_RETURN);
	goto *stmt_dispatch_table[stmt->stmt];

LEmpty: return 0;
LExpression:
	bindings += AnalyzeExpression(stmt->data.expression, cfg, scope);
	goto LEnd;
LBlock:
	FOREACH(stmt->data.block.stmts, node_stmt)
		bindings += AnalyzeStatement(ITEM(CkStatement *, node_stmt), cfg, stmt->data.block.scope, mut_return_found);
	goto LEnd;
LIf:
	bindings += AnalyzeExpression(stmt->data.if_.condition, cfg, scope);
	bindings += AnalyzeStatement(stmt->data.if_.cThen, cfg, scope, mut_return_found);
	if (stmt->data.if_.cElse) bindings += AnalyzeStatement(stmt->data.if_.cElse, cfg, scope, mut_return_found);
	goto LEnd;
LWhile:
	bindings += AnalyzeExpression(stmt->data.while_.condition, cfg, scope);
	bindings += AnalyzeStatement(stmt->data.while_.cWhile, cfg, scope, mut_return_found);
	goto LEnd;
LDoWhile:
	bindings += AnalyzeExpression(stmt->data.doWhile.condition, cfg, scope);
	bindings += AnalyzeStatement(stmt->data.doWhile.cWhile, cfg, scope, mut_return_found);
	goto LEnd;
LFor:
	bindings += AnalyzeStatement(stmt->data.for_.cInit, cfg, stmt->data.for_.scope, mut_return_found);
	bindings += AnalyzeExpression(stmt->data.for_.condition, cfg, stmt->data.for_.scope);
	bindings += AnalyzeExpression(stmt->data.for_.lead, cfg, stmt->data.for_.scope);
	bindings += AnalyzeStatement(stmt->data.for_.body, cfg, stmt->data.for_.scope, mut_return_found);
	goto LEnd;
LSwitch:
	// TODO
	goto LEnd;
LBreak: return 0;
LContinue: return 0;
LGoto:
	if (stmt->data.goto_.computed)
		bindings += AnalyzeExpression(stmt->data.goto_.computedExpression, cfg, scope);
	goto LEnd;
LAssert:
	bindings += AnalyzeExpression(stmt->data.assert.expression, cfg, scope);
	goto LEnd;
LSponge:
	bindings += AnalyzeStatement(stmt->data.sponge, cfg, scope, mut_return_found);
	goto LEnd;
LReturn:
	if (stmt->data.return_)
		bindings += AnalyzeExpression(stmt->data.return_, cfg, scope);
	*mut_return_found = true;

	// fallthrough
LEnd:

	return bindings;
}
