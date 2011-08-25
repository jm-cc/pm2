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


/***********************sigjmps******************************/
#ifdef MA__LIBPTHREAD


int ma_savesigs(sigjmp_buf env, int savemask)
{
#ifdef MA__DEBUG
	if (env == NULL) {
		MARCEL_LOG("ma_savesigs: env is NULL\n");
		errno = EINVAL;
		return -1;
	}
#endif
#ifdef MARCEL_SIGNALS_ENABLED
	env->__mask_was_saved = savemask;
	if (savemask != 0)
		lpt_sigmask(SIG_BLOCK, NULL, &env->__saved_mask);
#endif
	return 0;
}

extern void __libc_longjmp(sigjmp_buf env, int val) __attribute__((noreturn));
DEF_MARCEL_PMARCEL(void, siglongjmp, (sigjmp_buf env, int val), (env,val),
{	
#ifdef MA__DEBUG
	if (tbx_unlikely(env == NULL)) {
		MARCEL_LOG("(p)marcel_siglongjump: env is NULL\n");
		errno = EINVAL;
	}
#endif

#ifdef MARCEL_SIGNALS_ENABLED
	if (env->__mask_was_saved)
		lpt_sigmask(SIG_SETMASK, &env->__saved_mask, NULL);
#endif
	// TODO: vérifier qu'on longjumpe bien dans le même thread !!

#if defined(X86_ARCH) || defined(X86_64_ARCH)
	/* We define our own sigsetjmp using our own setjmp function, so
	 * use our own longjmp.  */
	ma_longjmp(env->__jmpbuf, val);
#else
	__libc_longjmp(env, val);
#endif
})
DEF_C(void, siglongjmp, (sigjmp_buf env, int val), (env,val))

/** define longjmp: break #define longjmp ma_longjmp 
    for internal optimisation **/
#undef longjmp
DEF_WEAK_ALIAS(PMARCEL_NAME(siglongjmp), longjmp)


#endif /** MA__LIBPTHREAD **/
