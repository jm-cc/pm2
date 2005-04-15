
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

#depend "linux_spinlock.h[macros]"

#section macros
#define MARCEL_MUTEX_INITIALIZER \
  {.__m_reserved=0, .__m_count=0, .__m_owner=0, .__m_kind=MARCEL_MUTEX_TIMED_NP, .__m_lock=MA_FASTLOCK_UNLOCKED}
//#ifdef __USE_GNU
# define MARCEL_RECURSIVE_MUTEX_INITIALIZER_NP \
  {.__m_reserved=0, .__m_count=0, .__m_owner=0, .__m_kind=MARCEL_MUTEX_RECURSIVE_NP, .__m_lock=MA_FASTLOCK_UNLOCKED}
# define MARCEL_ERRORCHECK_MUTEX_INITIALIZER_NP \
  {.__m_reserved=0, .__m_count=0, .__m_owner=0, .__m_kind=MARCEL_MUTEX_ERRORCHECK_NP, .__m_lock=MA_FASTLOCK_UNLOCKED}
# define MARCEL_ADAPTIVE_MUTEX_INITIALIZER_NP \
  {.__m_reserved=0, .__m_count=0, .__m_owner=0, .__m_kind=MARCEL_MUTEX_ADAPTIVE_NP, .__m_lock=MA_FASTLOCK_UNLOCKED}
//#endif


#section types
typedef struct marcel_mutex_s marcel_mutex_t;

/* Once-only execution */
typedef int marcel_once_t;

typedef struct marcel_mutexattr_s marcel_mutexattr_t;

enum
{
  MARCEL_MUTEX_TIMED_NP,
  MARCEL_MUTEX_RECURSIVE_NP,
  MARCEL_MUTEX_ERRORCHECK_NP,
  MARCEL_MUTEX_ADAPTIVE_NP
//#ifdef __USE_UNIX98
  ,
  MARCEL_MUTEX_NORMAL = MARCEL_MUTEX_TIMED_NP,
  MARCEL_MUTEX_RECURSIVE = MARCEL_MUTEX_RECURSIVE_NP,
  MARCEL_MUTEX_ERRORCHECK = MARCEL_MUTEX_ERRORCHECK_NP,
  MARCEL_MUTEX_DEFAULT = MARCEL_MUTEX_NORMAL
//#endif
//#ifdef __USE_GNU
  /* For compatibility.  */
  , MARCEL_MUTEX_FAST_NP = MARCEL_MUTEX_ADAPTIVE_NP
//#endif
};

#section structures
#depend "marcel_threads.h[types]"
/* Mutexes (not abstract because of MARCEL_MUTEX_INITIALIZER).  */
/* (The layout is unnatural to maintain binary compatibility
    with earlier releases of LinuxThreads.) */
struct marcel_mutex_s
{
  int __m_reserved;               /* Reserved for future use */
  int __m_count;                  /* Depth of recursive locking */
  p_marcel_task_t __m_owner;       /* Owner thread (if recursive or errcheck) */
  int __m_kind;                   /* Mutex kind: fast, recursive or errcheck */
  struct _marcel_fastlock __m_lock; /* Underlying fast lock */
};


/* Attribute for mutex.  */
struct marcel_mutexattr_s
{
  int __mutexkind;
};


#section functions
#include <sys/time.h>

DEC_MARCEL(int, mutex_init, (marcel_mutex_t *mutex,
			     __const marcel_mutexattr_t *attr) __THROW)
DEC_MARCEL(int, mutex_destroy, (marcel_mutex_t *mutex) __THROW)

DEC_MARCEL(int, mutex_lock, (marcel_mutex_t *mutex) __THROW)
DEC_MARCEL(int, mutex_trylock, (marcel_mutex_t *mutex) __THROW)
DEC_MARCEL(int, mutex_unlock, (marcel_mutex_t *mutex) __THROW)

DEC_MARCEL(int, mutexattr_init, (marcel_mutexattr_t *attr) __THROW)
DEC_MARCEL(int, mutexattr_destroy, (marcel_mutexattr_t *attr) __THROW)
DEC_MARCEL(int, once, (marcel_once_t * once_control, 
		      void (*init_routine)(void)) __THROW)



DEC_POSIX(int, mutex_init, (pmarcel_mutex_t *mutex,
			    __const pmarcel_mutexattr_t *attr))
DEC_POSIX(int, mutex_destroy, (pmarcel_mutex_t *mutex))

DEC_POSIX(int, mutex_lock, (pmarcel_mutex_t *mutex))
DEC_POSIX(int, mutex_trylock, (pmarcel_mutex_t *mutex))
DEC_POSIX(int, mutex_unlock, (pmarcel_mutex_t *mutex))

DEC_POSIX(int, mutexattr_init, (pmarcel_mutexattr_t *attr))
DEC_POSIX(int, mutexattr_destroy, (pmarcel_mutexattr_t *attr))
DEC_POSIX(int, mutexattr_settype, (pmarcel_mutexattr_t *attr, int kind))
DEC_POSIX(int, mutexattr_setkind_np, (pmarcel_mutexattr_t *attr, int kind))
DEC_POSIX(int, mutexattr_gettype, (const pmarcel_mutexattr_t *attr, int *kind))
DEC_POSIX(int, mutexattr_getkind_np, (const pmarcel_mutexattr_t *attr, int *kind))
DEC_POSIX(int, mutexattr_getpshared, (const pmarcel_mutexattr_t *attr,
				      int *pshared))
DEC_POSIX(int, mutexattr_setpshared, (pmarcel_mutexattr_t *attr, int pshared))

DEC_POSIX(int, once, (pmarcel_once_t * once_control, 
		      void (*init_routine)(void)))
#ifdef MA__PTHREAD_FUNCTIONS
extern void __pthread_once_fork_prepare (void);
extern void __pthread_once_fork_parent (void);
extern void __pthread_once_fork_child (void);
extern void __pmarcel_once_fork_prepare (void);
extern void __pmarcel_once_fork_parent (void);
extern void __pmarcel_once_fork_child (void);
extern void __flockfilelist (void);
extern void __funlockfilelist (void);
extern void __fresetlockfiles (void);
#endif
