#include <Defines.h>
#include <Syntax/Preprocessor.h>

void CkDefineConstants( CkArenaFrame *allocator, CkList *dest )
{
#define DEF(Mx) CkDeclareCompileTimeMacro(allocator, dest, Mx, &constant)

	CkToken constant; // Buffer used to hold constants
	constant.position = 0;  // No location
	constant.source = NULL; // No location
	
	// 1. __CC__
	constant.kind = 'S'; constant.value.cstr = "ck"; DEF( "__CC" );
	
	// 2. Integer ranges
	constant.kind = '0';
	constant.value.i8 = 0x7F; DEF( "__I8_MAX" );
	constant.value.i8 = -0x7F; DEF( "__I8_MIN" );
	constant.value.u8 = 0xFF; DEF( "__U8_MAX" );
	constant.value.u8 = 0; DEF( "__U8_MIN" );
	constant.value.i16 = 0x7FFF; DEF( "__I16_MAX" );
	constant.value.i16 = -0x7FFF; DEF( "__I16_MIN" );
	constant.value.u16 = 0xFFFF; DEF( "__U16_MAX" );
	constant.value.u16 = 0; DEF( "__U16_MIN" );
	constant.value.i32 = 0x7FFFFFFF; DEF( "__I32_MAX" );
	constant.value.i32 = -0x7FFFFFFF; DEF( "__I32_MIN" );
	constant.value.u32 = 0xFFFFFFFF; DEF( "__U32_MAX" );
	constant.value.u32 = 0; DEF( "__U32_MIN" );
	constant.value.i64 = 0x7FFFFFFF; DEF( "__I64_MAX" );
	constant.value.i64 = -0x7FFFFFFF; DEF( "__I64_MIN" );
	constant.value.u64 = 0xFFFFFFFF; DEF( "__U64_MAX" );
	constant.value.u64 = 0; DEF( "__U64_MIN" );

	// 3. __FOOD_VER__
	constant.kind = '0'; constant.value.u64 = 11; DEF( "__FOOD_VER" );

#undef DEF
}
