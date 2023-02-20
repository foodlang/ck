/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header declares the type binder stage. The binder runs between the
 * parser and the generator. It checks unresolved symbols, performs type
 * binding and verifies the usage of statements.
 *
 ***************************************************************************/

#ifndef CK_BINDER_H_
#define CK_BINDER_H_

#include <IL/FFStruct.h>

// Performs type binding + validates a library.
void CkBinderValidateAndBind( FFLibrary *library );

#endif
