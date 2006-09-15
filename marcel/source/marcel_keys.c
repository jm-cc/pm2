
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

#include "marcel.h"
#include <errno.h>

marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC]={NULL};
int marcel_key_present[MAX_KEY_SPECIFIC]={0};
static marcel_lock_t marcel_key_lock=MARCEL_LOCK_INIT;
unsigned marcel_nb_keys=1;
static unsigned marcel_last_key=0;

DEF_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
				    __const void* value), (key, value),
{
   int ret = 0;
   if ((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key])) {
      MA_WARN_ON(1);
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_setspecific : valeur key(%d) invalide\n",key);
#endif
      ret = EINVAL;
   } else
      SELF_GETMEM(key)[key] = (any_t)value;
   return ret;
})
DEF_PTHREAD(int, setspecific, (pthread_key_t key,
				    __const void* value), (key, value))
DEF___PTHREAD(int, setspecific, (pthread_key_t key,
				    __const void* value), (key, value))

DEF_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key), (key),
{
   any_t ret;
   if ((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key])) {
      MA_WARN_ON(1);
#ifdef MA__DEBUG
      fprintf(stderr,"(p)marcel_setspecific : valeur key(%d) invalide\n",key);
#endif
      errno = EINVAL;
      ret = NULL;
   } else
      ret = SELF_GETMEM(key)[key];
   return ret;
})
DEF_PTHREAD(any_t, getspecific, (pthread_key_t key), (key))
DEF___PTHREAD(any_t, getspecific, (pthread_key_t key), (key))

/* 
 * Hummm... Should be 0, but for obscure reasons,
 * 0 is a RESERVED value. DON'T CHANGE IT !!! 
*/

DEF_MARCEL_POSIX(int, key_create, (marcel_key_t *key, 
				   marcel_key_destructor_t func), (key, func),
{ /* pour l'instant, le destructeur n'est pas utilise */

   marcel_lock_acquire(&marcel_key_lock);
   while ((++marcel_last_key < MAX_KEY_SPECIFIC) &&
	  (marcel_key_present[marcel_last_key])) {
   }
   if(marcel_last_key == MAX_KEY_SPECIFIC) {
     /* sinon, il faudrait remettre � 0 toutes les valeurs sp�cifiques
	des threads existants */
      marcel_lock_release(&marcel_key_lock);
      MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
/*        marcel_last_key=0; */
/*        while ((++marcel_last_key < MAX_KEY_SPECIFIC) && */
/*  	     (marcel_key_present[marcel_last_key])) { */
/*        } */
/*        if(new_key == MAX_KEY_SPECIFIC) { */
/*  	 marcel_lock_release(&marcel_key_lock); */
/*  	 MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR); */
/*        } */
   }
   *key = marcel_last_key;
   marcel_nb_keys++;
   marcel_key_present[marcel_last_key]=1;
   marcel_key_destructor[marcel_last_key]=func;
   marcel_lock_release(&marcel_key_lock);
   return 0;
})
DEF_PTHREAD(int, key_create, (pthread_key_t *key, 
				   void (*func)(void *)), (key, func))
DEF___PTHREAD(int, key_create, (pthread_key_t *key, 
				   void (*func)(void *)), (key, func))

DEF_MARCEL_POSIX(int, key_delete, (marcel_key_t key), (key),
{ /* pour l'instant, le destructeur n'est pas utilise */

   marcel_lock_acquire(&marcel_key_lock);
   if (marcel_key_present[key]) {
      marcel_nb_keys--;
      marcel_key_present[key]=0;
      marcel_key_destructor[key]=NULL;
   }
   marcel_lock_release(&marcel_key_lock);
   return 0;
})
DEF_PTHREAD(int, key_delete, (pthread_key_t key), (key))
//DEF___PTHREAD(int, key_delete, (pthread_key_t key), (key))

#undef errno
#pragma weak errno
DEF_MARCEL_POSIX(int *, __errno_location,(void),(),
{
	int * res;

#ifdef MA__PROVIDE_TLS
	extern __thread int errno;
	res=&errno;
#else
	static int _first_errno;

	if (ma_init_done[MA_INIT_TLS]) {
		res=&SELF_GETMEM(__errno);
	} else {
		res=&_first_errno;
	}
#endif
	return res;
})
DEF_C(int *, __errno_location,(void),());
DEF___C(int *, __errno_location,(void),());

#undef h_errno
#pragma weak h_errno
DEF_MARCEL_POSIX(int *, __h_errno_location,(void),(),
{
	int * res;

#ifdef MA__PROVIDE_TLS
	extern __thread int h_errno;
	res=&h_errno;
#else
	static int _first_h_errno;

	if (ma_init_done[MA_INIT_TLS]) {
		res=&SELF_GETMEM(__h_errno);
	} else {
		res=&_first_h_errno;
	}
#endif
	return res;
})
extern int *__h_errno_location(void);
DEF_C(int *, __h_errno_location,(void),());
DEF___C(int *, __h_errno_location,(void),());

#ifdef MA__LIBPTHREAD
/* Return thread specific resolver state.  */
struct __res_state *lpt___res_state(void)
{
	struct __res_state * res;
	static struct __res_state _fisrt_res_state;

	if (ma_init_done[MA_INIT_TLS]) {
		res=&SELF_GETMEM(__res_state);
	} else {
		res=&_fisrt_res_state;
	}
	return res;
}
extern struct __res_state *__res_state(void);
DEF_LIBC(struct __res_state *, __res_state, (void), ());
DEF___LIBC(struct __res_state *, __res_state, (void), ());
#endif /* MA__LIBPTHREAD */
