
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

static __inline__ int dsm_mutexattr_init(dsm_mutexattr_t *attr) __attribute__ ((unused));
static __inline__ int dsm_mutexattr_init(dsm_mutexattr_t *attr)
{
  attr->owner = 0;
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
