
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

#include <string.h>
#include <errno.h>
#ifdef __MINGW32__
#define SCHED_RR (-1)
#else
#include <sched.h>
#endif
#ifndef SOLARIS_SYS
#include <stdint.h>
#endif
#include <inttypes.h>

marcel_attr_t marcel_attr_default = MARCEL_ATTR_INITIALIZER;
marcel_attr_t marcel_attr_destroyer = MARCEL_ATTR_DESTROYER;

DEF_MARCEL_POSIX(int, attr_init, (marcel_attr_t *attr), (attr),
{
	LOG_IN();
	*attr = marcel_attr_default;
	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_attr_init (lpt_attr_t *__attr)
{
	LOG_IN();
	memcpy(__attr, &marcel_attr_default, sizeof(pthread_attr_t));	// changement d'ABI, on adapte taille struct
	LOG_RETURN(0);
}
versioned_symbol (libpthread, lpt_attr_init,
                  pthread_attr_init, GLIBC_2_1);
#endif

/**********************attr_set/getstacksize***********************/
DEF_MARCEL_POSIX(int, attr_setstacksize, (marcel_attr_t *attr, size_t stack), (attr, stack),
{
	LOG_IN();
	if ((stack < sizeof(marcel_task_t)) || (stack > THREAD_SLOT_SIZE)) {
		mdebug("(p)marcel_attr_setstacksize : stack size error: "
					 "requested %u B but `THREAD_SLOT_SIZE' is only %lu B\n",
					 (unsigned int) stack, THREAD_SLOT_SIZE);
		LOG_RETURN(EINVAL);
	}

	if (attr->__stackaddr)
		attr->__stackaddr = (char *) attr->__stackaddr + stack - attr->__stacksize;

	attr->__stacksize = stack;
	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, __pmarcel_attr_setstacksize, pthread_attr_setstacksize, GLIBC_2_3_3);

#if MA_GLIBC_VERSION_MINIMUM < 20303
strong_alias(__pmarcel_attr_setstacksize, __old_lpt_attr_setstacksize)
compat_symbol(libpthread, __old_lpt_attr_setstacksize, pthread_attr_setstacksize, GLIBC_2_1);
#endif
#endif

DEF_MARCEL_POSIX(int, attr_getstacksize, (__const marcel_attr_t * __restrict attr, size_t * __restrict stack), (attr, stack),
{
	LOG_IN();
	*stack = attr->__stacksize;
	LOG_RETURN(0);
})
   
DEF_PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))
DEF___PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))

/**********************attr_set/getstackaddr***********************/
DEF_MARCEL(int, attr_setstackaddr, (marcel_attr_t *attr, void *addr), (attr, addr),
{
	LOG_IN();
	/* addr est le bas de la pile */
	attr->__stackaddr = (char *) addr + attr->__stacksize;
	LOG_RETURN(0);
})

DEF_POSIX(int, attr_setstackaddr, (marcel_attr_t *attr, void *addr), (attr, addr),
{
	LOG_IN();
	/* addr est le haut de la pile */
	attr->__stackaddr = addr;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))
DEF___PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))

DEF_POSIX(int, attr_getstackaddr, (__const marcel_attr_t * __restrict attr, void ** __restrict addr), (attr, addr),
{
	LOG_IN();
	*addr = attr->__stackaddr;
	LOG_RETURN(0);
})
DEF_PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))
DEF___PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))

/****************************get/setstack*************************/

