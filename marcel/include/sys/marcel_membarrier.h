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
 *
 * 
 * Memory barrier public functions
 */


#ifndef __MARCEL_MEMBARRIER_H__
#define __MARCEL_MEMBARRIER_H__


#include "sys/marcel_flags.h"
#include "marcel_compiler.h"
#include "asm/linux_system.h"


/** Public functions **/
void marcel_mb(void);
void marcel_rmb(void);
void marcel_wmb(void);
void marcel_barrier(void);


#endif /** __MARCEL_MEMBARRIER_H__ **/
