
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

#section common
/*
 * Similar to:
 * include/asm-i386/linkage.h
 */
#depend "asm/linux_linkage.h[marcel_macros]"

#section marcel_macros

#ifdef __cplusplus
#  define CPP_ASMLINKAGE extern "C"
#else
#  define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#  define asmlinkage CPP_ASMLINKAGE
#endif

#ifndef FASTCALL
#  define FASTCALL(x)	x
#  define fastcall
#endif

