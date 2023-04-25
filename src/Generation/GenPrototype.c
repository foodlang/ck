#include <Generation/GenPrototype.h>
#include <Util/Itoa.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define NOREG 0xFF
#define RB 0
#define RC 1
#define RD 2
#define R8 3
#define R9 4
#define R10 5
#define R11 6
#define R12 7
#define R13 8
#define R14 9
#define R15 10
#define ACC   11

// The counter for local labels.
static size_t _local_label_counter = 0;

// The counter for unnamed static data labels.
static size_t _static_unnamed_label_counter = 0;

// Stores the data to be stored in the data section (static data)
static CkList *_data_section = NULL;

// A x86-64 register.
typedef struct _Register
{
	bool_t free;
	char*  name8;
	char*  name16;
	char*  name32;
	char*  name64;

} _Register;

// A name for an entry of static data.
typedef union _StaticDataName
{
	char *cstr; // C-string name (named)
	size_t id;  // ID (unnamed)
} _StaticDataName;

// Static data.
typedef struct _StaticData
{
	void  *data;          // Constant data
	bool_t public;        // Whether the data is public or not
	bool_t named;         // Whether the data has a given name
	_StaticDataName name; // The name
	FoodTypeID typeid;    // The type of the data

} _StaticData;

// The table of integer registers.
static _Register _regint_table[] =
{
	/* 0 */ {TRUE, "bl", "bx", "ebx", "rbx"},
	/* 1 */ { TRUE, "cl", "cx", "ecx", "rcx" },
	/* 2 */ { TRUE, "dl", "dx", "edx", "rdx" },
	/* 3 */ { TRUE, "r8b", "r8w", "r8d", "r8" },
	/* 4 */ { TRUE, "r9b", "r9w", "r9d", "r9" },
	/* 5 */ { TRUE, "r10b", "r10w", "r10d", "r10" },
	/* 6 */ { TRUE, "r11b", "r11w", "r11d", "r11" },
	/* 7 */ { TRUE, "r12b", "r12w", "r12d", "r12" },
	/* 8 */ { TRUE, "r13b", "r13w", "r13d", "r13" },
	/* 9 */ { TRUE, "r14b", "r14w", "r14d", "r14" },
	/* 10 */ { TRUE, "r15b", "r15w", "r15d", "r15" },
	/* 11 */ { FALSE, "al", "ax", "eax", "rax" } // used for returns
};

// Allocates a new integer register.
static size_t _AllocateIntRegister( void )
{
	for ( size_t i = 0; i < sizeof( _regint_table ) / sizeof( _Register ) - 1; i++ ) {
		if ( _regint_table[i].free ) {
			_regint_table[i].free = FALSE;
			return i;
		}
	}

	printf( "ck: internal error, out of registers, TODO: more registers\n" );
	abort();
}

// Frees an integer register.
static void _FreeIntRegister( size_t reg )
{
	if ( reg == NOREG ) return;
	_regint_table[reg].free = TRUE;
}

// Integer register name.
static char* _RegnameInt( size_t reg, FoodTypeID typeid )
{
	switch ( typeid )
	{
	case CK_FOOD_I8:
	case CK_FOOD_U8:
	case CK_FOOD_BOOL:
		return _regint_table[reg].name8;

	case CK_FOOD_I16:
	case CK_FOOD_U16:
		return _regint_table[reg].name16;

	case CK_FOOD_I32:
	case CK_FOOD_U32:
		return _regint_table[reg].name32;

	case CK_FOOD_I64:
	case CK_FOOD_U64:
	case CK_FOOD_POINTER:
	case CK_FOOD_REFERENCE:
	case CK_FOOD_ARRAY:
	case CK_FOOD_FUNCPOINTER:
		return _regint_table[reg].name64;

	default:
		return "";
	}
}