DEF_POSIX(int, attr_setstack, (marcel_attr_t *attr, void * stackaddr, size_t stacksize), 
                    (attr, stackaddr, stacksize),
{
	LOG_IN();
	if ((stacksize < PMARCEL_STACK_MIN) || (stacksize > THREAD_SLOT_SIZE)) {
		mdebug("pthread_attr_setstack : taille de pile invalide !!\n");
		LOG_RETURN(EINVAL);
	}
	if ((uintptr_t) stackaddr % THREAD_SLOT_SIZE != 0) {
		mdebug("pthread_attr_setstack : pile non alignee !!\n");
		LOG_RETURN(EINVAL);
	}
	/* stackaddr est le bas de la pile */
	attr->__stacksize = stacksize;
	attr->__stackaddr = (char *) stackaddr + stacksize;
	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, __pmarcel_attr_setstack, pthread_attr_setstack, GLIBC_2_3_3);

#if MA_GLIBC_VERSION_MINIMUM < 20303
strong_alias(__pmarcel_attr_setstack, __old_lpt_attr_setstack)
compat_symbol(libpthread, __old_lpt_attr_setstack, pthread_attr_setstack, GLIBC_2_2);
#endif
#endif

DEF_POSIX(int, attr_getstack, (__const marcel_attr_t * __restrict attr, 
      void* *__restrict stackaddr, size_t * __restrict stacksize), 
      (attr, stackaddr, stacksize),
{
	LOG_IN();
	/* stackaddr est le bas de la pile */
	*stacksize = attr->__stacksize;
	*stackaddr = (char *) attr->__stackaddr - attr->__stacksize;
	LOG_RETURN(0);
})

DEF_PTHREAD(int, attr_getstack, (__const pthread_attr_t *attr, void* *stackaddr, size_t *stacksize), (attr, stackaddr, stacksize))
DEF___PTHREAD(int, attr_getstack, (__const pthread_attr_t *attr, void* *stackaddr, size_t *stacksize), (attr, stackaddr, stacksize))

/********************attr_set/getdetachstate**********************/
static inline int TBX_UNUSED check_attr_setdetachstate(int detached)
{
	LOG_IN();
	if ((detached != MARCEL_CREATE_DETACHED)
	    && (detached != MARCEL_CREATE_JOINABLE)) {
		mdebug
		    ("(p)marcel_attr_setdetachstate : valeur detached invalide !\n");
		LOG_RETURN(EINVAL);
	}
	LOG_RETURN(0);
}

DEF_MARCEL(int, attr_setdetachstate, (marcel_attr_t *attr, int detached), (attr, detached),
{
	LOG_IN();
#ifdef MA__DEBUG
	int ret = check_attr_setdetachstate(detached);
	if (ret) {
		LOG_RETURN(ret);
	}
#endif
	if (detached)
		attr->__flags |= MA_ATTR_FLAG_DETACHSTATE;
	else
		attr->__flags &= ~MA_ATTR_FLAG_DETACHSTATE;
	LOG_RETURN(0);
})

DEF_POSIX(int, attr_setdetachstate, (marcel_attr_t *attr, int detached), (attr, detached),
{
	int ret;
	LOG_IN();
	/* error checking */
	ret = check_attr_setdetachstate(detached);
	if (ret) {
		LOG_RETURN(ret);
	}
	/* fin error checking */

	if (detached)
		attr->__flags |= MA_ATTR_FLAG_DETACHSTATE;
	else
		attr->__flags &= ~MA_ATTR_FLAG_DETACHSTATE;
	LOG_RETURN(0);
})
DEF_PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, int detached), (attr, detached))
DEF___PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, int detached), (attr, detached))

DEF_MARCEL_POSIX(int, attr_getdetachstate, (__const marcel_attr_t *attr, int *detached), (attr, detached),
{
	LOG_IN();
	*detached = !!(attr->__flags & MA_ATTR_FLAG_DETACHSTATE);
	LOG_RETURN(0);
})

DEF_PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, int *detached), (attr, detached))
DEF___PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, int *detached), (attr, detached))

 /********************attr_set/getguardsize**********************/
DEF_POSIX(int, attr_setguardsize, (marcel_attr_t * attr TBX_UNUSED, size_t guardsize), (attr, guardsize), 
{
	LOG_IN();
	if (marcel_attr_setguardsize(attr, guardsize)) {
		errno = EINVAL;
		LOG_RETURN(-1);
	}
	LOG_RETURN(0);
})

DEF_PTHREAD(int, attr_setguardsize, (pthread_attr_t *attr, size_t guardsize), (attr, guardsize))
DEF___PTHREAD(int, attr_setguardsize, (pthread_attr_t *attr, size_t guardsize), (attr, guardsize))

