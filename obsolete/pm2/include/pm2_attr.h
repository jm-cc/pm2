
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

#ifndef PM2_ATTR_EST_DEF
#define PM2_ATTR_EST_DEF

#include "marcel.h"
#include "madeleine.h"
#include "pm2_types.h"

typedef unsigned pm2_channel_t;

_PRIVATE_ typedef struct {
  int sched_policy;
  pm2_channel_t channel;
  pm2_completion_t *completion;
} pm2_attr_t;

int pm2_attr_init(pm2_attr_t *attr);

int pm2_attr_setschedpolicy(pm2_attr_t *attr, int policy);
int pm2_attr_getschedpolicy(pm2_attr_t *attr, int *policy);

int pm2_attr_setchannel(pm2_attr_t *attr, unsigned channel);
int pm2_attr_getchannel(pm2_attr_t *attr, unsigned *channel);

int pm2_attr_setcompletion(pm2_attr_t *attr, pm2_completion_t *completion);
int pm2_attr_getcompletion(pm2_attr_t *attr, pm2_completion_t **completion);

_PRIVATE_ extern pm2_attr_t pm2_attr_default;

#endif
