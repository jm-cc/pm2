
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

#ifndef MARCEL_CTX_H_EST_DEF
#define MARCEL_CTX_H_EST_DEF

_PRIVATE_ enum {
	FIRST_RETURN,
	NORMAL_RETURN
};


/* set/longjmp based on get/setcontext due to the stack register */	

#include <ucontext.h>

typedef struct __marcel_ctx_tag { /* C++ doesn't like tagless structs.  */
		ucontext_t jbuf;
} marcel_ctx_t[1];

int ia64_setjmp(ucontext_t *ucp);
int ia64_longjmp(const ucontext_t *ucp, int ret);

#define marcel_ctx_getcontext(ctx) \
  ia64_setjmp(&(ctx[0].jbuf))
#define marcel_ctx_setjmp(ctx) marcel_ctx_getcontext(ctx)

#define marcel_ctx_setcontext(ctx, ret) \
	  ia64_longjmp(&(ctx[0].jbuf), ret)
#define marcel_ctx_longjmp(ctx, ret) marcel_ctx_setcontext(ctx, ret)

	/* marcel_create : passage père->fils */
#define marcel_ctx_set_new_stack(new_task, new_sp) \
  do { \
      call_ST_FLUSH_WINDOWS(); \
      set_sp_bsp(new_sp, new_task->stack_base); \
  } while (0)

/* marcel_deviate : passage temporaire sur une autre pile */
#define marcel_ctx_switch_stack(from_task, to_task, new_sp) \
  do { \
      call_ST_FLUSH_WINDOWS(); \
      set_sp_bsp(new_sp, marcel_ctx_get_bsp(to_task->ctx_yield)); \
  } while (0)

#define marcel_ctx_get_bsp(ctx) \
  (BSP_FIELD(ctx[0].jbuf))

#define marcel_ctx_get_sp(ctx) \
	  (SP_FIELD(ctx[0].jbuf))

#endif /* MARCEL_CTX_H_EST_DEF */

