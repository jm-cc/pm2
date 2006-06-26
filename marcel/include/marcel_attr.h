
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

#include <stdlib.h>

#section types
typedef struct __marcel_attr_s marcel_attr_t, pmarcel_attr_t;

#section structures

#include <sys/types.h> /* pour size_t */
#depend "marcel_sched_generic.h[types]"
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_sched.h[types]"

/* Attributes for threads.  */
struct __marcel_attr_s
{
      /* begin of pthread */
      int __detachstate;
      int __schedpolicy;
      struct marcel_sched_param __schedparam;
      int __inheritsched;
      int __scope;
      size_t __guardsize;
      int __stackaddr_set;
      void *__stackaddr;
      size_t __stacksize;
      /* end of pthread */

      /* marcel attributes */
      //unsigned stack_size;
      //char *stack_base;
      //int /*tbx_bool_t*/ detached;
      unsigned user_space;
      /*tbx_bool_t*/int immediate_activation;
      unsigned not_migratable;
      unsigned not_deviatable;
      int not_preemptible;
      //int sched_policy;
      /*tbx_bool_t int rt_thread; On utilise la priorit� maintenant */
      marcel_vpmask_t vpmask;
      int flags;
      char name[MARCEL_MAXNAMESIZE];
      int id;
      marcel_sched_attr_t sched;

};

#define lpt_attr_t pmarcel_attr_t

#section macros
/**********************************/
#define PTHREAD_STACK_MIN 16384
/**********************************/

// MARCEL_CREATE_JOINABLE
#depend "marcel_threads.h[types]"
// MARCEL_SCHED_OTHER
// MARCEL_SCHED_ATTR_INITIALIZER
#define MARCEL_SCHED_INTERNAL_INCLUDE
#depend "scheduler/marcel_sched.h[marcel_macros]"
// MARCEL_VPMASK_EMPTY
#depend "marcel_sched_generic.h[macros]"

#ifdef PM2STACKSGUARD
#define MARCEL_STACKSGUARD THREAD_SLOT_SIZE
#else
#define MARCEL_STACKSGUARD 0
#endif
#define MARCEL_ATTR_INITIALIZER { \
  .__detachstate= MARCEL_CREATE_JOINABLE, \
  .__schedpolicy= MARCEL_SCHED_OTHER, \
  .__schedparam= {0,}, \
  .__inheritsched= 0, \
  .__scope= 0, \
  .__guardsize= MARCEL_STACKSGUARD, \
  .__stackaddr_set= 0, \
  .__stackaddr= NULL, \
  .__stacksize= THREAD_SLOT_SIZE, \
  .user_space= 0, \
  .immediate_activation= tbx_false, \
  .not_migratable= 1, \
  .not_deviatable= 0, \
  .not_preemptible= 0, \
  .vpmask= MARCEL_VPMASK_EMPTY, \
  .flags= 0, \
  .name= "user_task", \
  .id = -1, \
  .sched= MARCEL_SCHED_ATTR_INITIALIZER, \
}

/* valeurs invalides */
#define MARCEL_SCHED_INVALID -1
#define MARCEL_SCHED_PARAM_INVALID {-1,}
#define MARCEL_SCOPE_INVALID -1
#define MARCEL_STACKSGUARD_INVALID -1
#define MARCEL_STACKADDR_SET_INVALID -1
#define MARCEL_STACKADDR_INVALID NULL
#define THREAD_SLOT_SIZE_INVALID -1
#define USER_SPACE_INVALID -1
#define MARCEL_MIGRATABLE_INVALID -1
#define MARCEL_DEVIATABLE_INVALID -1
#define MARCEL_PREEMPTIBLE_INVALID -1
#define MARCEL_VPMASK_INVALID -1
#define FLAGS_INVALID -1
#define MARCEL_ATTR_ID_INVALID -2
/* #define MARCEL_SCHED_ATTR_DESTROYER { \ */
/* 	.init_holder = NULL, \ /\* à voir *\/ */
/* 	.prio = -1, \ */
/* 	.inheritholder = tbx_false, \ /\* à voir *\/ */
/* } */

#define MARCEL_ATTR_DESTROYER { \
  .__detachstate= MARCEL_CREATE_DETACHSTATE_INVALID, \
  .__schedpolicy= MARCEL_SCHED_INVALID, \
  .__schedparam= MARCEL_SCHED_PARAM_INVALID, \
  .__inheritsched= MARCEL_INHERITSCHED_INVALID, \
  .__scope= MARCEL_SCOPE_INVALID, \
  .__guardsize= MARCEL_STACKSGUARD_INVALID, \
  .__stackaddr_set= MARCEL_STACKADDR_SET_INVALID, \
  .__stackaddr= MARCEL_STACKADDR_INVALID, \
  .__stacksize= THREAD_SLOT_SIZE_INVALID, \
  .user_space= USER_SPACE_INVALID, \
  .immediate_activation= tbx_false, \
  .not_migratable= MARCEL_MIGRATABLE_INVALID, \
  .not_deviatable= MARCEL_DEVIATABLE_INVALID, \
  .not_preemptible= MARCEL_PREEMPTIBLE_INVALID, \
  .vpmask= MARCEL_VPMASK_INVALID, \
  .flags= FLAGS_INVALID, \
  .name= "invalid", \
  .id = MARCEL_ATTR_ID_INVALID, \
}

/* realtime */
#define MARCEL_CLASS_REGULAR      tbx_false
#define MARCEL_CLASS_REALTIME     tbx_true

#section functions
#depend "marcel_utils.h[types]"

DEC_MARCEL_POSIX(int, attr_init, (marcel_attr_t *attr) __THROW);
DEC_MARCEL_POSIX(int, attr_destroy, (marcel_attr_t *attr) __THROW);
#define marcel_attr_destroy(attr_ptr)	0
#define pmarcel_attr_destroy(attr_ptr)	marcel_attr_destroy(attr_ptr)

