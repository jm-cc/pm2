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


#ifndef __SYS_SOLARIS_MARCEL_MISC_H__
#define __SYS_SOLARIS_MARCEL_MISC_H__


#include "marcel_config.h"
#include "tbx_compiler.h"
#ifdef MA__LWPS
#  include <thread.h>
#endif


/** Public macros **/
#define marcel_random() random()


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MA_WARN_ABOUT_MALLOC()						\
	do { MA_WARN_USER("this program appears to provide its own malloc(3)" \
			  ", which may not be thread-safe\n");		\
	} while (0);

#define MA_CHCK_SYSRUN() (void)0;


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_SOLARIS_MARCEL_MISC_H__ **/