DEF_POSIX(int, attr_getguardsize, (__const marcel_attr_t * __restrict attr TBX_UNUSED,
    size_t * __restrict guardsize), (attr, guardsize),
{
	LOG_IN();
	LOG_RETURN(marcel_attr_getguardsize(attr, guardsize));
})

DEF_PTHREAD(int, attr_getguardsize, (pthread_attr_t *attr, size_t *guardsize), (attr, guardsize))
DEF___PTHREAD(int, attr_getguardsize, (pthread_attr_t *attr, size_t *guardsize), (attr, guardsize))

#ifdef MARCEL_USERSPACE_ENABLED
int marcel_attr_setuserspace(marcel_attr_t * attr, unsigned space)
{
	attr->user_space = space;
	return 0;
}

int marcel_attr_getuserspace(__const marcel_attr_t * __restrict attr,
    unsigned *__restrict space)
{
	*space = attr->user_space;
	return 0;
}

int marcel_attr_setactivation(marcel_attr_t * attr, tbx_bool_t immediate)
{
	attr->immediate_activation = immediate;
	return 0;
}

int marcel_attr_getactivation(__const marcel_attr_t * __restrict attr,
    tbx_bool_t * __restrict immediate)
{
	*immediate = attr->immediate_activation;
	return 0;
}
#endif /* MARCEL_USERSPACE_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED
int marcel_attr_setmigrationstate(marcel_attr_t * attr, tbx_bool_t migratable)
{
	attr->not_migratable = (migratable ? 0 : 1);
	return 0;
}

int marcel_attr_getmigrationstate(__const marcel_attr_t * __restrict attr,
    tbx_bool_t * __restrict migratable)
{
	*migratable = (attr->not_migratable ? tbx_false : tbx_true);
	return 0;
}
#endif /* MARCEL_MIGRATION_ENABLED */

int marcel_attr_setdeviationstate(marcel_attr_t * attr, tbx_bool_t deviatable)
{
	attr->not_deviatable = (deviatable ? 0 : 1);
	return 0;
}

int marcel_attr_getdeviationstate(__const marcel_attr_t * __restrict attr,
    tbx_bool_t * __restrict deviatable)
{
	*deviatable = (attr->not_deviatable ? tbx_false : tbx_true);
	return 0;
}

int marcel_attr_setprio(marcel_attr_t * attr, int prio)
{
	attr->__schedparam.__sched_priority = prio;
	return 0;
}

int marcel_attr_getprio(__const marcel_attr_t * attr, int *prio)
{
	*prio = attr->__schedparam.__sched_priority;
	return 0;
}

int marcel_attr_setrealtime(marcel_attr_t * attr, tbx_bool_t realtime)
{
	return marcel_attr_setprio(attr, realtime ? MA_RT_PRIO : MA_DEF_PRIO);
}

int marcel_attr_getrealtime(__const marcel_attr_t * __restrict attr,
    tbx_bool_t * __restrict realtime)
{
	int prio;
	marcel_attr_getprio(attr, &prio);
	*realtime = prio < MA_DEF_PRIO;
	return 0;
}

int marcel_attr_setvpset(marcel_attr_t * attr, marcel_vpset_t vpset)
{
	attr->vpset = vpset;
	return 0;
}

DEF_POSIX(int, attr_setaffinity_np, (pmarcel_attr_t *attr, size_t cpusetsize TBX_UNUSED, const pmarcel_cpu_set_t *cpuset), (attr, cpusetsize, cpuset),
{
	MA_BUG_ON(cpusetsize != PMARCEL_CPU_SETSIZE);
	return marcel_attr_setvpset(attr, *cpuset);
})
#ifdef MA__LIBPTHREAD
int lpt_attr_setaffinity_np(lpt_attr_t *__attr, size_t cpusetsize, const cpu_set_t *cpuset)
{
	marcel_attr_t *attr = (marcel_attr_t *) __attr;
	marcel_vpset_t *vpset;
	if (attr->__cpuset)
		vpset = attr->__cpuset;
	else {
		attr->__cpuset = vpset = TBX_MALLOC(sizeof(marcel_vpset_t));
		marcel_vpset_fill(vpset);
	}
	return marcel_cpuset2vpset(cpusetsize, cpuset, vpset);
}
versioned_symbol(libpthread, lpt_attr_setaffinity_np, pthread_attr_setaffinity_np, GLIBC_2_3_4);
#endif

