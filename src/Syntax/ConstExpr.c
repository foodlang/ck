#include <Syntax/ConstExpr.h>
#include <CDebug.h>

CkExpression *CkConstExprReduce( CkArenaFrame *arena, CkExpression *src )
{
	CkExpression *left, *right, *extra; // Expr branches
	CkExpression *result;

	CK_ARG_NON_NULL( src );
	CK_ARG_NON_NULL( arena );

	// --- Making sure all of the child nodes have been reduced
	left = src->left ? CkConstExprReduce( arena, src->left ) : NULL;
	right = src->right ? CkConstExprReduce( arena, src->right ) : NULL;
	extra = src->extra ? CkConstExprReduce( arena, src->extra ) : NULL;

	switch ( src->kind ) {
	case CK_EXPRESSION_INTEGER_LITERAL: return src; // no copy
	case CK_EXPRESSION_FLOAT_LITERAL: return src;
	case CK_EXPRESSION_STRING_LITERAL: return src;
	case CK_EXPRESSION_BOOL_LITERAL: return src;
	case CK_EXPRESSION_ADD:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			CkExpressionKind k;
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 + right->token.value.i64;
				k = CK_EXPRESSION_INTEGER_LITERAL;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.f64 = left->token.value.f64 + right->token.value.f64;
				k = CK_EXPRESSION_FLOAT_LITERAL;
			}
			result = CkExpressionCreateLiteral( arena, &v, k, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_SUB:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			CkExpressionKind k;
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 - right->token.value.i64;
				k = CK_EXPRESSION_INTEGER_LITERAL;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.f64 = left->token.value.f64 - right->token.value.f64;
				k = CK_EXPRESSION_FLOAT_LITERAL;
			}
			result = CkExpressionCreateLiteral( arena, &v, k, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_MUL:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			CkExpressionKind k;
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 * right->token.value.i64;
				k = CK_EXPRESSION_INTEGER_LITERAL;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.f64 = left->token.value.f64 * right->token.value.f64;
				k = CK_EXPRESSION_FLOAT_LITERAL;
			}
			result = CkExpressionCreateLiteral( arena, &v, k, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_DIV:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			CkExpressionKind k;
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 / right->token.value.i64;
				k = CK_EXPRESSION_INTEGER_LITERAL;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.f64 = left->token.value.f64 / right->token.value.f64;
				k = CK_EXPRESSION_FLOAT_LITERAL;
			}
			result = CkExpressionCreateLiteral( arena, &v, k, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_MOD:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.i64 % right->token.value.i64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_UNARY_MINUS:
		if ( left->isConstant ) {
			CkToken v = {};
			CkExpressionKind k;
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = -left->token.value.i64;
				k = CK_EXPRESSION_INTEGER_LITERAL;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.f64 = -left->token.value.f64;
				k = CK_EXPRESSION_FLOAT_LITERAL;
			}
			result = CkExpressionCreateLiteral( arena, &v, k, src->type );
			result->isConstant = true;
			return result;
		}
	case CK_EXPRESSION_UNARY_PLUS: if ( left->isConstant ) return src->left;
	case CK_EXPRESSION_BITWISE_AND:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.u64 & right->token.value.u64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_BITWISE_OR:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.u64 | right->token.value.u64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_BITWISE_XOR:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.u64 ^ right->token.value.u64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_BITWISE_NOT: 
		if ( left->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = ~left->token.value.i64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
	case CK_EXPRESSION_LOGICAL_AND:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.boolean && right->token.value.boolean;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_LOGICAL_OR:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.boolean && right->token.value.boolean;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_LOGICAL_NOT:
		if ( left->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = !left->token.value.i64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_EQUAL:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.boolean == right->token.value.boolean;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_NOT_EQUAL:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.boolean != right->token.value.boolean;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_LEFT_SHIFT:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.i64 << right->token.value.i64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_RIGHT_SHIFT:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			v.kind = '0';
			v.position = 0;
			v.value.u64 = left->token.value.i64 >> right->token.value.i64;
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_INTEGER_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_LOWER:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 < right->token.value.i64;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.u64 = left->token.value.f64 < right->token.value.f64;
			}
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_LOWER_EQUAL:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 <= right->token.value.i64;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.u64 = left->token.value.f64 <= right->token.value.f64;
			}
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_GREATER:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 > right->token.value.i64;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.u64 = left->token.value.f64 > right->token.value.f64;
			}
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	case CK_EXPRESSION_GREATER_EQUAL:
		if ( left->isConstant && right->isConstant ) {
			CkToken v = {};
			if ( CK_TYPE_CLASSED_INT( left->type->id ) ) {
				v.kind = '0';
				v.position = 0;
				v.value.u64 = left->token.value.i64 >= right->token.value.i64;
			} else {
				v.kind = 'F';
				v.position = 0;
				v.value.u64 = left->token.value.f64 >= right->token.value.f64;
			}
			result = CkExpressionCreateLiteral( arena, &v, CK_EXPRESSION_BOOL_LITERAL, src->type );
			result->isConstant = true;
			return result;
		}
		return src;
	default: return src; // constant flag is going to be false
	}
}
