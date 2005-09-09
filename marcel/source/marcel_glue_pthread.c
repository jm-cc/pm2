
#include "marcel.h"
#ifdef MA__PTHREAD_FUNCTIONS

#include "pm2_common.h"


int __pthread_create_2_1(pthread_t *thread, const pthread_attr_t *attr,
                         void * (*start_routine)(void *), void *arg)
{
  /* The ATTR attribute is not really of type `pthread_attr_t *'.  It has
     the old size and access to the new members might crash the program.
     We convert the struct now.  */
  marcel_attr_t new_attr;
  static int _launched=0;

  if (__builtin_expect(_launched, 1)==0) {
    marcel_start_sched(NULL, NULL);
  }
  if (attr != NULL)
    {
      memcpy (&new_attr, attr,
              (size_t) &(((marcel_attr_t*)NULL)->user_space));
      new_attr.user_space = 0;
      new_attr.immediate_activation = FALSE;
      new_attr.not_migratable = 1;
      new_attr.not_deviatable = 0;
      new_attr.sched = marcel_sched_attr_default;
      new_attr.vpmask = MARCEL_VPMASK_EMPTY;
      new_attr.flags = 0;
      // Le cast par (void*) est demandé par gcc pour respecter la norme C
      // sinon, on a:
      // warning: dereferencing type-punned pointer will break strict-aliasing rules
      attr = (pthread_attr_t*)(void*)&new_attr;
    }
  return marcel_create ((marcel_t*)thread, (marcel_attr_t*)attr,
			start_routine, arg);
}

#if SHLIB_COMPAT (libpthread, GLIBC_2_0, GLIBC_2_2)
#include <sys/shm.h>
#ifndef STACK_SIZE
#define STACK_SIZE  (2 * 1024 * 1024)
#endif
int __pthread_create_2_0(pthread_t *thread, const pthread_attr_t *attr,
                         void * (*start_routine)(void *), void *arg)
{
  /* The ATTR attribute is not really of type `pthread_attr_t *'.  It has
     the old size and access to the new members might crash the program.
     We convert the struct now.  */
  pthread_attr_t new_attr;

  if (attr != NULL)
    {
      size_t ps = __getpagesize ();

      memcpy (&new_attr, attr,
              (size_t) &(((pthread_attr_t*)NULL)->__guardsize));
      new_attr.__guardsize = ps;
      new_attr.__stackaddr_set = 0;
      new_attr.__stackaddr = NULL;
      new_attr.__stacksize = STACK_SIZE - ps;
      attr = &new_attr;
    }
  return __pthread_create_2_1 (thread, attr, start_routine, arg);
}
compat_symbol (libpthread, __pthread_create_2_0, pthread_create, GLIBC_2_0);
#endif


versioned_symbol (libpthread, __pthread_create_2_1, pthread_create, GLIBC_2_1);

pthread_t pthread_self(void)
{
	return (pthread_t)marcel_self();
}

static int _fisrt_errno=0;
static int _initialized=0;

int * __errno_location()
{
	int * res;

	if (_initialized) {
		res=&marcel_self()->__errno;
	} else {
		res=&_fisrt_errno;
	}
	return res;
}

static int _fisrt_h_errno=0;

int * __h_errno_location()
{
	int * res;

	if (_initialized) {
		res=&marcel_self()->__h_errno;
	} else {
		res=&_fisrt_h_errno;
	}
	return res;
}

static struct __res_state _fisrt_res_state;

/* Return thread specific resolver state.  */
struct __res_state *
__res_state (void)
{
	struct __res_state * res;

	if (_initialized) {
		res=&marcel_self()->__res_state;
	} else {
		res=&_fisrt_res_state;
	}
	return res;
}

//static void pthread_initialize() __attribute__((constructor));

void __pthread_initialize_minimal(void)
{
	//marcel_init_section(MA_INIT_MAIN_LWP);
#ifdef PM2DEBUG
	printf("Initialisation mini libpthread marcel-based\n");
#endif
}

void __pthread_initialize(int* argc, char**argv)
{
	//marcel_init_section(MA_INIT_MAIN_LWP);
#ifdef PM2DEBUG
	printf("__Initialisation libpthread marcel-based\n");
#endif
	
}

void marcel_pthread_initialize(int* argc, char**argv)
{
	if (!_initialized) {
#ifdef PM2DEBUG
		//printf("Initialisation libpthread marcel-based\n");
#endif
		marcel_init_section(MA_INIT_MAIN_LWP);
		marcel_init_data(argc, argv);
		tbx_init(*argc, argv);
		/* TODO: A reporter : */
                //TODO__on_exit (pthread_onexit_process, NULL);
		/* TODO: à la création du premier thread */
#ifdef PM2DEBUG
		printf("Initialisation libpthread marcel-init done (and launched :-()\n");
#endif
	
		_initialized=1;
	}
}
//strong_alias(pthread_initialize,__pthread_initialize);
//static void pthread_finalize() __attribute__((destructor));
/*
static void pthread_finalize()
{
	printf("Ben c'est fini...\n");
	marcel_end();
}
*/

