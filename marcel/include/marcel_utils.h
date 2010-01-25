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


#ifndef __MARCEL_UTILS_H__
#define __MARCEL_UTILS_H__


#include "sys/marcel_flags.h"


/** Public macros **/
#define marcel_xstr(s) marcel_str(s)
#define marcel_str(s) #s
#define marcel_id(s) s
#define __ma_stringify(x)          marcel_str(x)
#define MA_PROFILE_TID(tid) ((long)(tid))


/** Public data types **/
#ifndef __ASSEMBLY__
typedef void* any_t;
typedef void (*marcel_handler_func_t)(any_t);
#endif


#endif /** __MARCEL_UTILS_H__ **/
