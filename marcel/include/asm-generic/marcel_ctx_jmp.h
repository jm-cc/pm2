
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

#section common
#depend "asm-generic/marcel_ctx_types.h[]"

#section structures
#include "sys/marcel_archsetjmp.h"
typedef struct marcel_ctx { /* C++ doesn't like tagless structs.  */
	jmp_buf jbuf;
} marcel_ctx_t[1];

#section macros
#define marcel_ctx_getcontext(ctx) \
  setjmp(ctx[0].jbuf)
#define marcel_ctx_setjmp(ctx) marcel_ctx_getcontext(ctx)

#define marcel_ctx_setcontext(ctx, ret) \
  longjmp(ctx[0].jbuf, ret)
#define marcel_ctx_longjmp(ctx, ret) marcel_ctx_setcontext(ctx, ret)

#section marcel_macros
/* marcel_create : passage père->fils */
#define marcel_ctx_set_new_stack(new_task, new_sp) \
  do { \
    call_ST_FLUSH_WINDOWS(); \
    set_sp(new_sp); \
  } while (0)

/* marcel_deviate : passage temporaire sur une autre pile */
#define marcel_ctx_switch_stack(from_task, to_task, new_sp) \
  do { \
    call_ST_FLUSH_WINDOWS(); \
    set_sp(new_sp); \
  } while (0)

#define marcel_ctx_get_sp(ctx) \
  (SP_FIELD(ctx[0].jbuf))

#ifdef FP_FIELD
#define marcel_ctx_get_fp(ctx) \
  (FP_FIELD(ctx[0].jbuf))
#endif

#ifdef BSP_FIELD
#define marcel_ctx_get_bsp(ctx) \
  (BSP_FIELD(ctx[0].jbuf))
#endif
