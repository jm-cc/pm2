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
    MA_ACT_SET_THREAD(current); \
    MTRACE(info, current);             \
  } while(0)

/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * être RUNNING. La variable previous_task doit être correctement
 * positionnée pour pouvoir annuler la propriété RUNNING du thread
 * courant.
 * */
#define MA_THR_LONGJMP(cur_num, next, ret) \
  do {                                     \
    PROF_SWITCH_TO(cur_num, next); \
    call_ST_FLUSH_WINDOWS();               \
    marcel_ctx_longjmp(next->ctx_yield, ret);              \
  } while(0)


#section marcel_functions
static __tbx_setjmp_inline__
marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next);

#section marcel_variables
MA_DECLARE_PER_LWP(marcel_task_t *, previous_thread);

#section marcel_inline
#depend "asm-generic/marcel_ctx_types.h[marcel_types]"
static __tbx_setjmp_inline__
marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next)
{
	MA_BUG_ON(!ma_in_atomic());
	if (cur != next) {
		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			MA_THR_RESTARTED(cur, "Switch_to");
			MA_BUG_ON(!ma_in_atomic());
			return __ma_get_lwp_var(previous_thread);
		}
		debug_printf(&MA_DEBUG_VAR_NAME(default),
			     "switchto(%p, %p) on LWP(%d)\n",
		       cur, next, LWP_NUMBER(GET_LWP(cur)));
		__ma_get_lwp_var(previous_thread)=cur;
		MA_THR_LONGJMP(cur->number, (next), NORMAL_RETURN);
	}
	return cur;
}

