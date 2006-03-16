
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

#section marcel_macros

#include "sys/marcel_flags.h"
#include "sys/marcel_win_sys.h"

#  define TOP_STACK_FREE_AREA     256

/* Linux PPC */
#if defined(LINUX_SYS)
#  define SP_FIELD(buf)           ((buf)->__jmpbuf[JB_GPR1])
#endif

/* Darwin PPC (Mac OS X) */
#if defined(DARWIN_SYS)
#  define STACK_INFO
#  define SP_FIELD(buf)           ((buf)[0])
#endif

#if defined(AIX_SYS)
#  define SP_FIELD(buf)           ((buf)[3])
#endif

#define call_ST_FLUSH_WINDOWS()  ((void)0)

extern void set_sp(unsigned long);
extern unsigned long get_sp(void);
extern void set_fp(unsigned long);
extern unsigned long get_fp(void);

#section marcel_variables
