
/*
 * leoparse.h
 * ----------
 */

#ifndef __LEOPARSE_H
#define __LEOPARSE_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build this tool
#endif __GNUC__

#include "common.h"
#include "pm2debug.h"
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

#endif /* __LEOPARSE_H */