DEC_MARCEL_POSIX(int, attr_setstacksize, (marcel_attr_t *attr, size_t stack) __THROW);
DEC_MARCEL_POSIX(int, attr_getstacksize, (__const marcel_attr_t * __restrict attr, size_t * __restrict stack) __THROW);
DEC_MARCEL_POSIX(int, attr_setstackaddr, (marcel_attr_t *attr, void *addr) __THROW);
DEC_MARCEL_POSIX(int, attr_getstackaddr, (__const marcel_attr_t * __restrict attr, void ** __restrict addr) __THROW);
DEC_MARCEL_POSIX(int, attr_setstack, (marcel_attr_t *attr, void * stackaddr, size_t stacksize) __THROW);
DEC_MARCEL_POSIX(int, attr_getstack, (__const marcel_attr_t * __restrict attr,
                                      void* *__restrict stackaddr, size_t * __restrict stacksize) __THROW);
DEC_MARCEL_POSIX(int, attr_setdetachstate, (marcel_attr_t *attr, int detached) __THROW);
DEC_MARCEL_POSIX(int, attr_getdetachstate, (__const marcel_attr_t *attr, int *detached) __THROW);

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space);
int marcel_attr_getuserspace(__const marcel_attr_t * __restrict attr,
                             unsigned * __restrict space);

int marcel_attr_setactivation(marcel_attr_t *attr, tbx_bool_t immediate);
int marcel_attr_getactivation(__const marcel_attr_t * __restrict attr,
                              tbx_bool_t * __restrict immediate);

int marcel_attr_setmigrationstate(marcel_attr_t *attr, tbx_bool_t migratable);
int marcel_attr_getmigrationstate(__const marcel_attr_t * __restrict attr,
                                  tbx_bool_t * __restrict migratable);

int marcel_attr_setdeviationstate(marcel_attr_t *attr, tbx_bool_t deviatable);
int marcel_attr_getdeviationstate(__const marcel_attr_t * __restrict attr,
                                  tbx_bool_t * __restrict deviatable);

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy) __THROW;
int marcel_attr_getschedpolicy(__const marcel_attr_t * __restrict attr,
                               int * __restrict policy) __THROW;

int marcel_attr_setrealtime(marcel_attr_t *attr, tbx_bool_t realtime);
int marcel_attr_getrealtime(__const marcel_attr_t * __restrict attr,
                            tbx_bool_t * __restrict realtime);

int marcel_attr_setvpmask(marcel_attr_t *attr, marcel_vpmask_t mask);
int marcel_attr_getvpmask(__const marcel_attr_t * __restrict attr,
                          marcel_vpmask_t * __restrict mask);

int marcel_attr_setname(marcel_attr_t * __restrict attr,
                        const char * __restrict name);
int marcel_attr_getname(__const marcel_attr_t * __restrict attr,
                        char * __restrict name, size_t n);

int marcel_attr_setid(marcel_attr_t * attr, int id);
int marcel_attr_getid(__const marcel_attr_t * __restrict attr,
                      int * __restrict id);

int marcel_attr_setpreemptible(marcel_attr_t *attr, int preemptible);
int marcel_attr_getpreemptible(__const marcel_attr_t * __restrict attr,
                               int * __restrict preemptible);

// only for internal use
int marcel_attr_setflags(marcel_attr_t *attr, int flags);
int marcel_attr_getflags(__const marcel_attr_t * __restrict attr,
                         int * __restrict flags);

#ifdef MA__LIBPTHREAD
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
int pthread_attr_getguardsize(__const pthread_attr_t * __restrict attr,
                              size_t * __restrict guardsize);
#endif

#ifdef PM2STACKSGUARD
#define marcel_attr_setguardsize(attr, guardsize) ((guardsize)>THREAD_SLOT_SIZE)
#define marcel_attr_getguardsize(attr, guardsize) (*(guardsize) = THREAD_SLOT_SIZE, 0)
#else
#define marcel_attr_setguardsize(attr, guardsize) (!!(guardsize))
#define marcel_attr_getguardsize(attr, guardsize) (*(guardsize) = 0, 0)
#endif
#define pmarcel_attr_setguardsize(attr, guardsize) marcel_attr_setguardsize(attr, guardsize)
#define pmarcel_attr_getguardsize(attr, guardsize) marcel_attr_getguardsize(attr, guardsize)

DEC_MARCEL_POSIX(int, attr_setinheritsched, (marcel_attr_t * attr,int inheritsched) __THROW);
DEC_MARCEL_POSIX(int, attr_getinheritsched, (marcel_attr_t * attr,int * inheritsched) __THROW);
DEC_POSIX(int, attr_setscope, (pmarcel_attr_t *__restrict attr, int contentionscope) __THROW);
DEC_POSIX(int, attr_getscope, (__const pmarcel_attr_t *__restrict attr, int *contentionscope) __THROW);
DEC_POSIX(int, attr_setschedpolicy, (pmarcel_attr_t *__restrict attr, int policy) __THROW);
DEC_POSIX(int, attr_getschedpolicy, (__const pmarcel_attr_t *__restrict attr, int *policy) __THROW);
DEC_MARCEL_POSIX(int, attr_setschedparam, (marcel_attr_t *attr, __const struct marcel_sched_param *param) __THROW);
DEC_MARCEL_POSIX(int, attr_getschedparam, (__const marcel_attr_t *__restrict attr, struct marcel_sched_param *param) __THROW);

#section marcel_variables
extern marcel_attr_t marcel_attr_default;
extern marcel_attr_t marcel_attr_destroyer;
