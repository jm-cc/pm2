
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

#section marcel_types
typedef struct marcel_lwp marcel_lwp_t;
typedef struct marcel_lwp *ma_lwp_t;

#section marcel_structures
#depend "pm2_list.h"
#depend "marcel_sched_generic.h[marcel_structures]"
#depend "marcel_sem.h[structures]"
#depend "sys/marcel_kthread.h[marcel_types]"

struct marcel_lwp {
	struct list_head lwp_list;
#ifdef MA__ACTIVATION
	act_proc_info_t act_infos;
#endif
#ifdef MA__SMP
	marcel_sem_t kthread_stop;
	marcel_kthread_t pid;
#endif
};

#section marcel_variables
#depend "linux_perlwp.h[marcel_macros]"
#ifdef MA__LWPS
MA_DECLARE_PER_LWP(unsigned, number);
#endif
#depend "marcel_descr.h[types]"
MA_DECLARE_PER_LWP(marcel_task_t *, run_task);

#section marcel_macros

#define MARCEL_LWP_EST_DEF
#ifdef MA__LWPS
#  ifdef MA__ACTIVATION
#    include <asm/act.h>
#    ifndef ACT_NB_MAX_CPU
#      error Perhaps, you should add CFLAGS=-I/lib/modules/`uname -r`/build/include
#      undef MA__ACTIVATION
#    endif
#    undef MAX_LWP
#    define MAX_LWP ACT_NB_MAX_CPU
#  else
#    define MA_NR_LWPS 32
#  endif
#else
#  define MA_NR_LWPS 1
#endif

#section marcel_variables
#ifdef MA__LWPS
extern marcel_lwp_t* addr_lwp[MA_NR_LWPS];
#endif

extern marcel_lwp_t __main_lwp;

#ifdef MA__LWPS

// Verrou protégeant la liste chaînée des LWPs
extern ma_rwlock_t __lwp_list_lock;
extern struct list_head list_lwp_head;

extern unsigned  ma__nb_lwp;
//extern marcel_lwp_t* addr_lwp[MA__MAX_LWPS];

#endif

#section marcel_functions
static __inline__ void lwp_list_lock_read(void);
#section marcel_inline
static __inline__ void lwp_list_lock_read(void)
{
#ifdef MA__LWPS
  ma_read_lock(&__lwp_list_lock);
#endif
}

#section marcel_functions
static __inline__ void lwp_list_unlock_read(void);
#section marcel_inline
static __inline__ void lwp_list_unlock_read(void)
{
#ifdef MA__LWPS
  ma_read_unlock(&__lwp_list_lock);
#endif
}

#section marcel_functions
static __inline__ void lwp_list_lock_write(void);
#section marcel_inline
static __inline__ void lwp_list_lock_write(void)
{
#ifdef MA__LWPS
  ma_write_lock(&__lwp_list_lock);
#endif
}

#section marcel_functions
static __inline__ void lwp_list_unlock_write(void);
#section marcel_inline
static __inline__ void lwp_list_unlock_write(void)
{
#ifdef MA__LWPS
  ma_write_unlock(&__lwp_list_lock);
#endif
}

#section functions
static __inline__ unsigned get_nb_lwps();
unsigned marcel_nbvps(void);
#section inline
#depend "[marcel_variables]"
static __inline__ unsigned get_nb_lwps()
{
#ifdef MA__LWPS
  return ma__nb_lwp;
#else
  return 1;
#endif
}

#section functions
static __inline__ unsigned marcel_get_nb_lwps_np();
#section inline
static __inline__ unsigned marcel_get_nb_lwps_np()
{
   return get_nb_lwps();
}

#section marcel_functions
static __inline__ void set_nb_lwps(unsigned value);
#section marcel_inline
static __inline__ void set_nb_lwps(unsigned value)
{
#ifdef MA__LWPS
  ma__nb_lwp=value;
#endif
}

#section marcel_functions
inline static marcel_lwp_t* marcel_lwp_next_lwp(marcel_lwp_t* lwp);
#section marcel_inline
inline static marcel_lwp_t* marcel_lwp_next_lwp(marcel_lwp_t* lwp)
{
  return list_entry(lwp->lwp_list.next, marcel_lwp_t, lwp_list);
}

#section marcel_functions
inline static marcel_lwp_t* marcel_lwp_prev_lwp(marcel_lwp_t* lwp);
#section marcel_inline
inline static marcel_lwp_t* marcel_lwp_prev_lwp(marcel_lwp_t* lwp)
{
  return list_entry(lwp->lwp_list.prev, marcel_lwp_t, lwp_list);
}

