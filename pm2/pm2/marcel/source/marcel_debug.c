/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: marcel_debug.c,v $
Revision 1.2  2000/05/25 00:23:53  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.1  2000/05/09 10:52:45  vdanjean
pm2debug module


______________________________________________________________________________
*/


#include "sys/marcel_debug.h"
#define NULL ((void*)0)

#ifdef PM2DEBUG
#ifdef MARCEL_DEBUG
debug_type_t marcel_mdebug=NEW_DEBUG_TYPE(0, "MAR: ", "mar-mdebug");
debug_type_t marcel_trymdebug=NEW_DEBUG_TYPE(0, "MAR: ", "mar-tick");
debug_type_t marcel_debug_state=NEW_DEBUG_TYPE(0, "MAR: ", "mar-state");
debug_type_t marcel_debug_work=NEW_DEBUG_TYPE(0, "MAR: ", "mar-work");
debug_type_t marcel_debug_deviate=NEW_DEBUG_TYPE(0, "MAR: ", "mar-deviate");
#else
debug_type_t marcel_mdebug=NEW_DEBUG_TYPE(1, "MAR: ", "mar-mdebug");
debug_type_t marcel_trymdebug=NEW_DEBUG_TYPE(1, "MAR: ", "mar-tick");
debug_type_t marcel_debug_state=NEW_DEBUG_TYPE(1, "MAR: ", "mar-state");
debug_type_t marcel_debug_work=NEW_DEBUG_TYPE(1, "MAR: ", "mar-work");
debug_type_t marcel_debug_deviate=NEW_DEBUG_TYPE(1, "MAR: ", "mar-deviate");
#endif

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
#endif

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
	pm2debug_setup(&marcel_trymdebug, DEBUG_TRYONLY, 1);
	pm2debug_register(&marcel_debug_work);
	pm2debug_register(&marcel_debug_deviate);

	pm2debug_register(&marcel_lock_task_debug);
	pm2debug_setup(&marcel_lock_task_debug, DEBUG_SHOW_FILE, 1);

	pm2debug_register(&marcel_sched_lock_debug);
	pm2debug_setup(&marcel_sched_lock_debug, DEBUG_SHOW_FILE, 1);

	pm2debug_register(&marcel_mtrace);
	pm2debug_register(&marcel_mtrace_timer);
#endif
	pm2debug_init_ext(argc, argv, debug_flags);
}


