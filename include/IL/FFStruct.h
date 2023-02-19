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

#ifndef FFLIB_FFSTRUCT_H_
#define FFLIB_FFSTRUCT_H_

#include <Memory/List.h>

// The kind of a statement.
typedef enum FFStatementKind
{
	// An empty statement.
	FF_STMT_EMPTY,

	// An expression statement.
	FF_STMT_EXPRESSION,

	// A block statement.
	FF_STMT_BLOCK,

	// An if statement.
	FF_STMT_IF,

	// A while statement.
	FF_STMT_WHILE,

	// A do ... while statement.
	FF_STMT_DO_WHILE,

	// A for statement.
	FF_STMT_FOR,

	// A switch statement.
	FF_STMT_SWITCH,

	// A break statement.
	FF_STMT_BREAK,

	// A continue statement.
	FF_STMT_CONTINUE,

	// A goto statement.
	FF_STMT_GOTO,

	// An assert statement.
	FF_STMT_ASSERT,

	// A sponge statement.
	FF_STMT_SPONGE,

} FFStatementKind;

typedef struct FFStatement FFStatement;
typedef struct FFScope FFScope;
typedef struct FFLibrary FFLibrary;
typedef struct FFModule FFModule;

// A variable declaration.
typedef struct FFVariable
{
	// The name of the variable.
	char *name;

	// The scope that contains the variable.
	FFScope *parentScope;

	// The type of the variable.
	CkFoodType *type;

} FFVariable;


// An address to code.
typedef struct FFLabel
{
	// The scope that contains the label.
	FFScope *parentScope;

	// The index to the statement to go to.
	size_t stmtIndex;

} FFLabel;

// A switch case entry.
typedef struct FFSwitchCase
{
	// The constant value to check for.
	uint64_t checkFor;

	// The label to go to.
	FFLabel label;

} FFSwitchCase;

typedef char FFStatementData_Empty; // Empty statement's data (no data)
typedef CkExpression *FFStatementData_Expression; // Expression statement data

typedef struct FFStatementData_Block // Block statement data
{
	// The statements of the block statement.
	CkList *stmts;

	// The scope of the block statement.
	FFScope *scope;

} FFStatementData_Block;

typedef struct FFStatementData_If // If statement data
{
	// The condition of the if statement.
	CkExpression *condition;

	// The statement to execute when the condition is true.
	FFStatement *cThen;

	// The statement to execute if the condition is false. Optional.
	FFStatement *cElse;

} FFStatementData_If;

typedef struct FFStatementData_While // While statement data
{
	// The condition of the while statement.
	CkExpression *condition;

	// The statement to be executed while the condition is true.
	FFStatement *cWhile;

} FFStatementData_While;

typedef struct FFStatementData_For // For statement data
{
	// This is executed once.
	FFStatement *cInit;

	// The condition that is checked before calling the body.
	CkExpression *condition;

	// This expression will be performed after the body.
	CkExpression *lead;

	// The body of the for loop.
	FFStatement *body;

} FFStatementData_For;

typedef struct FFStatementData_Switch // Switch statement data
{
	// The expression to check against the cases.
	CkExpression *expression;

	// The list of cases to check against. Element type: FFSwitchCase
	CkList *caseList;

	// A block statement that stores the body of the switch.
	FFStatement *block;

} FFStatementData_Switch;

typedef char FFStatementData_Break; // Break statement data (no data)
typedef char FFStatementData_Continue; // Continue statement data (no data)

typedef struct FFStatementData_Goto // Goto statement data
{
	// The label to go to.
	FFLabel destination;

} FFStatementData_Goto;

typedef struct FFStatementData_Assert // Assert statement data. Static assert is done at parser-level
{
	// The expression to check.
	CkExpression *expression;

} FFStatementData_Assert;

typedef struct FFStatementData_Sponge // Sponge statement data
{
	// The statement.
	FFStatement *statement;
} FFStatementData_Sponge;


