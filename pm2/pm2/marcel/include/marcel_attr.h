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

#ifndef MARCEL_ATTR_EST_DEF
#define MARCEL_ATTR_EST_DEF

_PRIVATE_ typedef struct {
  unsigned stack_size;
  char *stack_base;
  boolean detached;
  unsigned user_space;
  unsigned priority;
  unsigned not_migratable;
  unsigned not_deviatable;
  int sched_policy;
} marcel_attr_t;

int marcel_attr_init(marcel_attr_t *attr);
#define marcel_attr_destroy(attr_ptr)	0

int marcel_attr_setstacksize(marcel_attr_t *attr, size_t stack);
int marcel_attr_getstacksize(marcel_attr_t *attr, size_t *stack);

int marcel_attr_setstackaddr(marcel_attr_t *attr, void *addr);
int marcel_attr_getstackaddr(marcel_attr_t *attr, void **addr);

int marcel_attr_setdetachstate(marcel_attr_t *attr, boolean detached);
int marcel_attr_getdetachstate(marcel_attr_t *attr, boolean *detached);

int marcel_attr_setuserspace(marcel_attr_t *attr, unsigned space);
int marcel_attr_getuserspace(marcel_attr_t *attr, unsigned *space);

int marcel_attr_setprio(marcel_attr_t *attr, unsigned prio);
int marcel_attr_getprio(marcel_attr_t *attr, unsigned *prio);

int marcel_attr_setmigrationstate(marcel_attr_t *attr, boolean migratable);
int marcel_attr_getmigrationstate(marcel_attr_t *attr, boolean *migratable);

int marcel_attr_setdeviationstate(marcel_attr_t *attr, boolean deviatable);
int marcel_attr_getdeviationstate(marcel_attr_t *attr, boolean *deviatable);

int marcel_attr_setschedpolicy(marcel_attr_t *attr, int policy);
int marcel_attr_getschedpolicy(marcel_attr_t *attr, int *policy);

_PRIVATE_ extern marcel_attr_t marcel_attr_default;

#endif
