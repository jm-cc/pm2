
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
#include "marcel_pmarcel.h"
#include <errno.h>

#ifdef MARCEL_KEYS_ENABLED
/* a marcel_key_t is just an index in the key[] array of marcel_task_t */

marcel_key_destructor_t marcel_key_destructor[MARCEL_MAX_KEYS]={NULL};
int marcel_key_present[MARCEL_MAX_KEYS]={0};
static ma_spinlock_t marcel_key_lock=MA_SPIN_LOCK_UNLOCKED;
unsigned marcel_nb_keys=1;
static unsigned marcel_last_key=0;

DEF_MARCEL_PMARCEL(int, setspecific, (marcel_key_t key, __const void* value), (key, value),
{
	int ret = 0;
	MARCEL_LOG_IN();
	if (key >= MARCEL_MAX_KEYS || (!marcel_key_present[key])) {
		MA_WARN_ON(1);
		MARCEL_LOG("(p)marcel_setspecific : valeur key(%d) invalide\n", key);
		ret = EINVAL;
	} else
		SELF_GETMEM(key)[key] = (any_t) value;
	MARCEL_LOG_RETURN(ret);
})
DEF_PTHREAD(int, setspecific, (pthread_key_t key, __const void* value), (key, value))
DEF___PTHREAD(int, setspecific, (pthread_key_t key, __const void* value), (key, value))

DEF_MARCEL_PMARCEL(any_t, getspecific, (marcel_key_t key), (key),
{
	any_t ret;
	MARCEL_LOG_IN();
	if (key >= MARCEL_MAX_KEYS || (!marcel_key_present[key])) {
		MA_WARN_ON(1);
		MARCEL_LOG("(p)marcel_setspecific : valeur key(%d) invalide\n", key);
		errno = EINVAL;
		ret = NULL;
	} else
		ret = SELF_GETMEM(key)[key];
	MARCEL_LOG_RETURN(ret);
})
DEF_PTHREAD(any_t, getspecific, (pthread_key_t key), (key))
DEF___PTHREAD(any_t, getspecific, (pthread_key_t key), (key))

/* 
 * Hummm... Should be 0, but for obscure reasons,
 * 0 is a RESERVED value. DON'T CHANGE IT !!! 
 */
DEF_MARCEL_PMARCEL(int, key_create, (marcel_key_t *key, marcel_key_destructor_t func), (key, func),
{
	MARCEL_LOG_IN();

#ifdef MA__LIBPTHREAD
	if (tbx_unlikely(!marcel_test_activity()))
		/* This hack aims to work around a bootstrap problem: We may be called by
		   the GNU C Library when PukABI is initializing (e.g., calling
		   `dlsym()') but Marcel itself is not yet initialized.  Since we know
		   the GNU C Library, specifically `init()' in
		   `$(GLIBC)/dlfcn/dlerror.c', is able to gracefully handle a situation
		   where `pthread_key_create(3)' fails, we shamelessly fail.  */
		MARCEL_LOG_RETURN(EAGAIN);
#endif
	/* Posix says:
	 * Upon key creation, the value NULL shall be associated with the new key in all active threads.
	 * Let this job to the marcel_key_delete (cost less because key_delete is called less often)
	 * Invariant: new key is always NULL *before* key_create call */

	ma_spin_lock(&marcel_key_lock);
	while ((marcel_key_present[marcel_last_key]) && (++marcel_last_key < MARCEL_MAX_KEYS));

	if (marcel_last_key >= MARCEL_MAX_KEYS) {
		ma_spin_unlock(&marcel_key_lock);
		MARCEL_LOG_RETURN(EAGAIN);
	}

	*key = marcel_last_key;
	marcel_nb_keys++;
	marcel_key_present[marcel_last_key] = 1;
	marcel_key_destructor[marcel_last_key] = func;
	ma_spin_unlock(&marcel_key_lock);
	MARCEL_LOG_RETURN(0);
})
DEF_PTHREAD(int, key_create, (pthread_key_t *key, void (*func)(void *)), (key, func)) 
DEF___PTHREAD(int, key_create, (pthread_key_t *key, void (*func)(void *)), (key, func))

DEF_MARCEL_PMARCEL(int, key_delete, (marcel_key_t key), (key),
{
	int number;
	marcel_t t;
	struct marcel_topo_level *vp;

	/* Note: This function does *not* invoke KEY's destructors.  Quoting
	   http://www.opengroup.org/onlinepubs/009695399/functions/pthread_key_delete.html,
	   "It is the responsibility of the application to free any application
	   storage or perform any cleanup actions for data structures related to
	   the deleted key or associated thread-specific data in any threads".  */

	MARCEL_LOG_IN();

	ma_spin_lock(&marcel_key_lock);
	if (key < MARCEL_MAX_KEYS && marcel_key_present[key]) {
		marcel_nb_keys--;
		marcel_key_present[key] = 0;
		marcel_key_destructor[key] = NULL;
		/* rewind !!! */
		if (key < marcel_last_key)
			marcel_last_key = key;
		ma_spin_unlock(&marcel_key_lock);

		/* do the key_create job: NULLize this key is all active threads */
		number = ma_vpnum(MA_LWP_SELF);
		for_all_vp_from_begin(vp, number) {
			ma_spin_lock(&ma_topo_vpdata_l(vp, threadlist_lock));
			tbx_fast_list_for_each_entry(t, &ma_topo_vpdata_l(vp, all_threads), all_threads)
				t->key[key] = NULL;
			ma_spin_unlock(&ma_topo_vpdata_l(vp, threadlist_lock));
		} for_all_vp_from_end(vp, number);

		MARCEL_LOG_RETURN(0);
	}
	ma_spin_unlock(&marcel_key_lock);
	MARCEL_LOG_RETURN(EAGAIN);
})
DEF_PTHREAD(int, key_delete, (pthread_key_t key), (key))
#endif /* MARCEL_KEYS_ENABLED */

#ifdef MA__LIBPTHREAD
/* Return thread specific resolver state.  */
extern struct __res_state *lpt___res_state(void);
struct __res_state *lpt___res_state(void)
{
	struct __res_state *res;
	static struct __res_state _first_res_state;
	if (ma_init_done[MA_INIT_TLS]) {
		res = &SELF_GETMEM(__res_state);
	} else
		res = &_first_res_state;
	return (res);
}
versioned_symbol(libpthread, lpt___res_state, __res_state, GLIBC_2_2);
#endif /* MA__LIBPTHREAD */
