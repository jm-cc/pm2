
#ifndef MARCEL_MUTEX_EST_DEF
#define MARCEL_MUTEX_EST_DEF

#include <sys/time.h>

#define MARCEL_MUTEX_INITIALIZER { 1 }

_PRIVATE_ typedef struct {
	int dummy;
} marcel_mutexattr_struct;

_PRIVATE_ typedef marcel_sem_t marcel_mutex_t;

_PRIVATE_ typedef struct {
	int dummy;
} marcel_condattr_struct;

_PRIVATE_ typedef marcel_sem_t marcel_cond_t;

typedef marcel_mutexattr_struct marcel_mutexattr_t;

typedef marcel_condattr_struct marcel_condattr_t;

int marcel_mutexattr_init(marcel_mutexattr_t *attr);
int marcel_mutexattr_destroy(marcel_mutexattr_t *attr);

int marcel_mutex_init(marcel_mutex_t *mutex, marcel_mutexattr_t *attr);
int marcel_mutex_destroy(marcel_mutex_t *mutex);

int marcel_mutex_lock(marcel_mutex_t *mutex);
int marcel_mutex_trylock(marcel_mutex_t *mutex);
int marcel_mutex_unlock(marcel_mutex_t *mutex);

int marcel_condattr_init(marcel_condattr_t *attr);
int marcel_condattr_destroy(marcel_condattr_t *attr);

int marcel_cond_init(marcel_cond_t *cond, marcel_condattr_t *attr);
int marcel_cond_destroy(marcel_cond_t *cond);

int marcel_cond_signal(marcel_cond_t *cond);
int marcel_cond_broadcast(marcel_cond_t *cond);

int marcel_cond_wait(marcel_cond_t *cond, marcel_mutex_t *mutex);
int marcel_cond_timedwait(marcel_cond_t *cond, marcel_mutex_t *mutex,
			  const struct timespec *abstime);
#endif
