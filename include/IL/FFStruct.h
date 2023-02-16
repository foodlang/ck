/*
 * Fast Food program structure (AST and scopes)
*/

#ifndef FFLIB_FFSTRUCT_H_
#define FFLIB_FFSTRUCT_H_

#include <Memory/List.h>

/// <summary>
/// The kind of a statement.
/// </summary>
typedef enum FFStatementKind
{
	/// <summary>
	/// An empty statement.
	/// </summary>
	FF_STMT_EMPTY,

	/// <summary>
	/// An expression statement.
	/// </summary>
	FF_STMT_EXPRESSION,

	/// <summary>
	/// A block statement.
	/// </summary>
	FF_STMT_BLOCK,

	/// <summary>
	/// An if statement.
	/// </summary>
	FF_STMT_IF,

	/// <summary>
	/// A while statement.
	/// </summary>
	FF_STMT_WHILE,

	/// <summary>
	/// A do ... while statement.
	/// </summary>
	FF_STMT_DO_WHILE,

	/// <summary>
	/// A for statement.
	/// </summary>
	FF_STMT_FOR,

	/// <summary>
	/// A switch statement.
	/// </summary>
	FF_STMT_SWITCH,

	/// <summary>
	/// A break statement.
	/// </summary>
	FF_STMT_BREAK,

	/// <summary>
	/// A continue statement.
	/// </summary>
	FF_STMT_CONTINUE,

	/// <summary>
	/// A goto statement.
	/// </summary>
	FF_STMT_GOTO,

	/// <summary>
	/// An assert statement.
	/// </summary>
	FF_STMT_ASSERT,

	/// <summary>
	/// A sponge statement.
	/// </summary>
	FF_STMT_SPONGE,

} FFStatementKind;

typedef struct FFStatement FFStatement;
typedef struct FFScope FFScope;

/// <summary>
/// A variable declaration.
/// </summary>
typedef struct FFVariable
{
	/// <summary>
	/// The name of the variable.
	/// </summary>
	char *name;

	/// <summary>
	/// The scope that contains the variable.
	/// </summary>
	FFScope *parentScope;

	/// <summary>
	/// The type of the variable.
	/// </summary>
	CkFoodType *type;

} FFVariable;

/// <summary>
/// An address to code.
/// </summary>
typedef struct FFLabel
{
	/// <summary>
	/// The scope that contains the label.
	/// </summary>
	FFScope *parentScope;

	/// <summary>
	/// The index to the statement to go to.
	/// </summary>
	size_t stmtIndex;

} FFLabel;

/// <summary>
/// A switch case entry.
/// </summary>
typedef struct FFSwitchCase
{
	/// <summary>
	/// The constant value to check for.
	/// </summary>
	uint64_t checkFor;

	/// <summary>
	/// The label to go to.
	/// </summary>
	FFLabel label;

} FFSwitchCase;

typedef char FFStatementData_Empty; // Empty statement's data (no data)
typedef CkExpression *FFStatementData_Expression; // Expression statement data

typedef struct FFStatementData_Block // Block statement data
{
	/// <summary>
	/// The statements of the block statement.
	/// </summary>
	CkList *stmts;

	/// <summary>
	/// The scope of the block statement.
	/// </summary>
	FFScope *scope;

} FFStatementData_Block;

typedef struct FFStatementData_If // If statement data
{
	/// <summary>
	/// The condition of the if statement.
	/// </summary>
	CkExpression *condition;

	/// <summary>
	/// The statement to execute when the condition is true.
	/// </summary>
	FFStatement *cThen;

	/// <summary>
	/// The statement to execute if the condition is false. Optional.
	/// </summary>
	FFStatement *cElse;

} FFStatementData_If;

typedef struct FFStatementData_While // While statement data
{
	/// <summary>
	/// The condition of the while statement.
	/// </summary>
	CkExpression *condition;

	/// <summary>
	/// The statement to be executed while the condition is true.
	/// </summary>
	FFStatement *cWhile;

} FFStatementData_While;

typedef struct FFStatementData_For // For statement data
{
	/// <summary>
	/// This is executed once.
	/// </summary>
	FFStatement *cInit;

	/// <summary>
	/// The condition that is checked before calling the body.
	/// </summary>
	CkExpression *condition;

	/// <summary>
	/// This expression will be performed after the body.
	/// </summary>
	CkExpression *lead;

	/// <summary>
	/// The body of the for loop.
	/// </summary>
	FFStatement *body;

} FFStatementData_For;

typedef struct FFStatementData_Switch // Switch statement data
{
	/// <summary>
	/// The expression to check against the cases.
	/// </summary>
	CkExpression *expression;

	/// <summary>
	/// The list of cases to check against. Element type: FFSwitchCase
	/// </summary>
	CkList *caseList;

	/// <summary>
	/// A block statement that stores the body of the switch.
	/// </summary>
	FFStatement *block;

} FFStatementData_Switch;

typedef char FFStatementData_Break; // Break statement data (no data)
typedef char FFStatementData_Continue; // Continue statement data (no data)

typedef struct FFStatementData_Goto // Goto statement data
{
	/// <summary>
	/// The label to go to.
	/// </summary>
	FFLabel destination;

} FFStatementData_Goto;

typedef struct FFStatementData_Assert // Assert statement data. Static assert is done at parser-level
{
	/// <summary>
	/// The expression to check.
	/// </summary>
	CkExpression *expression;

} FFStatementData_Assert;

