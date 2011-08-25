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


#ifndef __SYS_LINUX_MARCEL_ISOMALLOC_H__
#define __SYS_LINUX_MARCEL_ISOMALLOC_H__


#include "tbx_compiler.h"
#include "marcel_config.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Experimental chosen value: may fail on untested system
 *  Look cat /proc/self/maps and find a free region ! */

#ifdef X86_ARCH
#  define ISOADDR_AREA_TOP       0x40000000
#  define SLOT_AREA_BOTTOM       0x10000000
#elif defined(X86_64_ARCH)
#  ifdef PM2VALGRIND
     /* Valgrind doesn't like us taking a lot of adressing space. */
#    define ISOADDR_AREA_TOP       0x20000000
#    define SLOT_AREA_BOTTOM       0x18000000
#  elif defined(MA__PROVIDE_TLS)
     /* segmentation for TLS is restricted to 32bits */
#    define ISOADDR_AREA_TOP       0x100000000
#    define SLOT_AREA_BOTTOM       0x10000000
#  else
#    define ISOADDR_AREA_TOP       0x2000000000
#    define SLOT_AREA_BOTTOM       0x10000000
#  endif
#elif defined(IA64_ARCH)
#  define ISOADDR_AREA_TOP       0x10000000000
#  define SLOT_AREA_BOTTOM       0x100000000
#elif defined(PPC64_ARCH)
#  define ISOADDR_AREA_TOP       0x40000000000
#else
#  error Sorry. This architecture is not yet supported.
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_ISOMALLOC_ARCHDEP_H__ **/
