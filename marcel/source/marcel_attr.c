
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
#include <stdint.h>
#include <inttypes.h>

/* Déclaré non statique car utilisé dans marcel.c : */
marcel_attr_t marcel_attr_default = MARCEL_ATTR_INITIALIZER;
marcel_attr_t marcel_attr_destroyer = MARCEL_ATTR_DESTROYER;

/* Déclaré dans marcel.c : */
extern volatile unsigned default_stack;

DEF_MARCEL_POSIX(int, attr_init, (marcel_attr_t *attr), (attr),
{
   *attr = marcel_attr_default;
   return 0;
})

#ifdef MA__LIBPTHREAD

   int __pthread_attr_init_2_1 (pthread_attr_t *__attr)
{
int ret;
marcel_attr_t attr;
ret=marcel_attr_init(&attr);
memcpy(__attr, &attr, sizeof(pthread_attr_t));   // changement d'ABI, on adapte taille struct
return ret;
}
versioned_symbol (libpthread, __pthread_attr_init_2_1, pthread_attr_init, GLIBC_2_1);
#endif

/**********************attr_set/getstacksize***********************/
DEF_MARCEL_POSIX(int, attr_setstacksize, (marcel_attr_t *attr, size_t stack), (attr, stack),
{
   if ((stack < sizeof(marcel_task_t) + TLS_AREA_SIZE) || (stack > THREAD_SLOT_SIZE))
   {
      errno = EINVAL;
      return 1;
   }
   attr->__stacksize = stack;
   return 0;
})

DEF_PTHREAD(int, attr_setstacksize, (pthread_attr_t *attr, size_t stack), (attr, stack))
DEF___PTHREAD(int, attr_setstacksize, (pthread_attr_t *attr, size_t stack), (attr, stack))

DEF_MARCEL_POSIX(int, attr_getstacksize, (__const marcel_attr_t * __restrict attr, size_t * __restrict stack), (attr, stack),
   {
      *stack = attr->__stacksize;
      return 0;
   })
   
DEF_PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))
DEF___PTHREAD(int, attr_getstacksize, (__const pthread_attr_t *attr, size_t *stack), (attr, stack))

/**********************attr_set/getstackaddr***********************/
DEF_MARCEL_POSIX(int, attr_setstackaddr, (marcel_attr_t *attr, void *addr), (attr, addr),
{
   if ((uintptr_t)addr %THREAD_SLOT_SIZE != 0)
   {
      errno = EINVAL;
      return -1;
   }
   attr->__stackaddr_set = 1;
   attr->__stackaddr = addr;
   return 0;
})

DEF_PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))
DEF___PTHREAD(int, attr_setstackaddr, (pthread_attr_t *attr, void *addr), (attr, addr))

DEF_MARCEL_POSIX(int, attr_getstackaddr, (__const marcel_attr_t * __restrict attr, void ** __restrict addr), (attr, addr),
{
   *addr = attr->__stackaddr;
   return 0;
})

DEF_PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))
DEF___PTHREAD(int, attr_getstackaddr, (__const pthread_attr_t *attr, void **addr), (attr, addr))

/****************************get/setstack*************************/

DEF_MARCEL_POSIX(int, attr_setstack, (marcel_attr_t *attr, void * stackaddr, size_t stacksize), 
                    (attr, stackaddr, stacksize),
{
   if ((stacksize < sizeof(marcel_task_t) + TLS_AREA_SIZE) || (stacksize > THREAD_SLOT_SIZE))
   {
      errno = EINVAL;
      return 1;
   }
   if ((uintptr_t)stackaddr %THREAD_SLOT_SIZE != 0) 
   {
      errno = EINVAL;
      return -1;
   }
   attr->__stacksize = stacksize;
   attr->__stackaddr_set = 1;
   attr->__stackaddr = stackaddr;
   return 0;
})
   
DEF_PTHREAD(int, attr_setstack, (pthread_attr_t *attr, void* stackaddr, size_t stacksize), 
               (attr, stackaddr, stacksize))
DEF___PTHREAD(int, attr_setstack, (pthread_attr_t *attr, void* stackaddr, size_t stacksize), 
                 (attr, stackaddr, stacksize))

