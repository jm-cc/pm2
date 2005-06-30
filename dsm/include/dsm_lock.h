
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

#ifndef DSM_LOCK_EST_DEF
#define DSM_LOCK_EST_DEF

//#define TRACE_LOCK

#include "dsm_mutex.h"
#include "dsm_protocol_policy.h"
/* the following is useful for "TOKEN_LOCK_NONE" */
#include "token_lock.h"

#define _MAX_PROT_PER_LOCK 10

typedef struct {
  dsm_mutex_t dsm_mutex;
  dsm_proto_t prot[_MAX_PROT_PER_LOCK];
  int nb_prot;
} dsm_lock_struct_t;

typedef dsm_lock_struct_t *dsm_lock_t;

typedef struct {
  dsm_mutexattr_t dsm_mutex_attr;
  dsm_proto_t prot[_MAX_PROT_PER_LOCK];
  int nb_prot;
} dsm_lock_attr_t;

#define dsm_lock_attr_set_owner(attr, owner) dsm_mutexattr_setowner(&attr->dsm_mutex, owner)
#define dsm_lock_attr_get_owner(attr, owner) dsm_mutexattr_getowner(&attr->dsm_mutex, owner)

void dsm_lock_attr_init(dsm_lock_attr_t *attr);

void dsm_lock_attr_register_protocol(dsm_lock_attr_t *attr, dsm_proto_t prot);

void dsm_lock_init(dsm_lock_t *lock, dsm_lock_attr_t *attr);

static __inline__ void dsm_lock(dsm_lock_t lock)  __attribute__ ((unused));

static __inline__ void dsm_lock(dsm_lock_t lock) 
{ 
    int i;

#ifdef TRACE_LOCK
  fprintf(stderr,"[%s]: Entering...(I am %p)\n", __FUNCTION__, marcel_self());
#endif
    dsm_mutex_lock(&(lock->dsm_mutex)); 
    for (i = 0; i < lock->nb_prot; i++) 
      if (dsm_get_acquire_func(lock->prot[i]) != NULL)
	(*dsm_get_acquire_func(lock->prot[i]))(TOKEN_LOCK_NONE); 
#ifdef TRACE_LOCK
  fprintf(stderr,"[%s]: Exiting...\n", __FUNCTION__);
#endif
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
	(*dsm_get_release_func(lock->prot[i]))(TOKEN_LOCK_NONE); 
    dsm_mutex_unlock(&(lock->dsm_mutex)); 
#ifdef TRACE_LOCK
  fprintf(stderr,"[%s]: Exiting...\n", __FUNCTION__);
#endif
}

#endif







