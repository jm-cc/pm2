
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

/* Note: ce fichier est aussi utilisé pour x86_64 */

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

#define marcel_ctx_get_sp(ctx) \
  (SP_FIELD(ctx[0].jbuf))

#define marcel_ctx_get_fp(ctx) \
  (FP_FIELD(ctx[0].jbuf))

#section marcel_macros
/* marcel_create : passage père->fils */
#define marcel_ctx_set_new_stack(new_task, top, cur_top) \
  do { \
    /* on a besoin d'avoir un 0 à *ebp */ \
    unsigned long _local = ((unsigned long)(cur_top)) - get_sp(); \
    unsigned long *_bp = (unsigned long *)(((unsigned long)(top)) - 2*sizeof(unsigned long)); \
    unsigned long _sp = ((unsigned long)_bp) - _local; \
    /* marqueur de fin de pile */ \
    *_bp = 0; \
    set_sp_bp(_sp, _bp); \
  } while (0)

/* marcel_deviate : passage temporaire sur une autre pile */
#define marcel_ctx_switch_stack(from_task, to_task, top, cur_top) \
  do { \
    unsigned long _local = ((unsigned long)(cur_top)) - get_sp(); \
    unsigned long *_bp = (unsigned long *)(((unsigned long)(top)) - 2*sizeof(unsigned long)); \
    unsigned long _sp = ((unsigned long)_bp) - _local; \
    /* marqueur de fin de pile */ \
    *_bp = 0; \
    set_sp_bp(_sp, _bp); \
  } while (0)