int __pthread_attr_init_2_1 (pthread_attr_t *__attr)
{
	return marcel_attr_init((marcel_attr_t*)__attr);
}
versioned_symbol (libpthread, __pthread_attr_init_2_1, pthread_attr_init, GLIBC_2_1);

int pthread_attr_setdetachstate (pthread_attr_t *__attr,
                                        int __detachstate)
{
	return marcel_attr_setdetachstate((marcel_attr_t*)__attr,
					  __detachstate);
}

#include <semaphore.h>

int __sem_init_2_1 (sem_t *__sem, int __pshared, unsigned int __value)
{
	marcel_sem_init((marcel_sem_t*)__sem, __value);
	return 0;
}
versioned_symbol (libpthread, __sem_init_2_1, sem_init, GLIBC_2_1);

int __sem_wait_2_1 (sem_t *__sem)
{
	marcel_sem_P((marcel_sem_t*)__sem);
	return 0;
}
versioned_symbol (libpthread, __sem_wait_2_1, sem_wait, GLIBC_2_1);

int __sem_post_2_1 (sem_t *__sem)
{
	marcel_sem_V((marcel_sem_t*)__sem);
	return 0;
}
versioned_symbol (libpthread, __sem_post_2_1, sem_post, GLIBC_2_1);


int pthread_equal(pthread_t thread1, pthread_t thread2)
{
  return thread1 == thread2;
}

int usleep_divert (__useconds_t __useconds)
{
  return 0;
}
versioned_symbol (libpthread, usleep_divert, usleep, GLIBC_2_2);

int __libc_sigaction (int sig, const struct sigaction *act, struct sigaction *oact);
int __sigaction (int sig, const struct sigaction *act, struct sigaction *oact)
{
#if 0
  if (__builtin_expect (sig == SIGCANCEL || sig == SIGSETXID, 0))
    {
      __set_errno (EINVAL);
      return -1;
    }
#endif

  return __libc_sigaction (sig, act, oact);
}
//libc_hidden_weak (__sigaction)
weak_alias (__sigaction, sigaction)

#ifdef PM2_DEV
#warning _pthread_cleanup_push,restore à écrire
#endif
#if 0
strong_alias(_pthread_cleanup_push_defer,_pthread_cleanup_push);
strong_alias(_pthread_cleanup_pop_restore,_pthread_cleanup_pop);

#warning _pthread_cleanup_push,restore à vérifier
void _pthread_cleanup_push(struct _pthread_cleanup_buffer * buffer,
                           void (*routine)(void *), void * arg)
{
	return marcel_cleanup_push(routine, arg);
}

void _pthread_cleanup_pop(struct _pthread_cleanup_buffer * buffer,
                          int execute)
{
	return marcel_cleanup_pop(execute);
}
#endif

int
//attribute_hidden
__pthread_enable_asynccancel (void)
{
#if 0
// FIXME
  struct marcel_task *self = MARCEL_SELF;
  int oldval = THREAD_GETMEM (self, cancelhandling);

  while (1)
    {
      int newval = oldval | CANCELTYPE_BITMASK;

      if (newval == oldval)
        break;

      int curval = THREAD_ATOMIC_CMPXCHG_VAL (self, cancelhandling, newval,
                                              oldval);
      if (__builtin_expect (curval == oldval, 1))
        {
          if (CANCEL_ENABLED_AND_CANCELED_AND_ASYNCHRONOUS (newval))
            {
              THREAD_SETMEM (self, result, PTHREAD_CANCELED);
              __do_cancel ();
            }

          break;
        }

      /* Prepare the next round.  */
      oldval = curval;
    }

  return oldval;
#endif
  return 0;
}


void
//internal_function attribute_hidden
__pthread_disable_asynccancel (int oldtype)
{
#if 0
// FIXME
  /* If asynchronous cancellation was enabled before we do not have
 *      anything to do.  */
  if (oldtype & CANCELTYPE_BITMASK)
    return;

  struct marcel_task *self = MARCEL_SELF;
  int oldval = THREAD_GETMEM (self, cancelhandling);

  while (1)
    {
      int newval = oldval & ~CANCELTYPE_BITMASK;

      if (newval == oldval)
        break;

      int curval = THREAD_ATOMIC_CMPXCHG_VAL (self, cancelhandling, newval,
                                              oldval);
      if (__builtin_expect (curval == oldval, 1))
        break;

      /* Prepare the next round.  */
      oldval = curval;
    }
#endif
}


#endif /* MA__PTHREAD_FUNCTIONS */
