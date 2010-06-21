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


#ifndef __MARCEL_ARCHDEP_H__
#define __MARCEL_ARCHDEP_H__


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define get_sp()				\
	({					\
		unsigned long long __var;	\
		(unsigned long) (&__var + 1);	\
	})


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_ARCHDEP_H__ **/
