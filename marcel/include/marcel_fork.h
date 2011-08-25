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


#ifndef __MARCEL_FORK_H__
#define __MARCEL_FORK_H__


#include "marcel_config.h"
#include "sys/marcel_alias.h"


/** Public functions **/
DEC_MARCEL(int, atfork, (void (*prepare)(void), void (*parent)(void), void (*child)(void)));
DEC_MARCEL(pid_t, fork, (void));
DEC_MARCEL(void, kill_other_threads_np, (void));


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** child counter **/
extern unsigned long int ma_fork_generation;


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_FORK_H__ **/
