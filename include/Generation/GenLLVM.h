/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the LLVM generator.
 *
 ***************************************************************************/

#ifndef CK_GEN_LLVM_H_
#define CK_GEN_LLVM_H_

// Syntax highlighting
typedef struct LLVMModuleRef LLVMModuleRef;
typedef struct LLVMTypeRef LLVMTypeRef;
typedef struct LLVMValueRef LLVMValueRef;
typedef struct LLVMBuilderRef LLVMBuilderRef;
typedef struct LLVMBasicBlockRef LLVMBasicBlockRef;

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

#include "../IL/FFStruct.h"
#include "../Diagnostics.h"

LLVMModuleRef CkCompileLibrariesLLVM(
	CkDiagnosticHandlerInstance *pDhi,
	CkList *libs/* elem=CkLibrary* */ );

#endif