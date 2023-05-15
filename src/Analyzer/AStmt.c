#include "Intern.h"

#include <CDebug.h>

size_t AnalyzeStatement(CkStatement *stmt, CkAnalyzeConfig *cfg)
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
	bindings += AnalyzeExpression(stmt->data.expression, cfg);
	goto LEnd;
LBlock:
	FOREACH(stmt->data.block.stmts, node_stmt)
		bindings += AnalyzeStatement(ITEM(CkStatement *, node_stmt), cfg);
	goto LEnd;
LIf:
	bindings += AnalyzeExpression(stmt->data.if_.condition, cfg);
	bindings += AnalyzeStatement(stmt->data.if_.cThen, cfg);
	if (stmt->data.if_.cElse) bindings += AnalyzeStatement(stmt->data.if_.cElse, cfg);
	goto LEnd;
LWhile:
	bindings += AnalyzeExpression(stmt->data.while_.condition, cfg);
	bindings += AnalyzeStatement(stmt->data.while_.cWhile, cfg);
	goto LEnd;
LDoWhile:
	bindings += AnalyzeExpression(stmt->data.doWhile.condition, cfg);
	bindings += AnalyzeStatement(stmt->data.doWhile.cWhile, cfg);
	goto LEnd;
LFor:
	bindings += AnalyzeStatement(stmt->data.for_.cInit, cfg);
	bindings += AnalyzeExpression(stmt->data.for_.condition, cfg);
	bindings += AnalyzeExpression(stmt->data.for_.lead, cfg);
	bindings += AnalyzeStatement(stmt->data.for_.body, cfg);
	goto LEnd;
LSwitch:
	// TODO
	goto LEnd;
LBreak: return 0;
LContinue: return 0;
LGoto:
	if (stmt->data.goto_.computed)
		bindings += AnalyzeExpression(stmt->data.goto_.computedExpression, cfg);
	goto LEnd;
LAssert:
	bindings += AnalyzeExpression(stmt->data.assert.expression, cfg);
	goto LEnd;
LSponge:
	bindings += AnalyzeStatement(stmt->data.sponge, cfg);
	goto LEnd;
LReturn:
	if (stmt->data.return_)
		bindings += AnalyzeExpression(stmt->data.return_, cfg);

	// fallthrough
LEnd:

	return bindings;
}
