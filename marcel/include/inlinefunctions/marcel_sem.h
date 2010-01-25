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


#ifndef __INLINEFUNCTIONS_MARCEL_SEM_H__
#define __INLINEFUNCTIONS_MARCEL_SEM_H__


#include "marcel_sem.h"


/** Public inline **/
static __tbx_inline__ int marcel_sem_destroy(marcel_sem_t* s)
{
	(void) s;
	return 0;
}


#endif /** __INLINEFUNCTIONS_MARCEL_SEM_H__ **/
