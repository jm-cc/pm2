
#include "marcel.h"
#include "dsm_lock.h"

#define DSM_LOCKS 15

static struct {
  dsm_lock_struct_t locks[DSM_LOCKS];
  int nb;
} _dsm_lock_table;


void dsm_lock_init(dsm_lock_t *lock, dsm_lock_attr_t *attr)
{
  int i;

  LOG_IN();

  *lock = &_dsm_lock_table.locks[_dsm_lock_table.nb++];

  if(!attr)
    (*lock)->dsm_mutex.owner = 0;
  else
    (*lock)->dsm_mutex.owner = attr->dsm_mutex_attr.owner;

  marcel_mutex_init(&((*lock)->dsm_mutex.mutex), NULL);

  if (attr)
    {
      (*lock)->nb_prot = attr->nb_prot;
      
      for(i = 0; i < attr->nb_prot; i++)
	((*lock)->prot)[i] = (attr->prot)[i];
    }
  else
     (*lock)->nb_prot =0;
  
  LOG_OUT();
}