// Inserts static data.
static void _InsertStaticData(
	CkArenaFrame *allocator,
	bool_t public,
	void *cdata,
	FoodTypeID typeid,
	bool_t named,
	char *optional_name )
{
	_StaticData entry = {};
	// --- Create list of not already existent ---
	if ( !_data_section )
		_data_section = CkListStart( allocator, sizeof( _StaticData ) );

	entry.named = named;
	entry.public = public;
	entry.typeid = typeid;
	if ( optional_name ) entry.name.cstr = optional_name;
	else entry.name.id = _static_unnamed_label_counter++;

	// Replace some characters with escape sequences
	if ( typeid == CK_FOOD_STRING ) {
		char c;
		char *cstr;
		size_t cstrlen;
		CkStrBuilder out;
		char *outbuf;
		size_t outbuflen;

		cstr = cdata;
		cstrlen = strlen( cstr );
		CkStrBuilderCreate( &out, 10 );
		
		for ( size_t i = 0; i < cstrlen; i++ ) {
			c = cstr[i];
			// These characters will be inserted as escape sequences
			if ( c <= 0x1F || c >= 0x7F ) {
				outbuflen = snprintf( NULL, 0, "\\x%02hhX", (uint8_t)c );
				outbuf = malloc( outbuflen + 1 );
				sprintf( outbuf, "\\x%02hhX", (uint8_t)c );
				CkStrBuilderAppendString( &out, outbuf );
				free( outbuf );

			} else CkStrBuilderAppendChar( &out, c );
		}
		entry.data = CkArenaAllocate( allocator, out.length + 1 );
		strcpy( entry.data, out.base );
		CkStrBuilderDispose( &out );

	} else entry.data = cdata;

	CkListAdd( _data_section, &entry );
}

// Inserts a line of code.
static void _InsertLine( CkStrBuilder *sb, const char* format, ... )
{
	va_list v_argv;      // Variadic argument vector
	va_list v_argv2;     // Copy of v_argv
	char*   vbuffer;     // Variadic buffer (outputted by vsprintf)
	size_t  vbufferSize; // The length of the vbuffer

	// --- Writing to vbuffer ---
	va_start( v_argv, format );
	va_copy( v_argv2, v_argv );
	vbufferSize = vsnprintf( NULL, 0, format, v_argv2 );
	va_end( v_argv2 );
	vbuffer = malloc( vbufferSize + 1 );
	(void)vsprintf( vbuffer, format, v_argv );
	va_end( v_argv );

	// --- Outputting ---
	CkStrBuilderAppendChar( sb, '\t');
	CkStrBuilderAppendString( sb, vbuffer );
	CkStrBuilderAppendChar( sb, '\n' );
	free( vbuffer );
}

// The size of a type (value types only)
static size_t _SizeOfV( FoodTypeID t )
{
	if ( t == CK_FOOD_I8 || t == CK_FOOD_U8 ) return 1;
	if ( t == CK_FOOD_I16 || t == CK_FOOD_U16 || t == CK_FOOD_F16 ) return 2;
	if ( t == CK_FOOD_I32 || t == CK_FOOD_U32 || t == CK_FOOD_F32 ) return 4;
	if ( t == CK_FOOD_I64 || t == CK_FOOD_U64 || t == CK_FOOD_F64 ) return 8;
	return 0;
}

static bool_t _Unsigned( FoodTypeID t )
{
	return t == CK_FOOD_U8
		|| t == CK_FOOD_U16
		|| t == CK_FOOD_U32
		|| t == CK_FOOD_U64
		|| t == CK_FOOD_POINTER
		|| t == CK_FOOD_REFERENCE
		|| t == CK_FOOD_STRING
		|| t == CK_FOOD_FUNCPOINTER;
}

// Extends a register accordingly.
static void _ExtendRegister( CkStrBuilder *sb, size_t r, FoodTypeID original, FoodTypeID result )
{
	char* resultname;
	char* sourcename;

	if ( _SizeOfV( result ) >= _SizeOfV( original ) ) return;

	sourcename = _RegnameInt( r, original );
	resultname = _RegnameInt( r, result );

	if ( !_Unsigned( result ) ) _InsertLine( sb, "movsx %s, %s", resultname, sourcename );
	else _InsertLine( sb, "movzx %s, %s", resultname, sourcename );
}

