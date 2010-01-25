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


#ifndef __INLINEFUNCTIONS_MARCEL_KEYS_H__
#define __INLINEFUNCTIONS_MARCEL_KEYS_H__


#include "marcel_keys.h"
#ifdef __MARCEL_KERNEL__
#include "marcel_descr.h"
#endif


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
#ifdef MARCEL_KEYS_ENABLED
static __tbx_inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key)
{
	if (key >= MAX_KEY_SPECIFIC
	    || (!marcel_key_present[key]))
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	return &pid->key[key];
}
#endif /* MARCEL_KEYS_ENABLED */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_KEYS_H__ **/
