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

#ifndef PM2_ATTR_EST_DEF
#define PM2_ATTR_EST_DEF

#include <marcel.h>

_PRIVATE_ typedef struct {
  unsigned priority;
  int sched_policy;
} pm2_attr_t;

int pm2_attr_init(pm2_attr_t *attr);

int pm2_attr_setprio(pm2_attr_t *attr, unsigned prio);
int pm2_attr_getprio(pm2_attr_t *attr, unsigned *prio);

int pm2_attr_setschedpolicy(pm2_attr_t *attr, int policy);
int pm2_attr_getschedpolicy(pm2_attr_t *attr, int *policy);

_PRIVATE_ extern pm2_attr_t pm2_attr_default;

#endif
