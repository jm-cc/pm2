#ifndef THREAD_H
#define THREAD_H

#ifdef PTHREAD

#  include <pthread.h>
#  include <semaphore.h>
#  include <unistd.h>

#  define marcel_mutex_t             pthread_mutex_t
#  define marcel_mutex_init          pthread_mutex_init
#  define marcel_mutex_lock          pthread_mutex_lock
#  define marcel_mutex_unlock        pthread_mutex_unlock

#  define marcel_cond_t              pthread_cond_t
#  define marcel_cond_init           pthread_cond_init
#  define marcel_cond_wait           pthread_cond_wait
#  define marcel_cond_signal         pthread_cond_signal

#  define marcel_sem_t               sem_t
#  define marcel_sem_init(s,v)       sem_init((s),0,(v))
#  define marcel_sem_V               sem_post
#  define marcel_sem_P               sem_wait

#  define marcel_attr_t              pthread_attr_t
#  define marcel_attr_init           pthread_attr_init
#  define marcel_attr_setdetachstate pthread_attr_setdetachstate
#  define MARCEL_CREATE_DETACHED     PTHREAD_CREATE_DETACHED
#  define marcel_attr_setschedpolicy(attr, val)

#  define marcel_t                   pthread_t
#  define marcel_create              pthread_create
#  define marcel_join                pthread_join
#  define marcel_self                pthread_self

#ifdef __linux__
#  define marcel_yield               pthread_yield
#else
#  define marcel_yield               sched_yield
#endif

#  define marcel_main                main

#  define marcel_init(a, b)
#  define marcel_end()

#ifdef _SC_NPROCESSORS_ONLN
#  define marcel_nbvps()             sysconf(_SC_NPROCESSORS_ONLN)
#else
#  define marcel_nbvps()             2
#endif

#  define marcel_printf              printf
#  define marcel_fflush              fflush

#ifdef __linux__
#  define marcel_apply_vpset(set) do { sched_setaffinity(0, sizeof(*set), set); } while(0)
#else
#  define marcel_apply_vpset(set) (void)0
#endif
#  define marcel_vpset_t            unsigned long
#  define MARCEL_VPSET_VP(num)      (1<<(num))

#  define TRUE 1
#  define FALSE 0

typedef   void*any_t;

#elif defined(PTH)

#  include <pth.h>

#  define pth_mutex_lock(m)          pth_mutex_acquire((m), FALSE, NULL)
#  define pth_mutex_unlock(m)        pth_mutex_release(m)

#  define marcel_mutex_t             pth_mutex_t
#  define marcel_mutex_init(m,a)     pth_mutex_init(m)
#  define marcel_mutex_lock(m)       pth_mutex_lock(m)
#  define marcel_mutex_unlock(m)     pth_mutex_unlock(m)

#  define marcel_cond_t              pth_cond_t
#  define marcel_cond_init           pth_cond_init
#  define marcel_cond_wait           pth_cond_wait
#  define marcel_cond_signal         pth_cond_signal

typedef struct {
	pth_mutex_t m;
	pth_cond_t c;
	int v;
} pth_sem_t;

static inline int pth_sem_init(pth_sem_t *sem, int pshared, unsigned int value)
{
	pth_mutex_init(&sem->m);
	pth_cond_init(&sem->c);
	sem->v=value;
	return 0;
}

static inline int pth_sem_wait(pth_sem_t * sem)
{
	pth_mutex_lock(&sem->m);
	
	while(sem->v < 1) {
		pth_cond_await(&sem->c, &sem->m, NULL);
	}
	sem->v--;
	pth_mutex_unlock(&sem->m);

	return 0;
}

static inline int pth_sem_post(pth_sem_t * sem)
{
	pth_mutex_lock(&sem->m);
	sem->v++;

	pth_cond_notify(&sem->c, FALSE);

	pth_mutex_unlock(&sem->m);
	return 0;
}



#  define marcel_sem_t               pth_sem_t
#  define marcel_sem_init(s,v)       pth_sem_init((s),0,(v))
#  define marcel_sem_V               pth_sem_post
#  define marcel_sem_P               pth_sem_wait

#  define marcel_attr_t              pth_attr_t
#  define marcel_attr_init(a)        (*(a)=pth_attr_new())
#  define marcel_attr_setdetachstate(a, v) pth_attr_set(*(a), PTH_ATTR_JOINABLE, (v))
#  define MARCEL_CREATE_DETACHED     FALSE
#  define marcel_attr_setschedpolicy(attr, val)

