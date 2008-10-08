
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

#section types
#include <sys/types.h> /* pour size_t */
typedef struct __marcel_attr_s marcel_attr_t, pmarcel_attr_t;

#section structures

#depend "marcel_sched_generic.h[types]"
#depend "marcel_topology.h[marcel_structures]"
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_sched.h[types]"

/* Attributes for threads.  */
struct __marcel_attr_s {
	/* begin of pthread, don't modify */
	/* Scheduler parameters and priority.  */
	struct marcel_sched_param __schedparam;
	int __schedpolicy;
	/* Various flags like detachstate, scope, etc. */
	int __flags;
#define MA_ATTR_FLAG_DETACHSTATE	0x0001
#define MA_ATTR_FLAG_INHERITSCHED	0x0002
#define MA_ATTR_FLAG_SCOPESYSTEM	0x0004
	/* Size of guard area.  */
	size_t __guardsize;
	/* Stack handling.  */
	void *__stackaddr;
	size_t __stacksize;
	/* Affinity map.  */
	marcel_vpset_t *__cpuset;
	size_t __cpusetsize; /* Unused */

	/* marcel attributes */
	/* unsigned stack_size; */
	/* char *stack_base; */
	/*int tbx_bool_t detached; */
#ifdef MARCEL_USERSPACE_ENABLED
	unsigned user_space;
	/*tbx_bool_t */ int immediate_activation;
#endif /* MARCEL_USERSPACE_ENABLED */
#ifdef MARCEL_MIGRATION_ENABLED
	unsigned not_migratable;
#endif /* MARCEL_MIGRATION_ENABLED */
	unsigned not_deviatable;
	int not_preemptible;
	/* int sched_policy; */
	/*tbx_bool_t int rt_thread; On utilise la priorit� maintenant */
	/* TODO: option de flavor */
	marcel_vpset_t vpset;
	marcel_topo_level_t *topo_level;
	int flags;
	char name[MARCEL_MAXNAMESIZE];
	int id;
	marcel_sched_attr_t sched;
	int seed;
};

#section macros

/*  MARCEL_CREATE_JOINABLE */
#depend "marcel_threads.h[types]"
/*  MARCEL_SCHED_OTHER */
/*  MARCEL_SCHED_ATTR_INITIALIZER */
#define MARCEL_SCHED_INTERNAL_INCLUDE
#depend "scheduler/marcel_sched.h[macros]"
/*  MARCEL_VPMASK_EMPTY */
#depend "marcel_sched_generic.h[macros]"

#ifdef PM2STACKSGUARD
#define MARCEL_STACKSGUARD THREAD_SLOT_SIZE
#else
#define MARCEL_STACKSGUARD 0
#endif

#ifdef MARCEL_USERSPACE_ENABLED
#  define MARCEL_ATTR_USERSPACE_INITIALIZER \
	.user_space= 0, \
        .immediate_activation= tbx_false,
#else /* MARCEL_USERSPACE_ENABLED */
#  define MARCEL_ATTR_USERSPACE_INITIALIZER
#endif /* MARCEL_USERSPACE_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED
#  define MARCEL_ATTR_MIGRATION_INITIALIZER \
        .not_migratable= 1,
#else /* MARCEL_MIGRATION_ENABLED */
#  define MARCEL_ATTR_MIGRATION_INITIALIZER
#endif /* MARCEL_MIGRATION_ENABLED */

#define MARCEL_ATTR_INITIALIZER { \
  .__schedparam= {MA_DEF_PRIO,}, \
  .__schedpolicy= SCHED_OTHER, \
  .__flags = 0, \
  .__guardsize= MARCEL_STACKSGUARD, \
  .__stackaddr= NULL, \
  .__stacksize= (THREAD_SLOT_SIZE - sizeof(struct marcel_task)),	\
  .__cpuset = NULL, \
  .__cpusetsize = 0, \
  MARCEL_ATTR_USERSPACE_INITIALIZER \
  MARCEL_ATTR_MIGRATION_INITIALIZER \
  .not_deviatable= 0, \
  .not_preemptible= 0, \
  .vpset= MARCEL_VPSET_FULL, \
  .flags= 0, \
  .name= "user_task", \
  .id = -1, \
  .sched= MARCEL_SCHED_ATTR_INITIALIZER, \
  .seed = 0, \
}

/* #define MARCEL_SCHED_ATTR_DESTROYER { \ */
/* 	.init_holder = NULL, \ /\* à voir *\/ */
/* 	.prio = -1, \ */
/* 	.inheritholder = tbx_false, \ /\* à voir *\/ */
/* } */

#ifdef MARCEL_USERSPACE_ENABLED
#  define MARCEL_ATTR_USERSPACE_DESTROYER \
	.user_space= -1, \
        .immediate_activation= tbx_false,
#else /* MARCEL_USERSPACE_ENABLED */
#  define MARCEL_ATTR_USERSPACE_DESTROYER
#endif /* MARCEL_USERSPACE_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED
#  define MARCEL_ATTR_MIGRATION_DESTROYER \
        .not_migratable= -1,
#else /* MARCEL_MIGRATION_ENABLED */
#  define MARCEL_ATTR_MIGRATION_DESTROYER
#endif /* MARCEL_MIGRATION_ENABLED */

