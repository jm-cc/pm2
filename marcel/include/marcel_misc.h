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


#ifndef __MARCEL_MISC_H__
#define __MARCEL_MISC_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "marcel_types.h"
#include "marcel_alias.h"


/* ========== once objects ============ */
#define MARCEL_ONCE_INITIALIZER { 0 }

/* ============== stack ============= */
TBX_FMALLOC marcel_t marcel_alloc_stack(unsigned size) __tbx_deprecated__;
unsigned long marcel_usablestack(void);

/* ============= miscellaneous ============ */
void marcel_start_playing(void);

#if defined(LINUX_SYS) || defined(GNU_SYS)
long marcel_random(void);
#endif

DEC_MARCEL(int, atfork,
	   (void (*prepare) (void), void (*parent) (void), void (*child) (void)) __THROW);


#endif /** __MARCEL_MISC_H__ **/
