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


#ifndef __SYS_LINUX_MARCEL_MISC_H__
#define __SYS_LINUX_MARCEL_MISC_H__


#include "tbx_compiler.h"
#include "marcel_config.h"
#include "sys/marcel_types.h"
#include <sys/utsname.h>
#ifdef MA__LWPS
#  include <sched.h>
#endif


/** Public functions **/
long marcel_random(void);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MA_WARN_ABOUT_MALLOC() \
	do { MA_WARN_USER("this program (%s) appears to provide its own malloc(3)" \
			  ", which may not be thread-safe\n", program_invocation_name ? : "unknown"); \
	} while (0);

#define MA_CHCK_SYSRUN()						\
	do { struct utsname utsname;					\
	uname(&utsname);						\
	if (strncmp(utsname.release, LINUX_VERSION, strlen(LINUX_VERSION))) { \
		MA_WARN_USER("was compiled for Linux %s, but you are running Linux %s, can't continue\n", \
			     LINUX_VERSION, utsname.version);		\
		exit(1);						\
	}while (0);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_MARCEL_MISC_H__ **/