#define MARCEL_ATTR_DESTROYER { \
  .__schedparam= {-1,}, \
  .__schedpolicy= MARCEL_SCHED_INVALID, \
  .__flags = -1, \
  .__guardsize= -1, \
  .__stackaddr= NULL, \
  .__stacksize= -1, \
  .__cpuset = NULL, \
  .__cpusetsize = -1, \
  MARCEL_ATTR_USERSPACE_DESTROYER \
  MARCEL_ATTR_MIGRATION_DESTROYER \
  .not_deviatable= -1, \
  .not_preemptible= -1, \
  .vpset= MARCEL_VPSET_ZERO, \
  .flags= -1, \
  .name= "invalid", \
  .id = -2, \
  .seed = -1, \
}

/* realtime */
#define MARCEL_CLASS_REGULAR      tbx_false
#define MARCEL_CLASS_REALTIME     tbx_true

#define PMARCEL_STACK_MIN sizeof(marcel_task_t)

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

#ifdef MARCEL_USERSPACE_ENABLED
int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space);
int marcel_attr_getuserspace(__const marcel_attr_t * __restrict attr,
                             unsigned * __restrict space);

int marcel_attr_setactivation(marcel_attr_t *attr, tbx_bool_t immediate);
int marcel_attr_getactivation(__const marcel_attr_t * __restrict attr,
                              tbx_bool_t * __restrict immediate);
#endif /* MARCEL_USERSPACE_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED
int marcel_attr_setmigrationstate(marcel_attr_t *attr, tbx_bool_t migratable);
int marcel_attr_getmigrationstate(__const marcel_attr_t * __restrict attr,
                                  tbx_bool_t * __restrict migratable);
#endif /* MARCEL_MIGRATION_ENABLED */

int marcel_attr_setdeviationstate(marcel_attr_t *attr, tbx_bool_t deviatable);
int marcel_attr_getdeviationstate(__const marcel_attr_t * __restrict attr,
                                  tbx_bool_t * __restrict deviatable);

int marcel_attr_setprio(marcel_attr_t *attr, int prio);
int marcel_attr_getprio(__const marcel_attr_t * __restrict attr,
                            int * __restrict prio);

int marcel_attr_setrealtime(marcel_attr_t *attr, tbx_bool_t realtime);
int marcel_attr_getrealtime(__const marcel_attr_t * __restrict attr,
                            tbx_bool_t * __restrict realtime);

/* TODO: pouvoir directement donner un niveau de topologie */
int marcel_attr_setvpset(marcel_attr_t *attr, marcel_vpset_t vpset);
int marcel_attr_getvpset(__const marcel_attr_t * __restrict attr,
                          marcel_vpset_t * __restrict vpset);

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

/*  only for internal use */
int marcel_attr_setflags(marcel_attr_t *attr, int flags);
int marcel_attr_getflags(__const marcel_attr_t * __restrict attr,
                         int * __restrict flags);

int marcel_attr_setseed(marcel_attr_t *attr, int seed);
int marcel_attr_getseed(__const marcel_attr_t * __restrict attr,
			 int * __restrict seed);
#ifdef PM2STACKSGUARD
#define marcel_attr_setguardsize(attr, guardsize) ((guardsize)>THREAD_SLOT_SIZE)
#define marcel_attr_getguardsize(attr, guardsize) (*(guardsize) = THREAD_SLOT_SIZE, 0)
#else
#define marcel_attr_setguardsize(attr, guardsize) (!!(guardsize))
#define marcel_attr_getguardsize(attr, guardsize) (*(guardsize) = 0, 0)
#endif

DEC_MARCEL_POSIX(int, attr_setinheritsched, (marcel_attr_t * attr,int inheritsched) __THROW);
DEC_MARCEL_POSIX(int, attr_getinheritsched, (marcel_attr_t * attr,int * inheritsched) __THROW);
DEC_MARCEL_POSIX(int, attr_setscope, (pmarcel_attr_t *__restrict attr, int contentionscope) __THROW);
DEC_MARCEL_POSIX(int, attr_getscope, (__const pmarcel_attr_t *__restrict attr, int *contentionscope) __THROW);
DEC_POSIX(int, attr_setschedpolicy, (pmarcel_attr_t *__restrict attr, int policy) __THROW);
DEC_POSIX(int, attr_getschedpolicy, (__const pmarcel_attr_t *__restrict attr, int *policy) __THROW);
DEC_MARCEL(int, attr_setschedparam, (marcel_attr_t *attr, __const struct marcel_sched_param *param) __THROW);
DEC_MARCEL(int, attr_getschedparam, (__const marcel_attr_t *__restrict attr, struct marcel_sched_param *param) __THROW);
DEC_MARCEL_POSIX(int,getattr_np,(marcel_t thread,marcel_attr_t *attr) __THROW);

int marcel_attr_settopo_level(marcel_attr_t * attr, marcel_topo_level_t *topo_level);
int marcel_attr_gettopo_level(__const marcel_attr_t * __restrict attr, marcel_topo_level_t ** __restrict topo_level);

#section marcel_variables
extern marcel_attr_t marcel_attr_default;
extern marcel_attr_t marcel_attr_destroyer;
