
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

#section marcel_macros

#ifdef MA__PROVIDE_TLS
#define marcel_ctx_set_tls_reg(new_task) \
  do {
    unsigned short val = ((((unsigned long)(new_task)) & ~THREAD_SLOT_SIZE) / THREAD_SLOT_SIZE - 1) * 8 | 0x4;
    asm volatile ("movw %w0, %%gs" : : "q" (val));
  } while(0)
#else
#define marcel_ctx_set_tls_reg(new_task) (void)0
#endif

/* marcel_create : passage père->fils */
#define marcel_ctx_set_new_stack(new_task, top, cur_top) \
  do { \
    /* on a besoin d'avoir un 0 à *ebp */ \
    unsigned long _local = ((unsigned long)(cur_top)) - get_sp(); \
    unsigned long *_bp = (unsigned long *)(((unsigned long)(top)) - 2*sizeof(unsigned long)); \
    unsigned long _sp = ((unsigned long)_bp) - _local; \
    /* marqueur de fin de pile */ \
    *_bp = 0; \
    marcel_ctx_set_tls_reg(new_task); \
    set_sp_bp(_sp, _bp); \
  } while (0)

/* marcel_deviate : passage temporaire sur une autre pile */
#define marcel_ctx_switch_stack(from_task, to_task, top, cur_top) \
	marcel_ctx_set_new_stack(to_task, top, cur_top)

#section common
#depend "asm-generic/marcel_ctx.h[]"
#section structures
#section macros
#section marcel_macros