DEF_MARCEL_POSIX(int, attr_getstack, (__const marcel_attr_t * __restrict attr, 
      void* *__restrict stackaddr, size_t * __restrict stacksize), 
      (attr, stackaddr, stacksize),
{
   *stacksize = attr->__stacksize;
   *stackaddr = attr->__stackaddr;
   return 0;
})

DEF_PTHREAD(int, attr_getstack, (__const pthread_attr_t *attr, void* *stackaddr, size_t *stacksize), 
   (attr, stackaddr, stacksize))
DEF___PTHREAD(int, attr_getstack, (__const pthread_attr_t *attr, void* *stackaddr, size_t *stacksize), 
   (attr, stackaddr, stacksize))

/********************attr_set/getdetachstate**********************/

DEF_MARCEL_POSIX(int, attr_setdetachstate, (marcel_attr_t *attr, int detached), (attr, detached),
{
   attr->__detachstate = detached;
   return 0;
})

DEF_PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, int detached), (attr, detached))
DEF___PTHREAD(int, attr_setdetachstate, (pthread_attr_t *attr, int detached), (attr, detached))

DEF_MARCEL_POSIX(int, attr_getdetachstate, (__const marcel_attr_t *attr, int *detached), (attr, detached),
{
   *detached = attr->__detachstate;
   return 0;
})

DEF_PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, int *detached), (attr, detached))
DEF___PTHREAD(int, attr_getdetachstate, (pthread_attr_t *attr, int *detached), (attr, detached))

#ifdef MA__LIBPTHREAD
 /********************attr_set/getguardsize**********************/
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize)
{
   return marcel_attr_setguardsize(attr, guardsize);
}

int pthread_attr_getguardsize(__const pthread_attr_t * __restrict attr,
                              size_t * __restrict guardsize)
{
   return marcel_attr_getguardsize(attr, guardsize);
}
#endif

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space)
{
   attr->user_space = space;
   return 0;
}

int marcel_attr_getuserspace(__const marcel_attr_t * __restrict attr,
                             unsigned * __restrict space)
{
   *space = attr->user_space;
   return 0;
}

int marcel_attr_setactivation(marcel_attr_t *attr, tbx_bool_t immediate)
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

int marcel_attr_setmigrationstate(marcel_attr_t *attr, tbx_bool_t migratable)
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

int marcel_attr_setdeviationstate(marcel_attr_t *attr, tbx_bool_t deviatable)
{
   attr->not_deviatable = (deviatable ? 0 : 1);
   return 0;
}

int marcel_attr_getdeviationstate(__const marcel_attr_t * __restrict attr,
                                  tbx_bool_t *__restrict deviatable)
{
   *deviatable = (attr->not_deviatable ? tbx_false : tbx_true);
   return 0;
}

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy)
{
   attr->__schedpolicy = policy;
   return 0;
}

int marcel_attr_getschedpolicy(__const marcel_attr_t * __restrict attr,
                               int * __restrict policy)
{
   *policy = attr->__schedpolicy;
   return 0;
}

int marcel_attr_setrealtime(marcel_attr_t *attr, tbx_bool_t realtime)
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

int marcel_attr_setvpmask(marcel_attr_t *attr, marcel_vpmask_t mask)
{
   attr->vpmask = mask;
   return 0;
}

int marcel_attr_getvpmask(__const marcel_attr_t * __restrict attr,
                          marcel_vpmask_t * __restrict mask)
{
   *mask = attr->vpmask;
   return 0;
}

static void setname(char * __restrict dest, const char * __restrict src)
{
   strncpy(dest,src,MARCEL_MAXNAMESIZE-1);
   dest[MARCEL_MAXNAMESIZE-1]='\0';
}

static void getname(char * __restrict dest, const char * __restrict src, size_t n)
{
   strncpy(dest, src, n);
   if (MARCEL_MAXNAMESIZE>n)
      dest[n-1]='0';
}

int marcel_attr_setname(marcel_attr_t * __restrict attr,
                        const char * __restrict name)
{
   setname(attr->name, name);
   return 0;
}

