
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

#section macros
#define MAX_KEY_SPECIFIC	20

#section types
/* Keys for thread-specific data */
typedef unsigned int marcel_key_t;
typedef void (*marcel_key_destructor_t)(any_t);

#section marcel_variables
extern unsigned marcel_nb_keys;
extern marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC];
extern int marcel_key_present[MAX_KEY_SPECIFIC];
extern marcel_lock_t marcel_key_lock;

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


#section functions
static __tbx_inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key);
#section inline
#depend "marcel_threads.h[marcel_structures]"
static __tbx_inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key)
{
#ifdef MA__DEBUG
   if((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key]))
      RAISE(CONSTRAINT_ERROR);
#endif
   return &pid->key[key];
}