// The data passed along a statement.
typedef union FFStatementData
{
	FFStatementData_Empty      empty;
	FFStatementData_Expression expression;
	FFStatementData_Block      block;
	FFStatementData_If         if_;
	FFStatementData_While      while_;
	FFStatementData_While      doWhile; // the same data, not the same interpretation
	FFStatementData_For        for_;
	FFStatementData_Switch     switch_;
	FFStatementData_Break      break_;
	FFStatementData_Continue   continue_;
	FFStatementData_Goto       goto_;
	FFStatementData_Assert     assert;
	FFStatementData_Sponge     sponge;

} FFStatementData;

// A scope is the frame that stores functions and variables.
typedef struct FFScope
{
	// The library that contains the scope.
	FFLibrary *library;

	// The module that contains the scope.
	FFModule *module;

	// The scope containing this one. If there's no higher scope, then this
	// is NULL.
	FFScope *parent;

	// If this is true, the scope supports labels (e.g. function bodies)
	bool_t supportsLabels;

	// If this is true, the scope can contain function declarations (e.g. modules,
	// libraries and base function scopes)
	bool_t supportsFunctions;

	// The variables declared in this scope.
	CkList *variableList;

	// The labels in the current scope. If supportsLabels is FALSE, this list is
	// not created.
	CkList *labelList;

	// The list of functions in the current scope.
	CkList *functionList;

} FFScope;

// A statement is a piece of code that performs operations.
typedef struct FFStatement
{
	// The statement.
	FFStatementKind stmt;

	// The data stored with the statement.
	FFStatementData data;

} FFStatement;

// A library contains many modules. Keep in mind that an
// executable file is also a library.
typedef struct FFLibrary
{
	// The namespace of the library (e.g. Food) and its name.
	char *name;

	// A list containing the library's modules. Element Type = FFModule *
	CkList *moduleList;

	// A list containing all of the library's other library dependencies. Element Type = char *
	CkList *dependenciesList;

	// The global scope of the library.
	FFScope *scope;

} FFLibrary;

// A module is an instanceable unit of code that performs a precise
// function.
typedef struct FFModule
{
	// The name of the module. This is not the name of an instance.
	char *name;

	// If this is true, the module is public.
	bool_t bPublic;

	// If this is true, the module cannot be instanced (only one state
	// should ever exist.)
	bool_t bStatic;

	// The scope of the module.
	FFScope *scope;

} FFModule;

// A function is a calleable block of code.
typedef struct FFFunction
{
	// The parent scope of the function.
	FFScope *parent;

	// The return type of the function.
	CkFoodType *returnType;

	// The name of the function.
	char *name;

	// If this is true, the module is public.
	bool_t bPublic;

	// The body of the function.
	FFStatement *body;

	// The list of all of the arguments. Passed.
	CkList *passedArguments;

} FFFunction;

// Creates and starts a new scope.
FFScope *FFStartScope( CkArenaFrame *allocator, FFScope *optionalParent, bool_t allowedLabels, bool_t allowedFunctions );

// Attempts to leave the current scope. If this function fails, the current scope is returned.
FFScope *FFLeaveScope( FFScope *current );

// Attempts to allocate a variable in a scope.
void FFAllocateVariable( FFScope *scope, CkFoodType *type, char *passedName );

// Attempts to allocate a new function in a scope.
void FFAllocateFunction(
	FFScope *scope,
	bool_t bPublic,
	CkFoodType *returnType,
	char *passedName,
	CkList *pPassedArgumentList,
	FFStatement *body );

// Creates a new library that is ready to use.
FFLibrary *FFCreateLibrary( CkArenaFrame *allocator, char *passedName );

// Creates a new module that is ready to use.
FFModule *FFCreateModule(
	CkArenaFrame *allocator,
	FFLibrary *parent,
	char *passedName,
	bool_t isPublic,
	bool_t isStatic );

// Prints a whole library's structure.
void FFPrintAST( FFLibrary *library );

#endif