int marcel_attr_getname(__const marcel_attr_t * __restrict attr,
                        char * __restrict name, size_t n)
{
   getname(name, attr->name, n);
   return 0;
}

int marcel_setname(marcel_t __restrict pid, const char * __restrict name)
{
   setname(pid->name, name);
   PROF_SET_THREAD_NAME(pid);
   return 0;
}

int marcel_getname(marcel_t __restrict pid, char * __restrict name, size_t n)
{
   getname(name, pid->name, n);
   return 0;
}

int marcel_attr_setid(marcel_attr_t * attr, int id)
{
   attr->id = id;
   return 0;
}

int marcel_attr_getid(__const marcel_attr_t * __restrict attr, int * __restrict id)
{
   *id = attr->id;
   return 0;
}

int marcel_attr_setpreemptible(marcel_attr_t *attr, int preemptible)
{
   attr->not_preemptible = preemptible?0:1;
   return 0;
}

int marcel_attr_getpreemptible(__const marcel_attr_t * __restrict attr,
                               int * __restrict preemptible)
{
   *preemptible = !attr->not_preemptible;
   return 0;
}

int marcel_attr_setflags(marcel_attr_t *attr, int flags)
{
   attr->flags = flags;
   return 0;
}

int marcel_attr_getflags(__const marcel_attr_t * __restrict attr,
                         int * __restrict flags)
{
   *flags = attr->flags;
   return 0;
}

/****************************destroy******************************/
#undef marcel_attr_destroy
DEF_MARCEL_POSIX(int,attr_destroy,(marcel_attr_t * attr),(attr),
{
   *attr = marcel_attr_destroyer;
   return 0;
})

#ifdef MA__LIBPTHREAD
int lpt_attr_destroy(lpt_attr_t * attr)
{
 memcpy(attr,&marcel_attr_destroyer,sizeof(lpt_attr_t)); 
 return 0;
}

DEF_LIBPTHREAD(int,attr_destroy,(pthread_attr_t * attr),(attr))
DEF___LIBPTHREAD(int,attr_destroy,(pthread_attr_t * attr),(attr))

#endif

/***********************get/setinheritsched***********************/
DEF_POSIX(int,attr_setinheritsched,(marcel_attr_t * attr,int inheritsched),(attr,inheritsched),
{
#ifdef MA__DEBUG
   if ((inheritsched != PTHREAD_INHERIT_SCHED)&&(inheritsched != PTHREAD_EXPLICIT_SCHED))
   {
      return EINVAL;
   }
#endif
   if (inheritsched == PTHREAD_INHERIT_SCHED)
   {
      attr->__inheritsched = -1;
      return ENOTSUP;
   }
   attr->__inheritsched = PTHREAD_EXPLICIT_SCHED;
   return 0;
})

DEF_PTHREAD(int,attr_setinheritsched,(pthread_attr_t *__restrict attr, int *inheritshed),(attr,inheritsched))
DEF___PTHREAD(int,attr_setinheritsched,(pthread_attr_t *__restrict attr, int *inheritshed),(attr,inheritsched))
   
DEF_POSIX(int,attr_getinheritsched,(__const pmarcel_attr_t * attr,int * inheritsched),(attr,inheritsched),
{
#ifdef MA__DEBUG
   if (attr->__inheritsched == -1)
   {
      *inheritsched = -1;
      return EINVAL;  
   }
#endif
   *inheritsched = PTHREAD_EXPLICIT_SCHED;
   return 0;
})

DEF_PTHREAD(int,attr_getinheritsched,(pthread_attr_t *__restrict attr, int *inheritshed),(attr,inheritsched))
DEF___PTHREAD(int,attr_getinheritsched,(pthread_attr_t *__restrict attr, int *inheritshed),(attr,inheritsched))

/***************************get/setscope**************************/
DEF_POSIX(int,attr_setscope,(pmarcel_attr_t *__restrict attr, int contentionscope),(attr,contentionscope),
{
#ifdef MA__DEBUG
   if ((contentionscope != PTHREAD_SCOPE_SYSTEM)
       &&(contentionscope != PTHREAD_SCOPE_PROCESS))
   {
      return EINVAL;
   }
#endif
   if (contentionscope == PTHREAD_SCOPE_SYSTEM)
   {
      attr->__scope = -1;
      return ENOTSUP;
   }
   attr->__scope = PTHREAD_SCOPE_PROCESS;
   return 0;
})

