
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
#include "tbx_compiler.h"

#section marcel_types
enum {
	FIRST_RETURN,
	NORMAL_RETURN
};

#section marcel_macros
#depend "asm/marcel_arch_switchto.h[]"

/* effectue un setjmp. On doit être RUNNING avant et après
 * */
#define MA_THR_SETJMP(current) \
  marcel_ctx_setjmp(current->ctx_yield)

/* On vient de reprendre la main.
 * */
#define MA_THR_RESTARTED(current, info) \
  do {                                 \
    MA_ARCH_SWITCHTO_LWP_FIX(current); \
    MTRACE(info, current);             \
  } while(0)

#ifdef MA__SELF_VAR
#  define MA_SET_INITIAL_SELF(self)	do { ma_self = (self); } while(0)
#  if defined(MA__LWPS) && defined(MARCEL_DONT_USE_POSIX_THREADS)
#    define MA_SET_SELF(self)	((void)0)
#  else
#    define MA_SET_SELF(self)	MA_SET_INITIAL_SELF(self)
#  endif
#else
#  define MA_SET_INITIAL_SELF(self)	((void)0)
#  define MA_SET_SELF(self)	((void)0)
#endif
/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * être RUNNING. La variable previous_task doit être correctement
 * positionnée pour pouvoir annuler la propriété RUNNING du thread
 * courant.
 * */
#define MA_THR_LONGJMP(cur_num, next, ret) \
  do {                                     \
    PROF_SWITCH_TO(cur_num, next); \
    marcel_ctx_set_tls_reg(next); \
    MA_SET_SELF(next); \
    call_ST_FLUSH_WINDOWS();               \
    marcel_ctx_longjmp(next->ctx_yield, ret);              \
  } while(0)


/* détruit le contexte en mettant des valeurs empoisonnées.
 * Utile pour le débuggage. */
#define MA_THR_DESTROYJMP(current) \
  marcel_ctx_destroyjmp(current->ctx_yield);

#section marcel_functions
marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next);
