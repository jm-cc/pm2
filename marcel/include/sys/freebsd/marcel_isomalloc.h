/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __SYS_FREEBSD_MARCEL_ISOMALLOC_H__
#define __SYS_FREEBSD_MARCEL_ISOMALLOC_H__


#include "tbx_compiler.h"
#include "marcel_config.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Experimental chosen value: may fail on untested system **/

#ifdef X86_ARCH
#  define ISOADDR_AREA_TOP       0x40000000
#else
#  error Sorry. This system/architecture is not yet supported.
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_FREEBSD_MARCEL_ISOMALLOC_ARCHDEP_H__ **/
