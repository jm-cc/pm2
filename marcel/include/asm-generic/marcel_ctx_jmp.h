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


#ifndef __ASM_GENERIC_MARCEL_CTX_JMP_H__
#define __ASM_GENERIC_MARCEL_CTX_JMP_H__


#include "asm/marcel_archdep.h"
#include "sys/marcel_archsetjmp.h"
#include "sys/inlinefunctions/marcel_archsetjmp.h"


/** Public macros **/
#define marcel_ctx_getcontext(ctx)		\
	setjmp(ctx[0].jbuf)
#define marcel_ctx_setjmp(ctx) marcel_ctx_getcontext(ctx)

#define marcel_ctx_setcontext(ctx, ret)		\
	longjmp(ctx[0].jbuf, ret)
#define marcel_ctx_longjmp(ctx, ret) marcel_ctx_setcontext(ctx, ret)

#ifdef MA__DEBUG
#define marcel_ctx_destroycontext(ctx)			\
	memset(&ctx[0].jbuf, 0xa5, sizeof(ctx[0].jbuf))
#else
#define marcel_ctx_destroycontext(ctx)
#endif
#define marcel_ctx_destroyjmp(ctx) marcel_ctx_destroycontext(ctx)

#define marcel_ctx_get_sp(ctx)			\
	(SP_FIELD(ctx[0].jbuf))

#ifdef FP_FIELD
#define marcel_ctx_get_fp(ctx)			\
	(FP_FIELD(ctx[0].jbuf))
#endif

#ifdef BSP_FIELD
#define marcel_ctx_get_bsp(ctx)			\
	(BSP_FIELD(ctx[0].jbuf))
#endif


/** Public data structures **/
typedef struct marcel_ctx {	/* C++ doesn't like tagless structs.  */
	jmp_buf jbuf;
} marcel_ctx_t[1];


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#ifndef marcel_ctx_set_tls_reg
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif

#ifdef set_sp_fp
#define marcel_generic_ctx_set_new_stack(new_task, top, cur_top)	\
	do {								\
		unsigned long _local1 = ((unsigned long)(cur_top)) - get_sp(); \
		unsigned long _local2 = ((unsigned long)(cur_top)) - get_fp(); \
		unsigned long _sp = ((unsigned long)(top)) - MAL(_local1); \
		unsigned long _fp = ((unsigned long)(top)) - MAL(_local2); \
		marcel_ctx_set_tls_reg(new_task);			\
		MA_SET_INITIAL_SELF(new_task);				\
		call_ST_FLUSH_WINDOWS();				\
		set_sp_fp(_sp, _fp);					\
	} while (0)
#else
#define marcel_generic_ctx_set_new_stack(new_task, top, cur_top)	\
	do {								\
		unsigned long _local = ((unsigned long)(cur_top)) - get_sp(); \
		unsigned long _sp = ((unsigned long)(top)) - _local;	\
		marcel_ctx_set_tls_reg(new_task);			\
		MA_SET_INITIAL_SELF(new_task);				\
		call_ST_FLUSH_WINDOWS();				\
		set_sp(_sp);						\
	} while (0)
#endif

/* marcel_create : passage père->fils */
#ifndef marcel_ctx_set_new_stack
#define marcel_ctx_set_new_stack(new_task, top, cur_top)		\
	marcel_generic_ctx_set_new_stack(new_task, top, cur_top)
#endif

/* marcel_deviate : passage temporaire sur une autre pile */
#ifndef marcel_ctx_switch_stack
#define marcel_ctx_switch_stack(from_task, to_task, top, cur_top)	\
	marcel_generic_ctx_set_new_stack(to_task, top, cur_top)
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_MARCEL_CTX_JMP_H__ **/
