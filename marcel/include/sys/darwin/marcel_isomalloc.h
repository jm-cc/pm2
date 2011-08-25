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


#ifndef __SYS_DARWIN_MARCEL_ISOMALLOC_H__
#define __SYS_DARWIN_MARCEL_ISOMALLOC_H__


#include "tbx_compiler.h"
#include "marcel_config.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Experimental chosen value: may fail on untested system **/

#ifdef PPC64_ARCH
#  define ISOADDR_AREA_TOP       0x7000000000000
#  define SLOT_AREA_BOTTOM       0x90000000
#elif defined(X86_ARCH)
#  define ISOADDR_AREA_TOP       0x80000000
#  define SLOT_AREA_BOTTOM       0x2000000
#elif defined(X86_64_ARCH)
#  define ISOADDR_AREA_TOP       0x400000000
#  define SLOT_AREA_BOTTOM       0x200000000
#else
#  error Sorry. This architecture is not yet supported.
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_DARWIN_MARCEL_ISOMALLOC_ARCHDEP_H__ **/
