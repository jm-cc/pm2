
/*
 * Tbx.h
 * ===========
 */

#ifndef TBX_H
#define TBX_H

/*
 * Checks
 * ------
 */
#ifndef __GNUC__
#error The GNU C Compiler is required to build the library
#endif __GNUC__

/*
 * Headers
 * -------
 */
#ifdef MARCEL
#include "marcel.h"
#endif /* MARCEL */
#include "pm2debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "tbx_timing.h"
#include "tbx_macros.h"
#include "tbx_pointers.h"
#include "tbx_types.h"

#include "tbx_malloc.h"
#include "tbx_slist.h"
#include "tbx_list.h"
#include "tbx_htable.h"

#include "tbx_interface.h"

#endif /* TBX_H */
