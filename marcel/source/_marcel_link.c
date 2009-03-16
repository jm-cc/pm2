
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
 * This file is used as a replacement for marcel.lds for the systems where GNU
 * ld is not available.  As such, it has to define __ma_debug_start and
 * __ma_debug_end as the beginning and end of section .ma.debug.var, for
 * marcel_debug.c to find it.  For that, we use zero-length arrays which
 * (hopefully) define symbols but do not allocate room for them.
 *
 * It needs to be included first while linking, to let the linker's (hopefully)
 * first-come first-served algorithm just allocate sections and thus symbols in
 * the order we want.
 */
#if !defined(LINUX_SYS) && !defined(GNU_SYS) && !defined(OSF_SYS)
#include <sys/types.h>
#include "tbx_macros.h"
#include "tbx_compiler.h"

#ifdef PM2DEBUG
#  ifdef DARWIN_SYS
     /* Unfortunately, the Darwin linker does allocate some room for
      * zero-length arrays, so we are out of luck and just disable the debug
      * option, to at least produce a program that does not always crash on
      * startup */
#    warning Marcel s debug was enabled but it will not work on Darwin
#  else
TBX_SECTION(".ma.debug.start") TBX_ALIGN(4096)	const int __ma_debug_start[0]={};
TBX_SECTION(".ma.debug.var")			const int __ma_debug_var[0]={};
TBX_SECTION(".ma.debug.end")			const int __ma_debug_end[0]={};

TBX_SECTION(".ma.debug.size") TBX_ALIGN(4096)	const int __ma_debug_size[0]={};
TBX_SECTION(".ma.debug.size.0")			const int __ma_debug_size_0[0]={};
TBX_SECTION(".ma.debug.size.1")			const int __ma_debug_size_1[0]={};
#  endif
#endif

#endif
