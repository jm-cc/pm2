
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


#include "marcel.h"

#ifdef PM2DEBUG

debug_type_t marcel_debug=
  NEW_DEBUG_TYPE_DEPEND("MA: ", "ma", NULL);
debug_type_t marcel_mdebug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-mdebug", &marcel_debug);
debug_type_t marcel_debug_state=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-state", &marcel_debug);
debug_type_t marcel_debug_work=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-work", &marcel_debug);
debug_type_t marcel_debug_deviate=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-deviate", &marcel_debug);
debug_type_t marcel_debug_upcall=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-debug-upcall", &marcel_debug);
debug_type_t marcel_mdebug_sched_q=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-mdebug-sched-q", &marcel_debug);

#ifdef DEBUG_LOCK_TASK
debug_type_t marcel_lock_task_debug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-locktask", &marcel_debug);
#endif

#ifdef DEBUG_SCHED_LOCK
debug_type_t marcel_sched_lock_debug=
  NEW_DEBUG_TYPE_DEPEND("MAR: ", "mar-schedlock", &marcel_debug);
#endif

#ifdef MARCEL_TRACE
debug_type_t marcel_mtrace=
  NEW_DEBUG_TYPE_DEPEND("MAR_TRACE: ", "mar-trace", &marcel_debug);
debug_type_t marcel_mtrace_timer=
  NEW_DEBUG_TYPE_DEPEND("MAR_TRACE: ", "mar-trace-timer", &marcel_debug);
#endif

#endif //PM2DEBUG

void marcel_debug_init(int* argc, char** argv, int debug_flags)
{
	static int called=0;
	if (called) 
		return;
	called=1;
	pm2debug_register(&marcel_mdebug);
	pm2debug_register(&marcel_debug_state);
	pm2debug_register(&marcel_debug_work);
	pm2debug_register(&marcel_debug_deviate);
	pm2debug_register(&marcel_mdebug_sched_q);
	pm2debug_register(&marcel_debug_upcall);

#ifdef DEBUG_LOCK_TASK
	pm2debug_register(&marcel_lock_task_debug);
	pm2debug_setup(&marcel_lock_task_debug, PM2DEBUG_SHOW_FILE, 1);
#endif

#ifdef DEBUG_SCHED_LOCK
	pm2debug_register(&marcel_sched_lock_debug);
	pm2debug_setup(&marcel_sched_lock_debug, PM2DEBUG_SHOW_FILE, 1);
#endif

#ifdef MARCEL_TRACE
	pm2debug_register(&marcel_mtrace);
	pm2debug_register(&marcel_mtrace_timer);
#endif
	pm2debug_init_ext(argc, argv, debug_flags);
}

debug_type_t ma_dummy1 __attribute__((section(".ma.debug.size.0"))) =  NEW_DEBUG_TYPE("dummy", "dummy");
debug_type_t ma_dummy2 __attribute__((section(".ma.debug.size.1"))) =  NEW_DEBUG_TYPE("dummy", "dummy");
extern debug_type_t __ma_debug_pre_start[];
extern debug_type_t __ma_debug_start[];
extern debug_type_t __ma_debug_end[];

void __init marcel_debug_init_auto(void)
{
	debug_type_t *var;
	unsigned long __ma_debug_size_entry=(void*)&ma_dummy2-(void*)&ma_dummy1;

	if (__ma_debug_size_entry != sizeof(debug_type_t)) {
		pm2debug("Warning : entry_size %li differ from sizeof %i\n"
			 "Someone to correct the linker script (that does extra alignments) ?\n",
			 __ma_debug_size_entry, sizeof(debug_type_t));
		for(var=__ma_debug_start; var < __ma_debug_end; 
		    var=(debug_type_t *)((void*)(var)+__ma_debug_size_entry)) {
			pm2debug_register(var);
		}
	} else {
		for(var=__ma_debug_start; var < __ma_debug_end; var++) {
			pm2debug_register(var);
		}
	}
}

__ma_initfunc_prio(marcel_debug_init_auto, MA_INIT_DEBUG, MA_INIT_DEBUG_PRIO, 
	      "Register debug variables");


