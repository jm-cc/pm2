
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

#section macros
#depend "sys/isomalloc_archdep.h"

#ifdef DO_PROFILE
/*
 * How to get the current TID in assembly, for tracing purpose. Not used
 * at the moment as the switch events are enough to know it.
 */
//#define MA_PROFILE_ASM_GET_TID(reg)
#endif
