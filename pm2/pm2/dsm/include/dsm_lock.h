
#ifndef DSM_LOCK_EST_DEF
#define DSM_LOCK_EST_DEF

#define TRACE_LOCK

#include "dsm_mutex.h"
#include "dsm_protocol_policy.h"

#define _MAX_PROT_PER_LOCK 10

typedef struct {
  dsm_mutex_t dsm_mutex;
  int prot[_MAX_PROT_PER_LOCK];
  int nb_prot;
}dsm_lock_struct_t;

typedef dsm_lock_struct_t *dsm_lock_t;

typedef struct {
  dsm_mutexattr_t dsm_mutex_attr;
  int prot[_MAX_PROT_PER_LOCK];
  int nb_prot;
  }dsm_lock_attr_t;

#define dsm_lock_attr_set_owner(attr, owner) dsm_mutexattr_setowner(&attr->dsm_mutex, owner)
#define dsm_lock_attr_get_owner(attr, owner) dsm_mutexattr_getowner(&attr->dsm_mutex, owner)


static __inline__ void dsm_lock_attr_register_protocol(dsm_lock_attr_t *attr, int prot) __attribute__ ((unused));
static __inline__ void dsm_lock_attr_register_protocol(dsm_lock_attr_t *attr, int prot)
{
  if (attr->nb_prot >= _MAX_PROT_PER_LOCK)
    RAISE(CONSTRAINT_ERROR);
  attr->prot[attr->nb_prot++] = prot;
}

void dsm_lock_init(dsm_lock_t *lock, dsm_lock_attr_t *attr);

static __inline__ void dsm_lock(dsm_lock_t lock)  __attribute__ ((unused));

static __inline__ void dsm_lock(dsm_lock_t lock) 
{ 
    int i;

#ifdef TRACE_LOCK
  fprintf(stderr,"[%s]: Entering...\n", __FUNCTION__);
#endif
    dsm_mutex_lock(&(lock->dsm_mutex)); 
    for (i = 0; i < lock->nb_prot; i++) 
      if (dsm_get_acquire_func(lock->prot[i]) != NULL)
	(*dsm_get_acquire_func(lock->prot[i]))(); 
}
   

static __inline__ void dsm_unlock(dsm_lock_t lock) __attribute__ ((unused));

static __inline__ void dsm_unlock(dsm_lock_t lock) 
{ 
  int i;
#ifdef TRACE_LOCK
  fprintf(stderr,"[%s]: Entering...\n", __FUNCTION__);
#endif

    for (i = 0; i < lock->nb_prot; i++) 
      if (dsm_get_release_func(lock->prot[i]) != NULL)
	(*dsm_get_release_func(lock->prot[i]))(); 
    dsm_mutex_unlock(&(lock->dsm_mutex)); 
}

#endif