// Inserts an expression. Returns the output register
static size_t _InsertExpression( CkArenaFrame *allocator, CkStrBuilder* sb, CkExpression* expr )
{
	size_t left = NOREG;  // Left register
	size_t right = NOREG; // Right register
	size_t extra = NOREG; // Extra register
	size_t out = NOREG;   // Output register

	// --- Generating operands ---
	if ( expr->left ) {
		left = _InsertExpression( allocator, sb, expr->left );
		_ExtendRegister( sb, left, expr->left->type->id, expr->type->id );
	}
	if ( expr->right ) {
		right = _InsertExpression( allocator, sb, expr->right );
		_ExtendRegister( sb, right, expr->right->type->id, expr->type->id );
	}
	if ( expr->extra ) {
		extra = _InsertExpression( allocator, sb, expr->extra );
		_ExtendRegister( sb, extra, expr->extra->type->id, expr->type->id );
	}

	// --- Generating expression ---
	switch ( expr->kind ) {

	case CK_EXPRESSION_INTEGER_LITERAL: {
		char* regname;
		size_t value;
		out = _AllocateIntRegister();
		switch ( expr->type->id ) {
			// 8-bit
		case CK_FOOD_I8:
		case CK_FOOD_U8:
			regname = _regint_table[out].name8;
			value = expr->token.value.u8;
			break;
			// 16-bit
		case CK_FOOD_I16:
		case CK_FOOD_U16:
			regname = _regint_table[out].name16;
			value = expr->token.value.u16;
			break;
			// 32-bit
		case CK_FOOD_I32:
		case CK_FOOD_U32:
			regname = _regint_table[out].name32;
			value = expr->token.value.u32;
			break;
			// 64-bit
		case CK_FOOD_I64:
		case CK_FOOD_U64:
		default:
			regname = _regint_table[out].name64;
			value = expr->token.value.u64;
			break;
		}
		if ( value == 0 ) _InsertLine( sb, "xor %s, %s", regname, regname );
		else _InsertLine( sb, "mov %s, %zu", regname, value );
		break;
	}
	case CK_EXPRESSION_BOOL_LITERAL:
		out = _AllocateIntRegister();
		if ( expr->token.value.boolean == FALSE )
			_InsertLine( sb, "xor %s, %s", _regint_table[out].name8, _regint_table[out].name8 );
		else
			_InsertLine( sb, "mov %s, 1", _regint_table[out].name8 );
		break;

	case CK_EXPRESSION_STRING_LITERAL: {
		size_t sref;

		out = _AllocateIntRegister();
		sref = _static_unnamed_label_counter;
		_InsertStaticData( allocator, FALSE, expr->token.value.ptr, CK_FOOD_STRING, FALSE, NULL );
		_InsertLine( sb, "lea %s, .S%zu", _regint_table[out].name64, sref );
		break;
	}
	case CK_EXPRESSION_UNARY_PLUS:
		out = left;
		break;

	case CK_EXPRESSION_UNARY_MINUS: {
		char* regname;
		out = left;
		regname = _RegnameInt( left, expr->type->id );
		_InsertLine( sb, "neg %s", regname );
		break;
	}
	case CK_EXPRESSION_BITWISE_NOT: {
		char* regname;
		out = left;
		regname = _RegnameInt( left, expr->type->id );
		_InsertLine( sb, "not %s", regname );
		break;
	}
	case CK_EXPRESSION_LOGICAL_NOT: {
		char* regname;
		char* maskname;
		out = left;
		regname = _RegnameInt( left, expr->type->id );
		maskname = _RegnameInt( right, expr->type->id );
		_InsertLine( sb, "mov %s, -1", maskname );
		_InsertLine( sb, "xor %s, %s", regname, maskname );
		break;
	}
	case CK_EXPRESSION_SUBSCRIPT: {
		char* regname;
		char* indexname;
		size_t scale; // 2^scale
		out = left;
		switch ( expr->type->child->id ) {
		case CK_FOOD_I8:
		case CK_FOOD_U8:
		case CK_FOOD_BOOL:
			regname = _regint_table[out].name8;
			indexname = _regint_table[right].name8;
			scale = 0;
			break;

		case CK_FOOD_I16:
		case CK_FOOD_U16:
			regname = _regint_table[out].name16;
			indexname = _regint_table[right].name16;
			scale = 1;
			break;

		case CK_FOOD_I32:
		case CK_FOOD_U32:
			regname = _regint_table[out].name32;
			indexname = _regint_table[right].name32;
			scale = 2;
			break;

		case CK_FOOD_I64:
		case CK_FOOD_U64:
		case CK_FOOD_POINTER:
		case CK_FOOD_FUNCPOINTER:
		case CK_FOOD_ARRAY:
		default:
			regname = _regint_table[out].name64;
			indexname = _regint_table[right].name64;
			scale = 3;
			break;
		}
		if ( scale != 0 ) _InsertLine( sb, "shl %s, %zu", indexname, scale );
		_InsertLine( sb, "add %s, %s", regname, indexname );
		_InsertLine( sb, "mov %s, [%s]", regname, regname );
		break;
	}
	case CK_EXPRESSION_ADD: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "add %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_SUB: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "sub %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_MUL: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		if ( _Unsigned( expr->type->id ) )
			_InsertLine( sb, "mul %s, %s", leftname, rightname );
		else _InsertLine( sb, "imul %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_DIV: {
		char* leftname;
		char* rightname;
		char* accname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );
		accname = _RegnameInt( ACC, expr->type->id );

		if ( _regint_table[RD].free ) _InsertLine( sb, "push rdx" );
		_InsertLine( sb, "mov %s, %s", accname, leftname );
		_InsertLine( sb, "div %s", rightname );
		_InsertLine( sb, "mov %s, %s", leftname, accname );
		if ( _regint_table[RD].free ) _InsertLine( sb, "pop rdx" );
		break;
	}
	case CK_EXPRESSION_MOD: {
		char* leftname;
		char* rightname;
		char* accname;
		char* remaindername;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );
		accname = _RegnameInt( ACC, expr->type->id );
		remaindername = _RegnameInt( RD, expr->type->id );

		if ( _regint_table[RD].free ) _InsertLine( sb, "push rdx" );
		_InsertLine( sb, "mov %s, %s", accname, leftname );
		_InsertLine( sb, "div %s", rightname );
		_InsertLine( sb, "mov %s, %s", leftname, remaindername );
		if ( _regint_table[RD].free ) _InsertLine( sb, "pop rdx" );
		break;
	}
	case CK_EXPRESSION_BITWISE_OR: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "or %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_BITWISE_AND: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "and %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_BITWISE_XOR: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "xor %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_LOGICAL_AND: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "test %s, %s", leftname, leftname );
		_InsertLine( sb, "setne %s", leftname );
		_InsertLine( sb, "test %s, %s", rightname, rightname );
		_InsertLine( sb, "setne %s", rightname );
		_InsertLine( sb, "and %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_LOGICAL_OR: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "or %s, %s", leftname, rightname );
		_InsertLine( sb, "setne %s", leftname );
		break;
	}
	case CK_EXPRESSION_LOWER: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "cmp %s, %s", leftname, rightname );
		_InsertLine( sb, "setl %s", leftname );
		break;
	}
	case CK_EXPRESSION_LOWER_EQUAL: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "cmp %s, %s", leftname, rightname );
		_InsertLine( sb, "setle %s", leftname );
		break;
	}
	case CK_EXPRESSION_GREATER: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "cmp %s, %s", leftname, rightname );
		_InsertLine( sb, "setg %s", leftname );
		break;
	}
	case CK_EXPRESSION_GREATER_EQUAL: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "cmp %s, %s", leftname, rightname );
		_InsertLine( sb, "setge %s", leftname );
		break;
	}
	case CK_EXPRESSION_EQUAL: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "cmp %s, %s", leftname, rightname );
		_InsertLine( sb, "sete %s", leftname );
		break;
	}

	case CK_EXPRESSION_NOT_EQUAL: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "cmp %s, %s", leftname, rightname );
		_InsertLine( sb, "setne %s", leftname );
		break;
	}
	case CK_EXPRESSION_LEFT_SHIFT: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "sal %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_RIGHT_SHIFT: {
		char* leftname;
		char* rightname;
		out = left;

		leftname = _RegnameInt( left, expr->type->id );
		rightname = _RegnameInt( right, expr->type->id );

		_InsertLine( sb, "shr %s, %s", leftname, rightname );
		break;
	}
	case CK_EXPRESSION_DEREFERENCE: {
		char* regname;
		char* ptrname;

		ptrname = _RegnameInt( left, expr->left->type->id );
		regname = _RegnameInt( left, expr->type->id );

		_InsertLine( sb, "mov %s, [%s]", regname, ptrname );
		break;
	}
	case CK_EXPRESSION_COMPOUND_LITERAL: _FreeIntRegister( right ); break;
	default:
		_InsertLine( sb, "; unsupported expression %d", expr->kind );
	}

	// --- Freeing registers and returning ---
	_FreeIntRegister( right );
	_FreeIntRegister( extra );
	return out;
}

