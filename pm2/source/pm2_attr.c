
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

#include "pm2_attr.h"

pm2_attr_t pm2_attr_default = {
  MARCEL_SCHED_OTHER, /* scheduling policy */
  0,                  /* channel number */
  NULL                /* completion ptr */
};

int pm2_attr_init(pm2_attr_t *attr)
{
   *attr = pm2_attr_default;
   return 0;
}

int pm2_attr_setschedpolicy(pm2_attr_t *attr, int policy)
{
  attr->sched_policy = policy;
  return 0;
}

int pm2_attr_getschedpolicy(pm2_attr_t *attr, int *policy)
{
  *policy = attr->sched_policy;
  return 0;
}

int pm2_attr_setchannel(pm2_attr_t *attr, pm2_channel_t channel)
{
   attr->channel = channel;
   return 0;
}

int pm2_attr_getchannel(pm2_attr_t *attr, pm2_channel_t *channel)
{
   *channel = attr->channel;
   return 0;
}

int pm2_attr_setcompletion(pm2_attr_t *attr, pm2_completion_t *completion)
{
  attr->completion = completion;
  return 0;
}

int pm2_attr_getcompletion(pm2_attr_t *attr, pm2_completion_t **completion)
{
  *completion = attr->completion;
  return 0;
}

