
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
#include "tbx_debug.h"

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
