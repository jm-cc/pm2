
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


#include "sys/marcel_debug.h"

#define NULL ((void*)0)

#ifdef PM2DEBUG

debug_type_t marcel_mdebug=NEW_DEBUG_TYPE(0, "MAR: ", "mar-mdebug");
debug_type_t marcel_trymdebug=NEW_DEBUG_TYPE(0, "MAR: ", "mar-tick");
debug_type_t marcel_debug_state=NEW_DEBUG_TYPE(0, "MAR: ", "mar-state");
debug_type_t marcel_debug_work=NEW_DEBUG_TYPE(0, "MAR: ", "mar-work");
debug_type_t marcel_debug_deviate=NEW_DEBUG_TYPE(0, "MAR: ", "mar-deviate");
debug_type_t marcel_mdebug_sched_q=NEW_DEBUG_TYPE(0, "MAR: ", "mar-mdebug-sched-q");

#ifdef DEBUG_LOCK_TASK
debug_type_t marcel_lock_task_debug=NEW_DEBUG_TYPE(0, "MAR: ", "mar-locktask");
#else
debug_type_t marcel_lock_task_debug=NEW_DEBUG_TYPE(1, "MAR: ", "mar-locktask");
#endif

#ifdef DEBUG_SCHED_LOCK
debug_type_t marcel_sched_lock_debug=NEW_DEBUG_TYPE(0, "MAR: ", 
						    "mar-schedlock");
#else
debug_type_t marcel_sched_lock_debug=NEW_DEBUG_TYPE(1, "MAR: ", 
						    "mar-schedlock");
#endif

#ifdef MARCEL_TRACE
debug_type_t marcel_mtrace=NEW_DEBUG_TYPE(0, "MAR_TRACE: ", "mar-trace");
debug_type_t marcel_mtrace_timer=NEW_DEBUG_TYPE(0, "MAR_TRACE: ", 
						"mar-trace-timer");
#else
debug_type_t marcel_mtrace=NEW_DEBUG_TYPE(1, "MAR_TRACE: ", "mar-trace");
debug_type_t marcel_mtrace_timer=NEW_DEBUG_TYPE(1, "MAR_TRACE: ", 
						"mar-trace-timer");
#endif

#endif //PM2DEBUG

void marcel_debug_init(int* argc, char** argv, int debug_flags)
{
	static int called=0;
	if (called) 
		return;
	called=1;
#ifdef PM2DEBUG
	pm2debug_register(&marcel_mdebug);
	pm2debug_register(&marcel_trymdebug);
	pm2debug_register(&marcel_debug_state);
	pm2debug_setup(&marcel_trymdebug, PM2DEBUG_TRYONLY, 1);
	pm2debug_register(&marcel_debug_work);
	pm2debug_register(&marcel_debug_deviate);
	pm2debug_register(&marcel_mdebug_sched_q);

	pm2debug_register(&marcel_lock_task_debug);
	pm2debug_setup(&marcel_lock_task_debug, PM2DEBUG_SHOW_FILE, 1);

	pm2debug_register(&marcel_sched_lock_debug);
	pm2debug_setup(&marcel_sched_lock_debug, PM2DEBUG_SHOW_FILE, 1);

	pm2debug_register(&marcel_mtrace);
	pm2debug_register(&marcel_mtrace_timer);
#endif
	pm2debug_init_ext(argc, argv, debug_flags);
}


