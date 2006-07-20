
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

/* effectue un setjmp. On doit �tre RUNNING avant et apr�s
 * */
#define MA_THR_SETJMP(current) \
  marcel_ctx_setjmp(current->ctx_yield)

/* On vient de reprendre la main.
 * */
#define MA_THR_RESTARTED(current, info) \
  do {                                 \
    MA_ARCH_SWITCHTO_LWP_FIX(current); \
    MA_ACT_SET_THREAD(current); \
    MTRACE(info, current);             \
  } while(0)

/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * �tre RUNNING. La variable previous_task doit �tre correctement
 * positionn�e pour pouvoir annuler la propri�t� RUNNING du thread
 * courant.
 * */
#define MA_THR_LONGJMP(cur_num, next, ret) \
  do {                                     \
    PROF_SWITCH_TO(cur_num, next); \
    call_ST_FLUSH_WINDOWS();               \
    marcel_ctx_set_tls_reg(next); \
    marcel_ctx_longjmp(next->ctx_yield, ret);              \
  } while(0)


/* d�truit le contexte en mettant des valeurs empoisonn�es.
 * Utile pour le d�buggage. */
#define MA_THR_DESTROYJMP(current) \
  marcel_ctx_destroyjmp(current->ctx_yield);

#section marcel_functions
marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next);
