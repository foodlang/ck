/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header defines structures and functions for the Fast Food AST.
 * Fast Food is the name given to the AST format. A fast food library
 * can contain many modules.
 *
***************************************************************************/

#ifndef CK_FFSTRUCT_H_
#define CK_FFSTRUCT_H_

#include <Memory/List.h>

#define CK_DECLATTR_INLINE_REGISTER_BIT    0b00000001 // [inline] (function-only)/[register] (variable-only)
#define CK_DECLATTR_CLANG_BIT              0b00000010 // [clang] (C symbols)
#define CK_DECLATTR_DYNAMIC_BIT            0b00000100 // [dynamic] (dllimport (requires extern)/dllexport)
#define CK_DECLATTR_DEPRECATED_BIT         0b00001000 // [deprecated] (generates warnings upon symbol usage)
#define CK_DECLATTR_NORETURN_NULLCHECK_BIT 0b00010000 // [noreturn] (function-only)/[nullcheck] (variable-only)
#define CK_DECLATTR_OVERRIDE_PACKED_BIT    0b00100000 // [override] (function-only)/[packed] (type-only)
#define CK_DECLATTR_MAYBE_UNUSED_BIT       0b01000000 // [maybe_unused] (symbol can be unused)
// alignas() and callconv() not supported in ckc (CK in C)

typedef uint_least8_t CkDeclAttr; // Declaration attribute storage

// The kind of a statement.
typedef enum CkStatementKind
{
	CK_STMT_EMPTY,      // An empty statement.
	CK_STMT_EXPRESSION, // An expression statement.
	CK_STMT_BLOCK,      // A block statement.
	CK_STMT_IF,         // An if statement.
	CK_STMT_WHILE,      // A while statement.
	CK_STMT_DO_WHILE,   // A do ... while statement.
	CK_STMT_FOR,        // A for statement.
	CK_STMT_SWITCH,     // A switch statement.
	CK_STMT_BREAK,      // A break statement.
	CK_STMT_CONTINUE,   // A continue statement.
	CK_STMT_GOTO,       // A goto statement.
	CK_STMT_ASSERT,     // An assert statement.
	CK_STMT_SPONGE,     // A sponge statement.
	CK_STMT_RETURN,     // A return statement.

} CkStatementKind;

typedef struct CkStatement CkStatement;
typedef struct CkScope CkScope;
typedef struct CkLibrary CkLibrary;
typedef struct CkModule CkModule;

// A variable declaration.
typedef struct CkVariable
{
	char *name;           // The name of the variable.
	CkScope *parentScope; // The scope that contains the variable.
	CkFoodType *type;     // The type of the variable.
	bool param;           // If this is true, this variable is a function argument.
	CkDeclAttr decl_attr; // Declaration attributes

} CkVariable;

// An address to code.
typedef struct CkLabel
{
	CkScope *parentScope; // The scope that contains the label.
	size_t stmtIndex;     // The index to the statement to go to.

} CkLabel;

// A switch case entry.
typedef struct CkSwitchCase
{
	uint64_t checkFor; // The constant value to check for.
	CkLabel label;     // The label to go to.

} CkSwitchCase;

typedef char CkStatementData_Empty; // Empty statement's data (no data)
typedef CkExpression *CkStatementData_Expression; // Expression statement data

typedef struct CkStatementData_Block // Block statement data
{
	
	CkList *stmts;  // The statements of the block statement. Element type=CkStatement*
	CkScope *scope; // The scope of the block statement.

} CkStatementData_Block;

typedef struct CkStatementData_If // If statement data
{
	
	CkExpression *condition; // The condition of the if statement.
	CkStatement *cThen;      // The statement to execute when the condition is true.
	CkStatement *cElse;      // The statement to execute if the condition is false. Optional.

} CkStatementData_If;

typedef struct CkStatementData_While // While statement data
{
	
	CkExpression *condition; // The condition of the while statement.
	CkStatement *cWhile;     // The statement to be executed while the condition is true.

} CkStatementData_While;

typedef struct CkStatementData_For // For statement data
{
	CkStatement *cInit;      // This is executed once.
	CkExpression *condition; // The condition that is checked before calling the body.
	CkExpression *lead;      // This expression will be performed after the body.
	CkStatement *body;       // The body of the for loop.
	CkScope *scope;          // For loops have their own scopes.

} CkStatementData_For;

typedef struct CkStatementData_Switch // Switch statement data
{
	CkExpression *expression; // The expression to check against the cases.
	CkList *caseList;         // The list of cases to check against. Element type: CkSwitchCase
	CkStatement *block;       // A block statement that stores the body of the switch.

} CkStatementData_Switch;

typedef char CkStatementData_Break;    // Break statement data (no data)
typedef char CkStatementData_Continue; // Continue statement data (no data)

typedef struct CkStatementData_Goto // Goto statement data
{
	bool computed;                    // Computed goto flag
	CkLabel destination;              // The label to go to.
	CkExpression *computedExpression; // The computed location (if not using a label)

} CkStatementData_Goto;

typedef struct CkStatementData_Assert // Assert statement data. Static assert is done at parser-level
{
	CkExpression *expression; // The expression to check.

} CkStatementData_Assert;

typedef CkStatement *CkStatementData_Sponge; // Sponge statement data

typedef CkExpression *CkStatementData_Return; // Return statement data

