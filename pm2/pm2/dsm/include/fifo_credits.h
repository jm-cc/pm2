/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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

#ifndef FIFO_CREDITS_IS_DEF
#define FIFO_CREDITS_IS_DEF

#include "dsm_const.h" /* access_t */

typedef dsm_node_t fifo_item_t;

typedef struct s_fifo
{
  fifo_item_t *head;
  fifo_item_t *start;
  fifo_item_t *end;
  marcel_sem_t sem;
  int size;
} fifo_t;

void fifo_init(fifo_t *fifo, int fifo_size);

void fifo_set_next(fifo_t *fifo, dsm_node_t node);

dsm_node_t fifo_get_next(fifo_t *fifo);

void fifo_exit(fifo_t *fifo);

#endif

