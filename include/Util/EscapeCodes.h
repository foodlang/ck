/***************************************************************************
 *
 * Copyright (C) 2023 The Food Project
 * Authors:
 *   - \e
 *
 * This header escape code macros.
 *
 ***************************************************************************/

#ifndef CK_ESCAPE_CODES_H_
#define CK_ESCAPE_CODES_H_

#define ESC   "\x1B["
#define B256F "38;5;"
#define B256B "48;5;"

#define ESCB256F ESC B256F
#define ESCB256B ESC B256B

#define RED256     "9m"
#define GREEN256   "10m"
#define BLUE256    "12m"
#define YELLOW256  "11m"
#define MAGENTA256 "13m"
#define CYAN256    "14m"
#define WHITE256   "15m"
#define ESCRESET   ESC "0m"

#endif