DEF_PTHREAD(int,attr_setscope,(pthread_attr_t *__restrict attr, int contentionscope),(attr,contentionscope))
DEF___PTHREAD(int,attr_setscope,(pthread_attr_t *__restrict attr, int contentionscope),(attr,contentionscope))

DEF_POSIX(int,attr_getscope,(__const pmarcel_attr_t *__restrict attr, int *contentionscope),(attr,contentionscope),
{
#ifdef MA__DEBUG
   if (attr->__scope == -1)
   {
      *contentionscope = -1;
      return EINVAL;  
   } 
#endif
   *contentionscope = PTHREAD_SCOPE_PROCESS;
   return 0;
})

DEF_PTHREAD(int,attr_getscope,(pthread_attr_t *__restrict attr, int *contentionscope),(attr,contentionscope))
DEF___PTHREAD(int,attr_getscope,(pthread_attr_t *__restrict attr, int *contentionscope),(attr,contentionscope))

/************************get/setschedpolicy***********************/
DEF_POSIX(int,attr_setschedpolicy,(pmarcel_attr_t *__restrict attr, int policy),(attr,policy),
   {
#ifdef MA__DEBUG
      if ((policy != SCHED_FIFO)&&(policy != SCHED_RR)
        &&(policy != SCHED_OTHER))
      {
         return EINVAL;
      }
#endif
      if (policy != SCHED_RR)
      {
         attr->__schedpolicy = -1;
         return ENOTSUP;
      }
      attr->__schedpolicy = SCHED_RR;
      return 0;
   })

DEF_PTHREAD(int,attr_setschedpolicy,(pthread_attr_t *__restrict attr, int policy),(attr,policy))
DEF___PTHREAD(int,attr_setschedpolicy,(pthread_attr_t *__restrict attr, int policy),(attr,policy))

DEF_POSIX(int,attr_getschedpolicy,(__const pmarcel_attr_t *__restrict attr, int *policy),(attr,policy),
{
#ifdef MA__DEBUG
   if (attr->__schedpolicy == -1)
   {
      *policy = -1;
      return EINVAL;  
   } 
#endif
   *policy = SCHED_RR;
   return 0;
})

DEF_PTHREAD(int,attr_getschedpolicy,(__const pthread_attr_t *__restrict attr, int *policy),(attr,policy))
DEF___PTHREAD(int,attr_getschedpolicy,(__const pthread_attr_t *__restrict attr, int *policy),(attr,policy))

/*************************get/setschedparam***********************/
DEF_MARCEL_POSIX(int,attr_setschedparam,(marcel_attr_t *attr, __const struct marcel_sched_param *param),(attr,param),
{
#ifdef MA__DEBUG
   if (param == NULL)   
   {
      return EINVAL;
   }
   if (attr->__schedpolicy != SCHED_RR)
      return -1;
#endif
   attr->__schedparam.__sched_priority = param->__sched_priority;
   return 0;
})
   
DEF_PTHREAD(int,attr_setschedparam,(pthread_attr_t *__restrict attr, const struct sched_param param),(attr,param))
DEF___PTHREAD(int,attr_setschedparam,(pthread_attr_t *__restrict attr, const struct sched_param param),(attr,param))
 
DEF_MARCEL_POSIX(int,attr_getschedparam,(__const marcel_attr_t *__restrict attr, struct marcel_sched_param *param),(attr,param),
{
#ifdef MA__DEBUG
   if (attr->__schedpolicy != SCHED_RR)
   {   
      errno = ENOTSUP;
      return -1;
   }
#endif
      param->__sched_priority = attr->__schedparam.__sched_priority;
      return 0;
   })

DEF_PTHREAD(int,attr_getschedparam,(__const pthread_attr_t *__restrict attr, struct sched_param *param),(attr,param))
DEF___PTHREAD(int,attr_getschedparam,(__const pthread_attr_t *__restrict attr, struct sched_param *param),(attr,param))

   
   
   
   
   
   
