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


#include "sys/marcel_flags.h"
#include "marcel_types.h"


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
extern marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC];
extern int marcel_key_present[MAX_KEY_SPECIFIC];
extern volatile unsigned _nb_keys;
#endif				/* MARCEL_KEYS_ENABLED */


/** Internal functions **/
#ifdef MARCEL_KEYS_ENABLED
static __tbx_inline__ any_t *marcel_specificdatalocation(marcel_t pid, marcel_key_t key);
#endif				/* MARCEL_KEYS_ENABLED */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_KEYS_H__ **/
