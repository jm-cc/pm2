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


#define MARCEL_LOGGER_DECLARE
#include "marcel.h"
#include "tbx_compiler.h"


static tbx_bool_t debug_show_thread = tbx_false;


#ifdef MARCEL_GDB
/*
 * Dumb variables to let gdb scripts know which Marcel options are configured.
 */
int ma_gdb_postexit_enabled =
#ifdef MARCEL_POSTEXIT_ENABLED
    1;
#else
    0;
#endif

/* Dumb array to compensate for gdb's inability for recursion */
struct tbx_fast_list_head *ma_gdb_cur[1024];
marcel_bubble_t *ma_gdb_b[1024];
#endif

void marcel_debug_init()
{
	marcel_debug_show_thread_info(tbx_false);

	MARCEL_LOG_ADD();
	MARCEL_SCHED_LOG_ADD();
	MARCEL_ALLOC_LOG_ADD();
	MARCEL_TIMER_LOG_ADD();
}

void marcel_debug_exit()
{
	MARCEL_LOG_DEL();
	MARCEL_SCHED_LOG_DEL();
	MARCEL_ALLOC_LOG_DEL();
	MARCEL_TIMER_LOG_DEL();

	marcel_debug_show_thread_info(tbx_false);
}

void marcel_debug_show_thread_info(tbx_bool_t show_info)
{
	debug_show_thread = show_info;
}

void marcel_logger_print(tbx_log_t * logd, tbx_msg_level_t level, char *format, ...)
{
	va_list args;

	if (tbx_likely(debug_show_thread)) {
#ifdef MA___LWPS
		tbx_logger_print(logd, level, "[P%02d] ",
				 (MA_LWP_SELF) ? ma_get_task_vpnum(ma_self()) : -1);
#endif

		tbx_logger_print(logd, level, "(%12p:% 3d:%-15s) ", ma_self(),
				 ma_self()->number, ma_self()->as_entity.name);
	}

	va_start(args, format);
	tbx_logger_vprint(logd, level, format, args);
	va_end(args);
}