static inline int pth_create(pth_t  *  thread, 
			     pth_attr_t * attr, void *
			     (*start_routine)(void *), void * arg)
{
	pth_attr_t myattr=PTH_ATTR_DEFAULT;
	
	if (attr!=NULL) myattr=*attr;
	
	*thread=pth_spawn(myattr,start_routine,arg);
	return *thread!=NULL;
}

#  define marcel_t                   pth_t
#  define marcel_create              pth_create
#  define marcel_join                pth_join
#  define marcel_self                pth_self

#  define marcel_yield()             pth_yield(NULL)

#  define marcel_main                main

#  define marcel_init(a, b)          pth_init()
#  define marcel_end()

typedef   void*any_t;

#elif defined(LINUX_CLONE)

#  include <unistd.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <sched.h>
#  include <sys/mman.h>
#  include <signal.h>
#  include <sys/types.h>
#  include <sys/wait.h>

#  define marcel_mutex_t             int
#  define marcel_mutex_init          
#  define marcel_mutex_lock          
#  define marcel_mutex_unlock        

#  define marcel_cond_t              int
#  define marcel_cond_init           
#  define marcel_cond_wait           
#  define marcel_cond_signal         

#  define marcel_attr_t              int
#  define marcel_attr_init           
#  define marcel_attr_setdetachstate 
#  define MARCEL_CREATE_DETACHED     

#  define marcel_t pid_t

#  define STACK_SIZE  (128*1024)
static void *_stack; // ATTENTION !!! Un seul clone à la fois hein !

int clone(int (*fn)(void *), void *child_stack, int flags, void *arg);

#  define marcel_create(pid_ptr, attr, func, arg) \
  do { \
    _stack = malloc(STACK_SIZE); \
    *(pid_ptr) = clone((int(*)(void*))(func), (_stack + STACK_SIZE - 64), \
		   CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | \
		   SIGCHLD, \
		   (arg)); \
  } while(0)

#  define marcel_join(pid, status_ptr) \
  do { \
    waitpid(pid, NULL, 0); \
    free(_stack); \
  } while(0)

#  define marcel_yield               sched_yield

#  define marcel_init(a, b)
#  define marcel_end()

#  define marcel_main                main

#ifdef _SC_NPROCESSORS_ONLN
#  define marcel_nbvps()             sysconf(_SC_NPROCESSORS_ONLN)
#else
#  define marcel_nbvps()             2
#endif

#  define marcel_printf              printf
#  define marcel_fflush              fflush

#ifdef __linux__
#  define marcel_apply_vpset(set) do { sched_setaffinity(0, sizeof(*set), set); } while(0)
#else
#  define marcel_apply_vpset(set) (void)0
#endif
#  define marcel_vpset_t            unsigned long
#  define MARCEL_VPSET_VP(num)      (1<<(num))

#elif defined(REGULAR_UNIX)

#  include <sys/types.h>
#  include <sys/wait.h>
#  include <sched.h>

#  define marcel_mutex_t             int
#  define marcel_mutex_init          
#  define marcel_mutex_lock          
#  define marcel_mutex_unlock        

#  define marcel_cond_t              int
#  define marcel_cond_init           
#  define marcel_cond_wait           
#  define marcel_cond_signal         

#  define marcel_attr_t              int
#  define marcel_attr_init           
#  define marcel_attr_setdetachstate 
#  define MARCEL_CREATE_DETACHED     

#  define marcel_t pid_t

#  define marcel_create(pid_ptr, attr, func, arg) \
  do { \
    *(pid_ptr) = fork(); \
    if(!*(pid_ptr)) { \
      (func)((arg)); \
      exit(0); \
    } \
  } while(0)

#  define marcel_join(pid, status_ptr) \
  do { \
    waitpid(pid, NULL, 0); \
  } while(0)

#  define marcel_yield               sched_yield

#  define marcel_init(a, b)
#  define marcel_end()

#  define marcel_main                main

#ifdef _SC_NPROCESSORS_ONLN
#  define marcel_nbvps()             sysconf(_SC_NPROCESSORS_ONLN)
#else
#  define marcel_nbvps()             2
#endif

#  define marcel_printf              printf
#  define marcel_fflush              fflush

#ifdef __linux__
#  define marcel_apply_vpset(set) do { sched_setaffinity(0, sizeof(*set), set); } while(0)
#else
#  define marcel_apply_vpset(set) (void)0
#endif
#  define marcel_vpset_t            unsigned long
#  define MARCEL_VPSET_VP(num)      (1<<(num))

#elif defined(MARCEL)

#  include "marcel.h"

#else

#  error "No selected mode..."

#endif // PTHREAD

#endif // THREAD_H
