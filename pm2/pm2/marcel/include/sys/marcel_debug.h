
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





