/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <pm2_attr.h>

pm2_attr_t pm2_attr_default = {
  STD_PRIO,           /* priority */
  MARCEL_SCHED_OTHER, /* scheduling policy */
  0,                  /* channel number */
  NULL                /* completion ptr */
};

int pm2_attr_init(pm2_attr_t *attr)
{
   *attr = pm2_attr_default;
   return 0;
}

int pm2_attr_setprio(pm2_attr_t *attr, unsigned prio)
{
   if(prio == 0 || prio > MAX_PRIO)
      RAISE(CONSTRAINT_ERROR);
   attr->priority = prio;
   return 0;
}

int pm2_attr_getprio(pm2_attr_t *attr, unsigned *prio)
{
   *prio = attr->priority;
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

