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


#if defined(MARCEL_LIBPTHREAD) && defined(__GLIBC__)


#  ifdef MARCEL_CLEANUP_ENABLED
#    ifdef PM2_DEV
#      warning TODO: _pthread_cleanup_push_defer,pop_restore
/* _defer and _restore version have to handle cancellation. See NPTL's
 * cleanup_defer_compat.c */
#    endif

/* Glibc/NPTL cleanup function stubs.  These symbols are required, e.g., by
 * `librt.so' from Glibc 2.7, with version `GLIBC_2.3.3'.
 *
 * pthread_cleanup_push and pthread_cleanup_pop use the following __pthread functions
 *
 * pthread_cleanup_push:
 *   do a sigsetjmp: registered cleanup handlers are called after a longjmp.
 *   we use marcel_cleanup{push|pop} mechanism, we store where to jump in cleanup_buf
 * 
 * pthread_cleanup_pop:
 *   call the routine previously pushed, so we just need to undo what we did with
 *   marcel_cleanup_push without calling the handler (need to free memory allocated !!!) 
 */
static void
ma_cleanup_siglongjmp(void *buf)
{
	struct __jmp_buf_tag *env = (struct __jmp_buf_tag *)((__pthread_unwind_buf_t *)buf)->__cancel_jmp_buf;
	marcel_siglongjmp(env, 1);
}

void __ma_cleanup_fct_attribute
__pthread_register_cancel(__pthread_unwind_buf_t * buf)
{
	struct _marcel_cleanup_buffer *cleanup_buf;

	cleanup_buf = (struct _marcel_cleanup_buffer *)marcel_malloc(sizeof(*cleanup_buf));
	if (! cleanup_buf)
		abort();
	
	cleanup_buf->__routine = ma_cleanup_siglongjmp;
	cleanup_buf->__arg = buf;
	cleanup_buf->__prev = SELF_GETMEM(lpt_last_cleanup);
	SELF_SETMEM(lpt_last_cleanup, cleanup_buf);
}

void __ma_cleanup_fct_attribute
__pthread_unregister_cancel(__pthread_unwind_buf_t * buf TBX_UNUSED)
{
	struct _marcel_cleanup_buffer *old_cleanup, *toexec_cleanup;

	old_cleanup = SELF_GETMEM(lpt_last_cleanup);
	if (old_cleanup) {
		toexec_cleanup = old_cleanup->__prev;
		SELF_SETMEM(lpt_last_cleanup, toexec_cleanup);
		marcel_free(old_cleanup);
	}
}

void __ma_cleanup_fct_attribute
__pthread_unwind_next(__pthread_unwind_buf_t * buf TBX_UNUSED)
{
	struct _marcel_cleanup_buffer *old_cleanup, *toexec_cleanup;

	old_cleanup = SELF_GETMEM(lpt_last_cleanup);
	if (old_cleanup) {
		toexec_cleanup = old_cleanup->__prev;
		SELF_SETMEM(lpt_last_cleanup, toexec_cleanup);

		free(old_cleanup);
		if (toexec_cleanup)
			(*toexec_cleanup->__routine)(toexec_cleanup->__arg);
	}

	pthread_exit(SELF_GETMEM(ret_val));
}

/* behave almost like _marcel_cleanup_pop but do not update last_cleanup buffer list.
 * lpt_last_cleanup buffer elements are dynamically allocated, they must be freed
 * after handler call.
 */
void 
ma_lpt_cleanup_pop(struct _marcel_cleanup_buffer *__buffer, int execute)
{
	MARCEL_LOG_IN();
	marcel_t cur = ma_self();

	if (cur->lpt_last_cleanup != __buffer)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);

	if (execute)
		(*__buffer->__routine) (__buffer->__arg);
	MARCEL_LOG_OUT();
}

#  endif /* MARCEL_CLEANUP_ENABLED */


#endif /** MARCEL_LIBPTHREAD & GLIBC **/
