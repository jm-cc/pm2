
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


void dsm_lock_attr_register_protocol(dsm_lock_attr_t *attr, dsm_proto_t prot)
{
  if (attr->nb_prot >= _MAX_PROT_PER_LOCK)
    MARCEL_EXCEPTION_RAISE(CONSTRAINT_ERROR);
  attr->prot[attr->nb_prot++] = prot;
}


void dsm_lock_attr_init(dsm_lock_attr_t *attr)
{
  dsm_mutexattr_init(&attr->dsm_mutex_attr);
  attr->nb_prot = 0;
}

