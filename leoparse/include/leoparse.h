/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * leoparse.h
 * ----------
 */

#ifndef LEOPARSE_H
#define LEOPARSE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif // __GNUC__

#include "pm2_common.h"
#include "tbx_debug.h"
#include "tbx.h"

/* Leoparse: data structures */
#include "leoparse_pointers.h"
#include "leoparse_types.h"
#ifndef LEOPARSE_IN_YACC
/*#include "leoparse_parser.h"*/
#include <leoparse_parser.h>
#endif /* LEOPARSE_IN_YACC */

/* Leoparse: interface */
#include "leoparse_interface.h"

#ifndef YYLTYPE_IS_DECLARED
#define YYLTYPE struct yylloc

struct yylloc {
       int first_line;
       int first_column;
       int last_line;
       int last_column;
};

extern YYLTYPE yylloc;
#endif /* YYLTYPE */

#endif /* LEOPARSE_H */
