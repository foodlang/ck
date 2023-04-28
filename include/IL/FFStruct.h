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

// The kind of a statement.
typedef enum CkStatementKind
{
	// An empty statement.
	CK_STMT_EMPTY,

	// An expression statement.
	CK_STMT_EXPRESSION,

	// A block statement.
	CK_STMT_BLOCK,

	// An if statement.
	CK_STMT_IF,

	// A while statement.
	CK_STMT_WHILE,

	// A do ... while statement.
	CK_STMT_DO_WHILE,

	// A for statement.
	CK_STMT_FOR,

	// A switch statement.
	CK_STMT_SWITCH,

	// A break statement.
	CK_STMT_BREAK,

	// A continue statement.
	CK_STMT_CONTINUE,

	// A goto statement.
	CK_STMT_GOTO,

	// An assert statement.
	CK_STMT_ASSERT,

	// A sponge statement.
	CK_STMT_SPONGE,

	// A return statement.
	CK_STMT_RETURN,

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
	bool_t param;         // If this is true, this variable is a function argument.

} CkVariable;

// An address to code.
typedef struct CkLabel
{
	// The scope that contains the label.
	CkScope *parentScope;

	// The index to the statement to go to.
	size_t stmtIndex;

} CkLabel;

// A switch case entry.
typedef struct CkSwitchCase
{
	// The constant value to check for.
	uint64_t checkFor;

	// The label to go to.
	CkLabel label;

} CkSwitchCase;

typedef char CkStatementData_Empty; // Empty statement's data (no data)
typedef CkExpression *CkStatementData_Expression; // Expression statement data

typedef struct CkStatementData_Block // Block statement data
{
	// The statements of the block statement. Element type=CkStatement*
	CkList *stmts;

	// The scope of the block statement.
	CkScope *scope;

} CkStatementData_Block;

typedef struct CkStatementData_If // If statement data
{
	// The condition of the if statement.
	CkExpression *condition;

	// The statement to execute when the condition is true.
	CkStatement *cThen;

	// The statement to execute if the condition is false. Optional.
	CkStatement *cElse;

} CkStatementData_If;

typedef struct CkStatementData_While // While statement data
{
	// The condition of the while statement.
	CkExpression *condition;

	// The statement to be executed while the condition is true.
	CkStatement *cWhile;

} CkStatementData_While;

typedef struct CkStatementData_For // For statement data
{
	// This is executed once.
	CkStatement *cInit;

	// The condition that is checked before calling the body.
	CkExpression *condition;

	// This expression will be performed after the body.
	CkExpression *lead;

	// The body of the for loop.
	CkStatement *body;
	
	// For loops have their own scopes.
	CkScope *scope;

} CkStatementData_For;

typedef struct CkStatementData_Switch // Switch statement data
{
	// The expression to check against the cases.
	CkExpression *expression;

	// The list of cases to check against. Element type: CkSwitchCase
	CkList *caseList;

	// A block statement that stores the body of the switch.
	CkStatement *block;

} CkStatementData_Switch;

typedef char CkStatementData_Break; // Break statement data (no data)
typedef char CkStatementData_Continue; // Continue statement data (no data)

typedef struct CkStatementData_Goto // Goto statement data
{
	// Computed goto flag
	bool_t computed;

	// The label to go to.
	CkLabel destination;

	// The computed location (if not using a label)
	CkExpression *computedExpression;

} CkStatementData_Goto;

typedef struct CkStatementData_Assert // Assert statement data. Static assert is done at parser-level
{
	// The expression to check.
	CkExpression *expression;

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
	// The members. Element type = CkVariable
	CkList *members;

} CkUserTypeData_Struct,
  CkUserTypeData_Record,
  CkUserTypeData_Union;

// A constant stored under an enum.
typedef struct CkEnumConstant
{
	// The name of the constant.
	char *name;

	// The value of the constant.
	uint64_t value;

} CkEnumConstant;

typedef struct CkUserTypeData_Enum
{
	// The native type used to store the members of the enum.
	CkFoodType *native;

	// The list of named constants. Element type = CkEnumConstant.
	CkList *namedConstants;

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
	// The kind of user type we are dealing with.
	CkUserTypeKind kind;

	// The name given to the user type.
	char           *name;

	// The custom data of the user type.
	CkUserTypeData custom;

} CkUserType;

// A scope is the frame that stores functions and variables.
typedef struct CkScope
{
	// The library that contains the scope.
	CkLibrary *library;

	// The module that contains the scope.
	CkModule *module;

	// The scope containing this one. If there's no higher scope, then this
	// is NULL.
	CkScope *parent;

	// If this is true, the scope supports labels (e.g. function bodies)
	bool_t supportsLabels;

	// If this is true, the scope can contain function declarations (e.g. modules,
	// libraries and base function scopes)
	bool_t supportsFunctions;

	// The variables declared in this scope. ElementType = CkVariable.
	CkList *variableList;

	// The labels in the current scope. If supportsLabels is FALSE, this list is
	// not created. Element type = CkLabel.
	CkList *labelList;

	// The list of functions in the current scope. Element type = CkFunction.
	// Requires supportsFunction.
	CkList *functionList;

	// The list of user types. Element type = CkUserType. Requires supportsFunction.
	CkList *usertypeList;

	// The list of children scopes. Element type = CkScope *
	CkList *children;

} CkScope;

// A statement is a piece of code that performs operations.
typedef struct CkStatement
{
	// The statement.
	CkStatementKind stmt;

	// The data stored with the statement.
	CkStatementData data;

} CkStatement;

// A library contains many modules. Keep in mind that an
// executable file is also a library.
typedef struct CkLibrary
{
	// The namespace of the library (e.g. Food) and its name.
	char *name;

	// A list containing the library's modules. Element Type = CkModule *
	CkList *moduleList;

	// A list containing all of the library's other library dependencies. Element Type = char *
	CkList *dependenciesList;

	// The global scope of the library.
	CkScope *scope;

} CkLibrary;

// A module is an instanceable unit of code that performs a precise
// function.
typedef struct CkModule
{
	// The name of the module. This is not the name of an instance.
	char *name;

	// If this is true, the module is public.
	bool_t bPublic;

	// If this is true, the module cannot be instanced (only one state
	// should ever exist.)
	bool_t bStatic;

	// The scope of the module.
	CkScope *scope;

} CkModule;

// A function is a calleable block of code.
typedef struct CkFunction
{
	// The parent scope of the function.
	CkScope *parent;

	// The scope of the function.
	CkScope *funscope;

	// The signature of a function.
	CkFoodType *signature;

	// The name of the function.
	char *name;

	// If this is true, the module is public.
	bool_t bPublic;

	// The body of the function.
	CkStatement *body;

} CkFunction;

// Creates and starts a new scope.
CkScope *CkStartScope( CkArenaFrame *allocator, CkScope *optionalParent, bool_t allowedLabels, bool_t allowedFunctions );

// Attempts to leave the current scope. If this function fails, the current scope is returned.
CkScope *CkLeaveScope( CkScope *current );

// Attempts to allocate a variable in a scope.
void CkAllocateVariable( CkScope *scope, CkFoodType *type, char *passedName, bool_t param );

// Attempts to allocate a new function in a scope.
CkFunction *CkAllocateFunction(
	CkArenaFrame *frame,
	CkScope *scope,
	bool_t bPublic,
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
	bool_t isPublic,
	bool_t isStatic );

// Prints a whole library's structure.
void CkPrintAST( CkLibrary *library );

// Returns true if a symbol of that name is declared in the scope.
bool_t CkSymbolDeclared( CkScope* current, char *passedName );

#endif
