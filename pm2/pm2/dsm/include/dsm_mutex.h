
#ifndef DSM_MUTEX_EST_DEF
#define DSM_MUTEX_EST_DEF

#include "marcel.h"
#include "dsm_const.h"

typedef struct {
  dsm_node_t owner;
} dsm_mutexattr_t;

typedef struct {
  marcel_mutex_t mutex;
  dsm_node_t owner;
} dsm_mutex_t;


static __inline__ int dsm_mutexattr_setowner(dsm_mutexattr_t *attr, dsm_node_t owner) __attribute__ ((unused));
static __inline__ int dsm_mutexattr_setowner(dsm_mutexattr_t *attr, dsm_node_t owner)
{
  attr->owner = owner;
  return 0;
}

static __inline__ int dsm_mutexattr_getowner(dsm_mutexattr_t *attr, dsm_node_t *owner) __attribute__ ((unused));
static __inline__ int dsm_mutexattr_getowner(dsm_mutexattr_t *attr, dsm_node_t *owner)
{
  *owner = attr->owner;
  return 0;
}

static __inline__ int dsm_mutex_init(dsm_mutex_t *mutex, dsm_mutexattr_t *attr) __attribute__ ((unused));
static __inline__ int dsm_mutex_init(dsm_mutex_t *mutex, dsm_mutexattr_t *attr)
{
  if(!attr)
    mutex->owner = 0;
  else
    mutex->owner = attr->owner;
  marcel_mutex_init(&mutex->mutex, NULL);
  return 0;
}

int dsm_mutex_lock(dsm_mutex_t *mutex);
int dsm_mutex_unlock(dsm_mutex_t *mutex);

#endif