int marcel_attr_getvpset(__const marcel_attr_t * __restrict attr,
    marcel_vpset_t * __restrict vpset)
{
	*vpset = attr->vpset;
	return 0;
}

DEF_POSIX(int, attr_getaffinity_np, (const pmarcel_attr_t *attr, size_t cpusetsize TBX_UNUSED, pmarcel_cpu_set_t *cpuset), (attr, cpusetsize, cpuset),
{
	MA_BUG_ON(cpusetsize != PMARCEL_CPU_SETSIZE);
	return marcel_attr_getvpset(attr, cpuset);
})
#ifdef MA__LIBPTHREAD
int lpt_attr_getaffinity_np(const lpt_attr_t *__attr, size_t cpusetsize, cpu_set_t *cpuset)
{
	marcel_attr_t *attr = (marcel_attr_t *) __attr;
	marcel_vpset_t *vpset;
	static marcel_vpset_t full_vpset = MARCEL_VPSET_FULL;
	if (attr->__cpuset)
		vpset = attr->__cpuset;
	else
		vpset = &full_vpset;
	return marcel_vpset2cpuset(vpset, cpusetsize, cpuset);
}
versioned_symbol(libpthread, lpt_attr_getaffinity_np, pthread_attr_getaffinity_np, GLIBC_2_3_4);
#endif

int marcel_attr_settopo_level(marcel_attr_t * attr, marcel_topo_level_t *topo_level)
{
	attr->topo_level = topo_level;
	return 0;
}

int marcel_attr_gettopo_level(__const marcel_attr_t * __restrict attr,
    marcel_topo_level_t ** __restrict topo_level)
{
	*topo_level = attr->topo_level;
	return 0;
}

static void setname(char *__restrict dest, const char *__restrict src)
{
	strncpy(dest, src, MARCEL_MAXNAMESIZE - 1);
	dest[MARCEL_MAXNAMESIZE - 1] = '\0';
}

static void getname(char *__restrict dest, const char *__restrict src, size_t n)
{
	strncpy(dest, src, n);
	if (MARCEL_MAXNAMESIZE > n)
		dest[n - 1] = '0';
}

int marcel_attr_setname(marcel_attr_t * __restrict attr,
    const char *__restrict name)
{
	setname(attr->name, name);
	return 0;
}

int marcel_attr_getname(__const marcel_attr_t * __restrict attr,
    char *__restrict name, size_t n)
{
	getname(name, attr->name, n);
	return 0;
}

int marcel_setname(marcel_t __restrict pid, const char *__restrict name)
{
	setname(pid->name, name);
	PROF_SET_THREAD_NAME(pid);
	return 0;
}

int marcel_getname(marcel_t __restrict pid, char *__restrict name, size_t n)
{
	getname(name, pid->name, n);
	return 0;
}

int marcel_attr_setid(marcel_attr_t * attr, int id)
{
	attr->id = id;
	return 0;
}

int marcel_attr_getid(__const marcel_attr_t * __restrict attr,
    int *__restrict id)
{
	*id = attr->id;
	return 0;
}

int marcel_attr_setpreemptible(marcel_attr_t * attr, int preemptible)
{
	attr->not_preemptible = !preemptible;
	return 0;
}

int marcel_attr_getpreemptible(__const marcel_attr_t * __restrict attr,
    int *__restrict preemptible)
{
	*preemptible = !attr->not_preemptible;
	return 0;
}

int marcel_attr_setflags(marcel_attr_t * attr, int flags)
{
	attr->flags = flags;
	return 0;
}

int marcel_attr_getflags(__const marcel_attr_t * __restrict attr,
    int *__restrict flags)
{
	*flags = attr->flags;
	return 0;
}

