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

#ifndef MARCEL_SEM_EST_DEF
#define MARCEL_SEM_EST_DEF

_PRIVATE_ typedef struct cell_struct {
  marcel_t task;
  struct cell_struct *next;
  boolean blocked;
} cell;

_PRIVATE_ struct semaphor_struct {
  int value;
  struct cell_struct *first,*last;
  marcel_lock_t lock;  /* For preventing concurrent access from multiple LWPs */
};

typedef struct semaphor_struct marcel_sem_t;

void marcel_sem_init(marcel_sem_t *s, int initial);
void marcel_sem_P(marcel_sem_t *s);
void marcel_sem_V(marcel_sem_t *s);
void marcel_sem_timed_P(marcel_sem_t *s, unsigned long timeout);
_PRIVATE_ void marcel_sem_VP(marcel_sem_t *s1, marcel_sem_t *s2);
_PRIVATE_ void marcel_sem_unlock_all(marcel_sem_t *s);

#endif
