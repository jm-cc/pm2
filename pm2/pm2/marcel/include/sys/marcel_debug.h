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
$Log: marcel_debug.h,v $
Revision 1.8  2000/05/25 00:23:50  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.7  2000/05/16 09:05:21  rnamyst
Fast Polling added into Marcel + make xconfig

Revision 1.6  2000/05/10 13:08:01  vdanjean
minor bugs fixes

Revision 1.5  2000/05/09 10:52:43  vdanjean
pm2debug module

Revision 1.4  2000/04/28 18:33:36  vdanjean
debug actsmp + marcel_key

Revision 1.2  2000/04/11 09:07:17  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/03/31 09:04:12  rnamyst
This is the old "debug.h" file.

______________________________________________________________________________
*/

#ifndef MARCEL_DEBUG_EST_DEF
#define MARCEL_DEBUG_EST_DEF

#include "pm2debug.h"
#include "sys/marcel_flags.h"



#undef mdebug

#define LWPS_HACK
#if defined(MA__LWPS) && (1)
#define LWPS_FM "[P%02d]"
#define LWPS_VAL ,((pm2debug_marcel_launched && (marcel_self())->lwp) ? (marcel_self())->lwp->number : -1)
#else
#define LWPS_FM
#define LWPS_VAL
#endif

#ifdef PM2DEBUG
extern debug_type_t marcel_mdebug;
extern debug_type_t marcel_trymdebug;
extern debug_type_t marcel_debug_state;
extern debug_type_t marcel_debug_work;
extern debug_type_t marcel_debug_deviate;

extern debug_type_t marcel_lock_task_debug;

extern debug_type_t marcel_sched_lock_debug;

extern debug_type_t marcel_mtrace;
extern debug_type_t marcel_mtrace_timer;
#endif

#ifdef MARCEL_DEBUG
#define mdebug(fmt, args...) \
    debug_printf(&marcel_mdebug, LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#define try_mdebug(fmt, args...) \
    debug_printf(&marcel_trymdebug, LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#define mdebug_state(fmt, args...) \
    debug_printf(&marcel_debug_state, LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#define mdebug_work(fmt, args...) \
    debug_printf(&marcel_debug_work, LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#define mdebug_deviate(fmt, args...) \
    debug_printf(&marcel_debug_deviate, LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#else
#define mdebug(fmt, args...)     (void)0
#define try_mdebug(fmt, args...)     (void)0
#define mdebug_state(fmt, args...)     (void)0
#define mdebug_work(fmt, args...)     (void)0
#define mdebug_deviate(fmt, args...)     (void)0
#endif

#ifdef DEBUG_LOCK_TASK
#define lock_task_debug(fmt, args...) debug_printf(&marcel_lock_task_debug, \
        LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#endif
#ifdef DEBUG_SCHED_LOCK
#define sched_lock_debug(fmt, args...) debug_printf(&marcel_sched_lock_debug, \
        LWPS_FM fmt LWPS_VAL LWPS_HACK, ##args)
#endif

#ifdef MARCEL_TRACE

#define MTRACE(msg, pid) \
    (msg[0] ? debug_printf(&marcel_mtrace, \
            "[P%02d][%-11s:%3ld (pid=%p:%X)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            ((pid)->lwp ? (pid)->lwp->number : -1), \
            msg, (pid)->number, (pid), (pid)->special_flags, \
            marcel_activethreads(), \
            marcel_sleepingthreads(), \
            marcel_blockedthreads(), \
            marcel_frozenthreads(), \
            marcel_nbthreads() + 1) : (void)0)
#define MTRACE_TIMER(msg, pid) \
    debug_printf(&marcel_mtrace_timer, \
            "[P%02d][%-11s:%3ld (pid=%p:%X)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            ((pid)->lwp ? (pid)->lwp->number : -1), \
            msg, (pid)->number, (pid), (pid)->special_flags, \
            marcel_activethreads(), \
            marcel_sleepingthreads(), \
            marcel_blockedthreads(), \
            marcel_frozenthreads(), \
            marcel_nbthreads() + 1)
#define marcel_trace_on() pm2debug_setup(&marcel_mtrace, DEBUG_SHOW, 1)
#define marcel_trace_off() pm2debug_setup(&marcel_mtrace, DEBUG_SHOW, 0)

#else

#define MTRACE(msg, pid) (void)0
#define MTRACE_TIMER(msg, pid) (void)0
#define marcel_trace_on() (void)0
#define marcel_trace_off() (void)0

#endif

void marcel_debug_init(int* argc, char** argv, int debug_flags);

#endif