typedef struct FFStatementData_Sponge // Sponge statement data
{
	/// <summary>
	/// The statement.
	/// </summary>
	FFStatement *statement;
} FFStatementData_Sponge;

/// <summary>
/// The data passed along a statement.
/// </summary>
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

/// <summary>
/// A scope is the frame that stores functions and variables.
/// </summary>
typedef struct FFScope
{
	/// <summary>
	/// The scope containing this one. If there's no higher scope, then this
	/// is NULL.
	/// </summary>
	struct FFScope *parent;

	/// <summary>
	/// If this is true, the scope supports labels (e.g. function bodies)
	/// </summary>
	bool_t supportsLabels;

	/// <summary>
	/// If this is true, the scope can contain function declarations (e.g. modules,
	/// libraries and base function scopes)
	/// </summary>
	bool_t supportsFunctions;

	/// <summary>
	/// The variables declared in this scope.
	/// </summary>
	CkList *variableList;

	/// <summary>
	/// The labels in the current scope. If supportsLabels is FALSE, this list is
	/// not created.
	/// </summary>
	CkList *labelList;

	/// <summary>
	/// The list of functions in the current scope.
	/// </summary>
	CkList *functionList;

} FFScope;

/// <summary>
/// A statement is a piece of code that performs operations.
/// </summary>
typedef struct FFStatement
{
	/// <summary>
	/// The statement.
	/// </summary>
	FFStatementKind stmt;

	/// <summary>
	/// The data stored with the statement.
	/// </summary>
	FFStatementData data;

} FFStatement;

/// <summary>
/// A library contains many modules. Keep in mind that an
/// executable file is also a library.
/// </summary>
typedef struct FFLibrary
{
	/// <summary>
	/// The namespace of the library (e.g. Food) and its name.
	/// </summary>
	char *name;

	/// <summary>
	/// A list containing the library's modules. Element Type = FFModule *
	/// </summary>
	CkList *moduleList;

	/// <summary>
	/// A list containing all of the library's other library dependencies. Element Type = char *
	/// </summary>
	CkList *dependenciesList;

	/// <summary>
	/// The global scope of the library.
	/// </summary>
	FFScope *scope;

} FFLibrary;

/// <summary>
/// A module is an instanceable unit of code that performs a precise
/// function.
/// </summary>
typedef struct FFModule
{
	/// <summary>
	/// The name of the module. This is not the name of an instance.
	/// </summary>
	char *name;

	/// <summary>
	/// If this is true, the module is public.
	/// </summary>
	bool_t bPublic;

	/// <summary>
	/// If this is true, the module cannot be instanced (only one state
	/// should ever exist.)
	/// </summary>
	bool_t bStatic;

	/// <summary>
	/// The scope of the module.
	/// </summary>
	FFScope *scope;

} FFModule;

/// <summary>
/// A function is a calleable block of code.
/// </summary>
typedef struct FFFunction
{
	/// <summary>
	/// The parent scope of the function.
	/// </summary>
	FFScope *parent;

	/// <summary>
	/// The return type of the function.
	/// </summary>
	CkFoodType *returnType;

	/// <summary>
	/// The name of the function.
	/// </summary>
	char *name;

	/// <summary>
	/// If this is true, the module is public.
	/// </summary>
	bool_t bPublic;

	/// <summary>
	/// The body of the function.
	/// </summary>
	FFStatement *body;

	/// <summary>
	/// The list of all of the arguments. Passed.
	/// </summary>
	CkList *passedArguments;

} FFFunction;

/// <summary>
/// Creates and starts a new scope.
/// </summary>
/// <returns></returns>
FFScope *FFStartScope( CkArenaFrame *allocator, FFScope *optionalParent, bool_t allowedLabels, bool_t allowedFunctions );

/// <summary>
/// Attempts to leave the current scope. If this function fails, the current scope is returned.
/// </summary>
/// <param name="current"></param>
/// <returns></returns>
FFScope *FFLeaveScope( FFScope *current );

/// <summary>
/// Attempts to allocate a variable in a scope.
/// </summary>
/// <returns></returns>
void FFAllocateVariable( FFScope *scope, CkFoodType *type, char *passedName );

/// <summary>
/// Attempts to allocate a new function in a scope.
/// </summary>
/// <returns></returns>
void FFAllocateFunction(
	FFScope *scope,
	bool_t bPublic,
	CkFoodType *returnType,
	char *passedName,
	CkList *pPassedArgumentList,
	FFStatement *body );

/// <summary>
/// Creates a new library that is ready to use.
/// </summary>
/// <param name="name">The name of the library.</param>
/// <returns></returns>
FFLibrary *FFCreateLibrary( CkArenaFrame *allocator, char *passedName );

/// <summary>
/// Creates a new module that is ready to use.
/// </summary>
/// <param name="name">The name of the module.</param>
/// <param name="isPublic">If this is true, the module should be exported outside of the library.</param>
/// <param name="isStatic">If this is true, only once state of the module can exist in the program's lifetime.</param>
/// <returns></returns>
FFModule *FFCreateModule(
	CkArenaFrame *allocator,
	FFLibrary *parent,
	char *passedName,
	bool_t isPublic,
	bool_t isStatic );

/// <summary>
/// Prints a whole library's structure.
/// </summary>
/// <param name="library"></param>
void FFPrintAST( FFLibrary *library );

#endif
