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


#ifndef __ASM_IA64_MARCEL_CTX_H__
#define __ASM_IA64_MARCEL_CTX_H__


#include <ucontext.h>
#include "tbx_compiler.h"
#include "asm/linux_types.h"


/** Public macros **/
int TBX_RETURNS_TWICE ma_ia64_setjmp(ucontext_t * ucp);
int TBX_NORETURN ma_ia64_longjmp(const ucontext_t * ucp, int ret);

#define marcel_ctx_getcontext(ctx)		\
	ma_ia64_setjmp(&(ctx[0].jbuf))
#define marcel_ctx_setjmp(ctx) marcel_ctx_getcontext(ctx)

#define marcel_ctx_setcontext(ctx, ret)		\
	ma_ia64_longjmp(&(ctx[0].jbuf), ret)
#define marcel_ctx_longjmp(ctx, ret) marcel_ctx_setcontext(ctx, ret)

#ifdef MA__DEBUG
#define marcel_ctx_destroycontext(ctx)			\
	memset(&ctx[0].jbuf, 0, sizeof(ctx[0].jbuf))
#else
#define marcel_ctx_destroycontext(ctx)
#endif

#define marcel_ctx_destroyjmp(ctx) marcel_ctx_destroycontext(ctx)

#define marcel_ctx_get_bsp(ctx)			\
	(BSP_FIELD(ctx[0].jbuf))

#define marcel_ctx_get_sp(ctx)			\
	(SP_FIELD(ctx[0].jbuf))


/** Public data structures **/
typedef struct __marcel_ctx_tag {	/* C++ doesn't like tagless structs.  */
	ucontext_t jbuf;
} marcel_ctx_t[1];


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
/* marcel_create : passage p�re->fils */
/* marcel_exit : d�placement de pile */
#define marcel_ctx_set_new_stack(new_task, top, cur_top)		\
	do {								\
		/* NOTE: sur ia64 on ne sait pas pr�cis�ment combien il faut de marge !! */ \
		unsigned long _local = ((unsigned long)(cur_top)) - get_sp_fresh(); \
		unsigned long _sp = ((unsigned long)(top)) - MAL(_local) - 0x200; \
		marcel_ctx_set_tls_reg(new_task);			\
		MA_SET_INITIAL_SELF(new_task);				\
		ma_ST_FLUSH_WINDOWS();				\
		set_sp_bsp(_sp, new_task->stack_base);			\
	} while (0)

/* marcel_deviate : passage temporaire un peu plus bas sur la pile */
#define marcel_ctx_switch_stack(from_task, to_task, top, cur_top)	\
	do {								\
		unsigned long _local = ((unsigned long)(cur_top)) - get_sp(); \
		unsigned long _sp = ((unsigned long)(top)) - MAL(_local) - 0x200; \
		marcel_ctx_set_tls_reg(to_task);			\
		ma_ST_FLUSH_WINDOWS();				\
		set_sp_bsp(_sp, marcel_ctx_get_bsp(to_task->ctx_yield)); \
	} while (0)


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_IA64_MARCEL_CTX_H__ **/
