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


#ifndef __ASM_I386_MARCEL_ARCH_SWITCHTO_H__
#define __ASM_I386_MARCEL_ARCH_SWITCHTO_H__


#include "sys/marcel_flags.h"
#include "asm-generic/marcel_arch_switchto.h"


#ifdef MA__SELF_VAR_TLS
#  if defined(X86_ARCH) && defined(__GLIBC__)
#    define MA_SET_INITIAL_SELF(self)					\
 	do { unsigned long tmp TBX_UNUSED;				\
	     asm volatile ("mov __ma_self@INDNTPOFF, %0;\n\t"           \
			   "mov %1, %%gs:(%0)" : "=&r"(tmp) : "r"(self)); \
	} while (0)
#  endif
#endif


#endif /** __ASM_I386_MARCEL_ARCH_SWITCHTO_H__ **/
