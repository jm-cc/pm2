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


#ifndef __SYS_MARCEL_UTILS_H__
#define __SYS_MARCEL_UTILS_H__


#include "marcel_config.h"


/** Public constants **/
#define MARCEL_TEST_SUCCEEDED 0
#define MARCEL_TEST_FAILED    1
#define MARCEL_TEST_SKIPPED   77


/** Public macros **/
#define marcel_xstr(s) marcel_str(s)
#define marcel_str(s) #s
#define marcel_id(s) s
#define __ma_stringify(x)          marcel_str(x)
#define MA_PROFILE_TID(tid) ((long)(tid))
#define MA_FAILURE_RETRY(expression)					\
	(__extension__							\
	 ({ long int __result;						\
		 do __result = (long int) (expression);			\
		 while (__result == -1L && errno == EINTR);		\
		 __result; }))

#define MA_NOT_IMPLEMENTED(symbol)      marcel_fprintf(stderr, "Marcel: %s is not yet implemented\n", symbol);
#define MA_NOT_SUPPORTED(functionality) marcel_fprintf(stderr, "Marcel: %s is not supported\n", functionality);
#define MA_WARN_USER(format, ...)       marcel_fprintf(stderr, "Marcel: "format, ##__VA_ARGS__)


/** Public data types **/
#ifndef __ASSEMBLY__
typedef void *any_t;
typedef void (*marcel_handler_func_t) (any_t);
#endif


#endif /** __SYS_MARCEL_UTILS_H__ **/