// Generates a statement.
static void _GenerateStatement( CkArenaFrame *allocator, CkStrBuilder* sb, CkStatement* stmt )
{
	switch ( stmt->stmt ) {
	case CK_STMT_EMPTY: break;
	case CK_STMT_EXPRESSION: _FreeIntRegister( _InsertExpression( allocator, sb, stmt->data.expression ) ); break;
	case CK_STMT_BLOCK: {
		// TODO: variables
		size_t stmtCount = CkListLength( stmt->data.block.stmts );
		for ( size_t i = 0; i < stmtCount; i++ ) {
			CkStatement* cstmt = *(CkStatement**)CkListAccess( stmt->data.block.stmts, i );
			_GenerateStatement( allocator, sb, cstmt );
		}
		break;
	}
	case CK_STMT_IF: {
		size_t lElse;
		size_t lLead;
		size_t exprReg;
		char *regname;

		lLead = _local_label_counter++;
		lElse = stmt->data.if_.cElse != NULL ? _local_label_counter++ : 0;

		exprReg = _InsertExpression( allocator, sb, stmt->data.if_.condition );
		regname = _RegnameInt( exprReg, stmt->data.if_.condition->type->id );
		_InsertLine( sb, "test %s, %s", regname, regname );
		_InsertLine( sb, "jz .L%zu", lElse != 0 ? lElse : lLead );
		_FreeIntRegister( exprReg );
		_GenerateStatement( allocator, sb, stmt->data.if_.cThen );
		if ( stmt->data.if_.cElse != NULL ) {
			_InsertLine( sb, "goto .L%zu", lLead );
			_InsertLine( sb, ".L%zu:", lElse );
			_GenerateStatement( allocator, sb, stmt->data.if_.cElse );
		}
		_InsertLine( sb, ".L%zu:", lLead );
		break;
	}
	case CK_STMT_WHILE: {
		size_t lLead;
		size_t lLoop;
		size_t exprReg;
		char *regname;

		lLoop = _local_label_counter++;
		lLead = _local_label_counter++;

		_InsertLine( sb, ".L%zu:", lLoop );
		exprReg = _InsertExpression( allocator, sb, stmt->data.while_.condition );
		regname = _RegnameInt( exprReg, stmt->data.while_.condition->type->id );
		_InsertLine( sb, "test %s, %s", regname, regname );
		_InsertLine( sb, "jz .L%zu", lLead );
		_FreeIntRegister( exprReg );
		_GenerateStatement( allocator, sb, stmt->data.while_.cWhile );
		_InsertLine( sb, "goto .L%zu", lLoop );
		_InsertLine( sb, ".L%zu:", lLead );
		break;
	}
	}
}

