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


#ifndef __MARCEL_KEYS_H__
#define __MARCEL_KEYS_H__


#include "marcel_config.h"
#include "sys/marcel_types.h"


/** Public macros **/
/* Max number of marcel_key_create() calls
 * - PTHREAD_KEYS_MAX = 1024
 * - POSIX_KEYS_MAX = 128
 * - POSIX_DESTRUCTOR_ITERATIONS = 4
 */
#define MAX_KEY_DESTR_ITER      4

#ifdef MARCEL_KEYS_ENABLED
#define MARCEL_KEYS_MAX  MARCEL_MAX_KEYS
#define PMARCEL_KEYS_MAX MARCEL_MAX_KEYS
#endif


/** Public data types **/
#ifdef MARCEL_KEYS_ENABLED
typedef void (*marcel_key_destructor_t) (any_t);
#endif				/* MARCEL_KEYS_ENABLED */


/** Public functions **/
#ifdef MARCEL_KEYS_ENABLED

DEC_MARCEL(int, key_create, (marcel_key_t * key, marcel_key_destructor_t any_t) __THROW);
DEC_MARCEL(int, key_delete, (marcel_key_t key) __THROW);

DEC_MARCEL(int, setspecific, (marcel_key_t key, __const void *value));
DEC_MARCEL(any_t, getspecific, (marcel_key_t key));

#endif				/* MARCEL_KEYS_ENABLED */


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal global variables **/
#ifdef MARCEL_KEYS_ENABLED
extern unsigned marcel_nb_keys;
extern marcel_key_destructor_t marcel_key_destructor[MARCEL_MAX_KEYS];
extern int marcel_key_present[MARCEL_MAX_KEYS];
extern volatile unsigned _nb_keys;
#endif				/* MARCEL_KEYS_ENABLED */


/** Internal functions **/
#ifdef MARCEL_KEYS_ENABLED
static __tbx_inline__ any_t *marcel_specificdatalocation(marcel_t pid, marcel_key_t key);
#endif				/* MARCEL_KEYS_ENABLED */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_KEYS_H__ **/