#section marcel_functions
/****************************************************************
 * Départ
 */
#if defined(MA__LWPS)
void marcel_lwp_fix_nb_vps(unsigned nb_lwp);
#endif

#ifdef MA__SMP
unsigned marcel_lwp_add_vp(void);
void marcel_lwp_stop_lwp(marcel_lwp_t *lwp);
#endif

#section marcel_macros
/****************************************************************
 * Accès aux LWP
 */

#define GET_LWP_NUMBER(current)             (LWP_NUMBER(THREAD_GETMEM(current,sched.lwp)))
#ifdef MA__LWPS

#define LWP_NUMBER(lwp)                     (ma_per_lwp(number, lwp))
#define GET_LWP_BY_NUM(proc)                (addr_lwp[proc])
#define GET_LWP(current)                    ((current)->sched.lwp)
#define SET_LWP(current, value)             ((current)->sched.lwp=(value))
#define GET_CUR_LWP()                       (cur_lwp)
#define SET_CUR_LWP(value)                  (cur_lwp=(value))
#define SET_LWP_NB(number, lwp)             (addr_lwp[number]=(lwp), \
	                                     LWP_NUMBER(lwp)=number)
#define GET_LWP_BY_NUMBER(number)           (addr_lwp[number])
#define DEFINE_CUR_LWP(OPTIONS, signe, lwp) \
   OPTIONS marcel_lwp_t *cur_lwp signe lwp
#define IS_FIRST_LWP(lwp)                   (LWP_NUMBER(lwp)==0)
#define for_all_lwp(lwp) \
   list_for_each_entry(lwp, &list_lwp_head, lwp_list)
#define for_each_lwp_begin(lwp) \
   list_for_each_entry(lwp, &list_lwp_head, lwp_list) {\
      if (ma_lwp_online(lwp)) {
#define for_each_lwp_end() \
      } \
   }

#define lwp_isset(num, map) ma_test_bit(num, &map)
#define lwp_is_offline(lwp) 0

#else

#define cur_lwp                             (&__main_lwp)
#define LWP_NUMBER(lwp)                     ((void)(lwp),0)
#define GET_LWP_BY_NUM(nb)                  (cur_lwp)
#define GET_LWP(current)                    (cur_lwp)
#define SET_LWP(current, value)             ((void)0)
#define GET_CUR_LWP()                       (cur_lwp)
#define SET_CUR_LWP(value)                  ((void)0)
#define SET_LWP_NB(proc, value)             ((void)0)
#define GET_LWP_BY_NUMBER(number)           (cur_lwp)
#define DEFINE_CUR_LWP(OPTIONS, signe, current) \
   int __cur_lwp_unused__ __attribute__ ((unused))
#define IS_FIRST_LWP(lwp)                   (1)

#define for_all_lwp(lwp) lwp=cur_lwp;
#define for_each_lwp_begin(lwp) for_all_lwp(lwp) {
#define for_each_lwp_end() }

#endif

#define LWP_GETMEM(lwp, member) ((lwp)->member)
#define LWP_SELF                (GET_LWP(MARCEL_SELF))
#define ma_softirq_pending(lwp) \
   (ma_per_lwp(softirq_pending,(lwp)))
#define ma_local_softirq_pending() \
   (ma_softirq_pending(LWP_SELF))

#section marcel_variables
#depend "linux_perlwp.h[marcel_macros]"
MA_DECLARE_PER_LWP(unsigned long, softirq_pending);

#section marcel_functions
static inline int ma_lwp_online(ma_lwp_t lwp);
#ifdef MA__LWPS
/* Need to know about LWPs going up/down? */
extern int ma_register_lwp_notifier(struct ma_notifier_block *nb);
extern void ma_unregister_lwp_notifier(struct ma_notifier_block *nb);

#else
static inline int ma_register_lwp_notifier(struct ma_notifier_block *nb);
static inline void ma_unregister_lwp_notifier(struct ma_notifier_block *nb);
#endif /* MA__LWPS */
#section marcel_inline
#ifndef MA__LWPS
static inline int ma_register_lwp_notifier(struct ma_notifier_block *nb)
{
        return 0;
}
static inline void ma_unregister_lwp_notifier(struct ma_notifier_block *nb)
{
}
#else /* MA__LWPS */
MA_DECLARE_PER_LWP(int, online);
#endif /* MA__LWPS */

static inline int ma_lwp_online(ma_lwp_t lwp)
{
#ifdef MA__LWPS
	return ma_per_lwp(online,lwp);
#else
	return 1;
#endif
}