// Inserts a label.
static void _InsertLabel( CkStrBuilder* sb, char* label )
{
	CkStrBuilderAppendString( sb, label );
	CkStrBuilderAppendString( sb, ":\n" );
}

// Inserts a local label.
static uint32_t _InsertLocalLabel( CkStrBuilder* sb )
{
	char itoabuf[17]; // 17 because no hex number has digits beyond 16 (64-bit)
	CkUtoaHex( _local_label_counter, itoabuf ); // itoa(), hex and unsigned

	// Writing
	CkStrBuilderAppendString( sb, ".L" );
	CkStrBuilderAppendString( sb, itoabuf );
	CkStrBuilderAppendString( sb, ":\n" );

	// Returning current label
	return _local_label_counter++;
}

// Inserts a function's name.
static void _InsertFuncName( CkStrBuilder* sb, CkFunction* pFunc )
{
	char* buffer;        // The output buffer
	size_t bufferLength; // The output buffer's length

	// With module name
	if ( pFunc->parent->module != NULL ) {
		bufferLength = snprintf(
			NULL,
			0,
			"%s_%s_%s",
			pFunc->parent->library->name,
			pFunc->parent->module->name,
			pFunc->name );
		buffer = malloc( bufferLength + 1 );
		(void)sprintf(
			buffer,
			"%s_%s_%s",
			pFunc->parent->library->name,
			pFunc->parent->module->name,
			pFunc->name );
	} else { // Without module
		bufferLength = sprintf(
			NULL,
			"%s_%s",
			pFunc->parent->library->name,
			pFunc->name );
		buffer = malloc( bufferLength + 1 );
		(void)sprintf(
			buffer,
			"%s_%s",
			pFunc->parent->library->name,
			pFunc->name );
	}

	// Outputting
	CkStrBuilderAppendString( sb, buffer );
	free( buffer );
}