int marcel_attr_setseed(marcel_attr_t *attr, int seed)
{
	attr->seed = seed;
	return 0;
}

int marcel_attr_getseed(__const marcel_attr_t * __restrict attr,
    int * __restrict seed)
{
	*seed = attr->seed;
	return 0;
}
/****************************destroy******************************/
#undef marcel_attr_destroy
DEF_MARCEL_POSIX(int,attr_destroy,(marcel_attr_t * attr),(attr),
{
	LOG_IN();
	*attr = marcel_attr_destroyer;
	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_attr_destroy(lpt_attr_t * __attr)
{
	marcel_attr_t *attr = (marcel_attr_t *) __attr;
	LOG_IN();
	if (attr->__cpuset)
		TBX_FREE(attr->__cpuset);
	memcpy(attr, &marcel_attr_destroyer, sizeof(lpt_attr_t));
	LOG_RETURN(0);
}

DEF_LIBPTHREAD(int,attr_destroy,(pthread_attr_t * attr),(attr))
DEF___LIBPTHREAD(int,attr_destroy,(pthread_attr_t * attr),(attr))

#endif

/***********************get/setinheritsched***********************/
DEF_POSIX(int,attr_setinheritsched,(pmarcel_attr_t * attr,int inheritsched),(attr,inheritsched),
{
	LOG_IN();

	if (!attr) {
		mdebug("pthread_attr_setinheritsched : attr NULL !\n");
		LOG_RETURN(EINVAL);
	}

	if ((inheritsched != PMARCEL_INHERIT_SCHED)
	    && (inheritsched != PMARCEL_EXPLICIT_SCHED)) {
		mdebug
		    ("pmarcel_attr_setinheritsched : valeur inheritsched (%d) invalide\n",
		    inheritsched);
		LOG_RETURN(EINVAL);
	}

	if (inheritsched)
		attr->__flags |= MA_ATTR_FLAG_INHERITSCHED;
	else
		attr->__flags &= ~MA_ATTR_FLAG_INHERITSCHED;
	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_setinheritsched,(pthread_attr_t *attr, int *inheritshed),(attr,inheritsched))
DEF___PTHREAD(int,attr_setinheritsched,(pthread_attr_t *attr, int *inheritshed),(attr,inheritsched))
   
DEF_POSIX(int,attr_getinheritsched,(__const pmarcel_attr_t * __restrict attr,int * __restrict inheritsched),(attr,inheritsched),
{
	LOG_IN();
	if (!attr) {
		mdebug
		    ("pthread_attr_getinheritsched : attr NULL ou adresse policy invalide !\n");
		LOG_RETURN(EINVAL);
	}

	*inheritsched = !!(attr->__flags & MA_ATTR_FLAG_INHERITSCHED);
	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_getinheritsched,(pthread_attr_t *__restrict attr, int *__restrict inheritshed),(attr,inheritsched))
DEF___PTHREAD(int,attr_getinheritsched,(pthread_attr_t *__restrict attr, int *__restrict inheritshed),(attr,inheritsched))

/***************************get/setscope**************************/
DEF_MARCEL_POSIX(int,attr_setscope,(pmarcel_attr_t *attr, int contentionscope),(attr,contentionscope),
{
	LOG_IN();

	/* error checking */
	if (!attr) {
		mdebug("pmarcel_attr_setscope : attr NULL !\n");
		LOG_RETURN(EINVAL);
	}

	if ((contentionscope != MARCEL_SCOPE_SYSTEM)
	    && (contentionscope != MARCEL_SCOPE_PROCESS)) {
		mdebug("pthread_attr_setscope : invalid value scope !\n");
		LOG_RETURN(EINVAL);
	}
	/* error checking end */

	if (contentionscope == MARCEL_SCOPE_SYSTEM)
		attr->__flags |= MA_ATTR_FLAG_SCOPESYSTEM;
	else
		attr->__flags &= ~MA_ATTR_FLAG_SCOPESYSTEM;
	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_setscope,(pthread_attr_t *__restrict attr, int contentionscope),(attr,contentionscope))
DEF___PTHREAD(int,attr_setscope,(pthread_attr_t *__restrict attr, int contentionscope),(attr,contentionscope))

DEF_MARCEL_POSIX(int,attr_getscope,(__const pmarcel_attr_t *__restrict attr, int *contentionscope),(attr,contentionscope),
{
	/* error checking */
	if (!attr) {
		mdebug("pmarcel_attr_setscope : attr NULL !\n");
		LOG_RETURN(EINVAL);
	}
	/* error checking end */

	*contentionscope = attr->__flags & MA_ATTR_FLAG_SCOPESYSTEM ? MARCEL_SCOPE_SYSTEM : MARCEL_SCOPE_PROCESS;
	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_getscope,(pthread_attr_t *__restrict attr, int *contentionscope),(attr,contentionscope))
DEF___PTHREAD(int,attr_getscope,(pthread_attr_t *__restrict attr, int *contentionscope),(attr,contentionscope))

/************************get/setschedpolicy***********************/
DEF_POSIX(int,attr_setschedpolicy,(pmarcel_attr_t *__restrict attr, int policy),(attr,policy),
{
	LOG_IN();

	if (!attr) {
		mdebug("pthread_attr_setschedpolicy : valeur attr NULL !\n");
		errno = EINVAL;
		LOG_RETURN(-1);
	}
	if ((policy != SCHED_FIFO) && (policy != SCHED_RR)
	    && (policy != SCHED_OTHER)) {
		mdebug
		    ("pthread_attr_setschedpolicy : police d'ordonnancement invalide!\n");
		LOG_RETURN(EINVAL);
	}
	attr->__schedpolicy = policy;
	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_setschedpolicy,(pthread_attr_t *__restrict attr, int policy),(attr,policy))
DEF___PTHREAD(int,attr_setschedpolicy,(pthread_attr_t *__restrict attr, int policy),(attr,policy))

DEF_POSIX(int,attr_getschedpolicy,(__const pmarcel_attr_t *__restrict attr, int *policy),(attr,policy),
{
	LOG_IN();
	if (!attr) {
		mdebug("pthread_attr_getschedpolicy : valeur attr NULL !\n");
		errno = EINVAL;
		LOG_RETURN(-1);
	}
	if (!policy) {
		mdebug("pthread_attr_getschedpolicy : valeur policy NULL !\n");
		errno = EINVAL;
		LOG_RETURN(-1);
	}
	*policy = attr->__schedpolicy;
	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_getschedpolicy,(__const pthread_attr_t *__restrict attr, int *policy),(attr,policy))
DEF___PTHREAD(int,attr_getschedpolicy,(__const pthread_attr_t *__restrict attr, int *policy),(attr,policy))

/*************************attr_get/setschedparam***********************/
DEF_MARCEL(int,attr_setschedparam,(marcel_attr_t * __restrict attr, __const struct marcel_sched_param * __restrict param),(attr,param),
{
	LOG_IN();
	if ((param == NULL) || (attr == NULL)) {
		mdebug("(p)marcel_attr_setschedparam : attr ou param NULL\n");
		LOG_RETURN(EINVAL);
	}

	marcel_attr_setprio(attr, param->__sched_priority);

	LOG_RETURN(0);
})

DEF_POSIX(int,attr_setschedparam,(pmarcel_attr_t * __restrict attr, __const struct marcel_sched_param * __restrict param),(attr,param),
{
	int policy;
	int ret;
	LOG_IN();
	if ((param == NULL) || (attr == NULL)) {
		mdebug("pmarcel_attr_setschedparam : attr ou param NULL\n");
		LOG_RETURN(EINVAL);
	}

	ret = pmarcel_attr_getschedpolicy(attr, &policy);

	if (ret == -1) {
		mdebug("et quelle police ?\n");
		LOG_RETURN(-1);
	}

	/* détermination de la priority Marcel en fonction de la priority et policy POSIX */
	if ((policy == SCHED_RR) || (policy == SCHED_FIFO)) {
		if ((param->__sched_priority >= 0)
		    && (param->__sched_priority <= MA_RT_PRIO))
			marcel_attr_setprio(attr,
			    MA_RT_PRIO - param->__sched_priority);
		else {
			mdebug("pmarcel_attr_setschedparam : valeur policy %d invalide\n", param->__sched_priority);
			LOG_RETURN(EINVAL);	//pas sur
		}
	} else if (policy == SCHED_OTHER) {
		if (param->__sched_priority == 0)
			marcel_attr_setprio(attr, MA_DEF_PRIO);
		else {
			mdebug("pmarcel_attr_setschedparam : valeur policy %d invalide\n", param->__sched_priority);
			LOG_RETURN(EINVAL);	//pas sur
		}
	}
	LOG_RETURN(0);
})
   
DEF_PTHREAD(int,attr_setschedparam,(pthread_attr_t *__restrict attr, const struct sched_param *__restrict param),(attr,param))
DEF___PTHREAD(int,attr_setschedparam,(pthread_attr_t *__restrict attr, const struct sched_param *__restrict param),(attr,param))
 
DEF_MARCEL(int,attr_getschedparam,(__const marcel_attr_t *__restrict attr, struct marcel_sched_param *__restrict param),(attr,param),
{
	LOG_IN();
	if ((param == NULL) || (attr == NULL)) {
		mdebug("(p)marcel_attr_getschedparam : attr ou param NULL\n");
		LOG_RETURN(EINVAL);
	}
	marcel_attr_getprio(attr, &param->__sched_priority);
	LOG_RETURN(0);
})

DEF_POSIX(int,attr_getschedparam,(__const pmarcel_attr_t *__restrict attr, struct marcel_sched_param *__restrict param),(attr,param),
{
	int policy;
	int ret;
	LOG_IN();
	if ((param == NULL) || (attr == NULL)) {
		mdebug("(p)marcel_attr_getschedparam : attr ou param NULL\n");
		LOG_RETURN(EINVAL);
	}

	ret = pmarcel_attr_getschedpolicy(attr, &policy);
	if (ret == -1) {
		mdebug
		    ("pmarcel_attr_getschedpolicy retourne une police invalide\n");
		LOG_RETURN(-1);
	}

	if (policy == SCHED_OTHER) {
		param->__sched_priority = 0;
	} else if ((policy == SCHED_RR) || (policy == SCHED_FIFO))
		param->__sched_priority =
		    MA_RT_PRIO - attr->__schedparam.__sched_priority;

	LOG_RETURN(0);
})
DEF_PTHREAD(int,attr_getschedparam,(__const pthread_attr_t *__restrict attr, struct sched_param *__restrict param),(attr,param))
DEF___PTHREAD(int,attr_getschedparam,(__const pthread_attr_t *__restrict attr, struct sched_param *__restrict param),(attr,param))

/*************************getattr_np***********************/
#ifdef MA__IFACE_PMARCEL
int pmarcel_getattr_np(pmarcel_t __t,pmarcel_attr_t *__attr)
{
	marcel_attr_t *attr;
	marcel_t t;
        LOG_IN();
        attr = (marcel_attr_t *)__attr;
        t = (marcel_t) __t;
        marcel_attr_init(__attr);
	if (t->detached)
		attr->__flags |= MA_ATTR_FLAG_DETACHSTATE;
        attr->__stackaddr = t;
        attr->__stacksize = THREAD_SLOT_SIZE - MAL(sizeof(*t));
        LOG_RETURN(0);
}

#ifdef MA__LIBPTHREAD
int lpt_getattr_np(lpt_t t,lpt_attr_t *__attr)
{
	LOG_IN();
	marcel_attr_t *attr = (marcel_attr_t *) __attr;
	lpt_attr_init(__attr);
	if (t->detached)
		attr->__flags |= MA_ATTR_FLAG_DETACHSTATE;
	attr->__stackaddr = t;
	attr->__stacksize = THREAD_SLOT_SIZE - MAL(sizeof(*t));
	LOG_RETURN(0);
}
versioned_symbol (libpthread, lpt_getattr_np,
                  pthread_getattr_np, GLIBC_2_2_3);
#endif
#endif
