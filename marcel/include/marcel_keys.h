
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#section common
#include "tbx_compiler.h"
/*********************************************************************
 * thread_key 
 * */
/* ====== specific data ====== */
#section macros
#section types
#section functions
#section inline
#depend "[marcel_variables]"
#section marcel_variables [no-depend-previous]

#section common
#ifdef MARCEL_KEYS_ENABLED

#section types
/* Keys for thread-specific data */
typedef unsigned int marcel_key_t,  pmarcel_key_t;
typedef void (*marcel_key_destructor_t)(any_t);

#section marcel_variables
extern unsigned marcel_nb_keys;
extern marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC];
extern int marcel_key_present[MAX_KEY_SPECIFIC];

#section functions
DEC_MARCEL_POSIX(int, key_create, (marcel_key_t *key, 
				   marcel_key_destructor_t any_t) __THROW);
DEC_MARCEL_POSIX(int, key_delete, (marcel_key_t key) __THROW);

#section marcel_variables
extern volatile unsigned _nb_keys;

#section functions
DEC_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
				    __const void* value));
DEC_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key));

#section marcel_variables
extern int marcel_key_present[MAX_KEY_SPECIFIC];

#section marcel_functions
static __tbx_inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key);

#section marcel_inline
#depend "marcel_descr.h[types]"
#depend "marcel_descr.h[structures]"
#depend "marcel_descr.h[marcel_inline]"
static __tbx_inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key)
{
	if ((key < 0) || (key >= MAX_KEY_SPECIFIC)
	    || (!marcel_key_present[key]))
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	return &pid->key[key];
}

#section common
#endif /* MARCEL_KEYS_ENABLED */

#section functions
int *pmarcel___errno_location(void);
int *pmarcel___h_errno_location(void);
DEC_MARCEL_POSIX(int *, __errno_location, (void));
DEC_MARCEL_POSIX(int *, __h_errno_location, (void));

#section macros
#define marcel_errno (*marcel___errno_location())
#define pmarcel_errno (*pmarcel___errno_location())
#define marcel_h_errno (*marcel___h_errno_location())
#define pmarcel_h_errno (*pmarcel___h_errno_location())