// The data passed along with a statement.
typedef union CkStatementData
{
	CkStatementData_Empty      empty;
	CkStatementData_Expression expression;
	CkStatementData_Block      block;
	CkStatementData_If         if_;
	CkStatementData_While      while_;
	CkStatementData_While      doWhile; // the same data, not the same interpretation
	CkStatementData_For        for_;
	CkStatementData_Switch     switch_;
	CkStatementData_Break      break_;
	CkStatementData_Continue   continue_;
	CkStatementData_Goto       goto_;
	CkStatementData_Assert     assert;
	CkStatementData_Sponge     sponge;
	CkStatementData_Return     return_;

} CkStatementData;

typedef enum CkUserTypeKind
{
	CK_USERTYPE_STRUCT,
	CK_USERTYPE_RECORD,
	CK_USERTYPE_UNION,
	CK_USERTYPE_ENUM,

} CkUserTypeKind;

// Common structs for the struct, record and union user data types.
typedef struct CkUserTypeData_StructRecordUnion
{
	CkList *members; // The members. Element type = CkVariable

} CkUserTypeData_Struct,
  CkUserTypeData_Record,
  CkUserTypeData_Union;

// A constant stored under an enum.
typedef struct CkEnumConstant
{
	char *name;     // The name of the constant.
	uint64_t value; // The value of the constant.

} CkEnumConstant;

typedef struct CkUserTypeData_Enum
{
	CkFoodType *native;     // The native type used to store the members of the enum.
	CkList *namedConstants; // The list of named constants. Element type = CkEnumConstant.

} CkUserTypeData_Enum;

// Custom data passed for each kind of user type.
typedef union CkUserTypeData
{
	CkUserTypeData_Struct struct_; // As struct
	CkUserTypeData_Record record;  // As record
	CkUserTypeData_Union  union_;  // As union
	CkUserTypeData_Enum   enum_;   // As enum

} CkUserTypeData;

// A user type declaration.
typedef struct CkUserType
{
	CkUserTypeKind kind;      // The kind of user type we are dealing with.
	char           *name;     // The name given to the user type.
	CkUserTypeData custom;    // The custom data of the user type.
	CkDeclAttr     decl_attr; // Declaration attributes

} CkUserType;

// A scope is the frame that stores functions and variables.
typedef struct CkScope
{
	CkLibrary *library;     // The library that contains the scope.
	CkModule *module;       // The module that contains the scope.
	CkScope *parent;        // The scope containing this one. No parent = NULL
	bool supportsLabels;    // If this is true, the scope supports labels (e.g. function bodies)
	bool supportsFunctions; // If this is true, the scope can contain function declarations.
	CkList *variableList;   // The variables declared in this scope. ElementType = CkVariable.
	CkList *labelList;      // The list of labels. Element type = CkLabel. Requires supportsLabels.
	CkList *functionList;   // The list of functions. Element type = CkFunction. Requires supportsFunctions.
	CkList *usertypeList;   // The list of user types. Element type = CkUserType. Requires supportsFunction.
	CkList *children;       // The list of children scopes. Element type = CkScope *

} CkScope;

// A statement is a piece of code that performs operations.
typedef struct CkStatement
{
	CkStatementKind stmt; // The statement.
	CkStatementData data; // The data stored with the statement.
	CkToken         prim; // Primary token.

} CkStatement;

// A library contains many modules. Keep in mind that an
// executable file is also a library.
typedef struct CkLibrary
{
	char *name;               // The namespace of the library (e.g. Food) and its name.
	CkList *moduleList;       // A list containing the library's modules. Element Type = CkModule *
	CkList *dependenciesList; // A list containing all of the library's dependencies. Element Type = char *
	CkScope *scope;           // The global scope of the library.

} CkLibrary;

// A module is an instanceable unit of code that performs a precise
// function.
typedef struct CkModule
{
	char *name;    // The name of the module. This is not the name of an instance.
	bool bPublic;  // If this is true, the module is public.
#if __STDC_VERSION__ > 201710L
	[[deprecated]]
#endif
	bool bStatic;   // Static module declaration
	CkScope *scope; // The scope of the module.

} CkModule;

// A function is a calleable block of code.
typedef struct CkFunction
{
	CkScope *parent;       // The parent scope of the function.
	CkScope *funscope;     // The scope of the function.
	CkFoodType *signature; // The signature of a function.
	char *name;            // The name of the function.
	bool bPublic;          // If this is true, the function is public.
	bool bExtern;          // If this is true, the function is external.
	CkStatement *body;     // The body of the function.
	CkDeclAttr decl_attr;  // Declaration attributes

} CkFunction;

// Creates and starts a new scope.
CkScope *CkStartScope( CkArenaFrame *allocator, CkScope *optionalParent, bool allowedLabels, bool allowedFunctions );

// Attempts to leave the current scope. If this function fails, the current scope is returned.
CkScope *CkLeaveScope( CkScope *current );

// Attempts to allocate a variable in a scope.
void CkAllocateVariable( CkScope *scope, CkFoodType *type, char *passedName, bool param );

// Attempts to allocate a new function in a scope.
CkFunction *CkAllocateFunction(
	CkArenaFrame *frame,
	CkScope *scope,
	bool bPublic,
	CkFoodType *signature,
	char *passedName,
	CkStatement *body );

// Creates a new library that is ready to use.
CkLibrary *CkCreateLibrary( CkArenaFrame *allocator, char *passedName );

// Creates a new module that is ready to use.
CkModule *CkCreateModule(
	CkArenaFrame *allocator,
	CkLibrary *parent,
	char *passedName,
	bool isPublic,
	bool isStatic );

// Prints a whole library's structure.
void CkPrintAST( CkLibrary *library );

// Returns true if a symbol of that name is declared in the scope.
bool CkSymbolDeclared( CkScope* current, char *passedName );

#endif
