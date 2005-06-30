
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
#ifndef MA__LWPS
#  define SCHED_YIELD()  do {} while(0)
#else // MA__LWP
#  if defined(SOLARIS_SYS)
#    include <thread.h>
#    define SCHED_YIELD() \
do { \
	thr_yield(); \
} while (0)
#  else // pas SOLARIS
#    include <sched.h>
#    define SCHED_YIELD() \
do { \
	sched_yield(); \
} while (0)
#  endif
#endif // MA__LWP

#define cpu_relax() SCHED_YIELD()