// Inserts a function.
static void _InsertFunction( CkArenaFrame *allocator, CkStrBuilder* sb, CkFunction* pFunc )
{
	// --- Insert function header ---

	if ( pFunc->bPublic ) {
		CkStrBuilderAppendString( sb, "global " );
		_InsertFuncName( sb, pFunc );
		CkStrBuilderAppendChar( sb, '\n' );
	}

	_InsertFuncName( sb, pFunc );
	CkStrBuilderAppendString( sb, ":\n" );

	// --- Insert function body ---
	_GenerateStatement( allocator, sb, pFunc->body );
}

// Inserts a new module.
static void _InsertModule( CkArenaFrame *allocator, CkStrBuilder* sb, CkModule* module )
{
	size_t funcCount;
	_InsertLine( sb, "; Module %s::%s", module->scope->library->name, module->name );

	funcCount = CkListLength( module->scope->functionList );
	for ( size_t i = 0; i < funcCount; i++ )
		_InsertFunction( allocator, sb, CkListAccess( module->scope->functionList, i ) );
}

char* CkGenProgram_Prototype(
	CkDiagnosticHandlerInstance* pDhi, // The diagnostics handler
	CkArenaFrame* allocator,           // Allocator for statically sized objects
	CkList* libraries                  // Element type = CkLibrary*
	)
{
	CkStrBuilder outsb;   // The output code (string buffer)
	char* out;            // The output code
	size_t libCount;      // The number of libraries to generate

	// --- String Builder initialization ---
	CkStrBuilderCreate( &outsb, 4096 );

	// --- CK credit ---
	CkStrBuilderAppendString( &outsb, "; Generated with CK\n" );
	CkStrBuilderAppendString( &outsb, "section .code\n" );

	// --- Generating libraries ---
	libCount = CkListLength( libraries );
	for ( size_t i = 0; i < libCount; i++ ) {
		size_t glblFuncCount;
		size_t moduleCount;
		CkLibrary* lib = *(CkLibrary**)CkListAccess( libraries, i );

		_InsertLine( &outsb, "; Library %s", lib->name );

		// Global functions
		glblFuncCount = CkListLength( lib->scope->functionList );
		for ( size_t j = 0; j < glblFuncCount; j++ )
			_InsertFunction( allocator, &outsb, CkListAccess( lib->scope->functionList, j ) );

		// Modules
		moduleCount = CkListLength( lib->moduleList );
		for ( size_t j = 0; j < moduleCount; j++ )
			_InsertModule( allocator, &outsb, *(CkModule**)CkListAccess(lib->moduleList, j) );
	}

	// --- Generating static data ---
	if ( _data_section ) {
		size_t elems = CkListLength( _data_section );
		CkStrBuilderAppendString( &outsb, "section .data\n" );
		for ( size_t i = 0; i < elems; i++ ) {
			_StaticData *sd = CkListAccess( _data_section, i );
			if ( sd->named ) {
				if ( sd->public ) {
					CkStrBuilderAppendString( &outsb, "global " );
					CkStrBuilderAppendString( &outsb, sd->name.cstr );
					CkStrBuilderAppendChar( &outsb, '\n' );
				}
				CkStrBuilderAppendString( &outsb, sd->name.cstr );
				CkStrBuilderAppendString( &outsb, ":\n" );
			} else {
				_InsertLine( &outsb, "\r.S%zu:", sd->name.id );
			}
			switch ( sd->typeid ) {
			case CK_FOOD_I8:
			case CK_FOOD_U8:
			case CK_FOOD_BOOL:
				_InsertLine( &outsb, "db %hhu", *(uint8_t *)sd->data );
				break;
			case CK_FOOD_I16:
			case CK_FOOD_U16:
				_InsertLine( &outsb, "dw %hu", *(uint16_t *)sd->data );
				break;
			case CK_FOOD_I32:
			case CK_FOOD_U32:
			case CK_FOOD_ENUM:
				_InsertLine( &outsb, "dd %u", *(uint32_t *)sd->data );
				break;
			case CK_FOOD_I64:
			case CK_FOOD_U64:
			case CK_FOOD_POINTER:
				_InsertLine( &outsb, "dq %llu", *(uint64_t *)sd->data );
				break;
			case CK_FOOD_STRING:
				_InsertLine( &outsb, "db \"%s\"", (char *)sd->data );
				break;
			default:
				_InsertLine( &outsb, "; unsupported literal %u", sd->typeid );
				break;
			}
		}
	}
	
	// --- Copying output to arena allocated memory ---
	out = CkArenaAllocate( allocator, outsb.length + 1 );
	strcpy( out, outsb.base );
	CkStrBuilderDispose( &outsb );

	return out;
}