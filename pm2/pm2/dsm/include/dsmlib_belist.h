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

              Eager Release Consistency Implementation
                         Vincent Bernardi

                      2000 All Rights Reserved


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
/* Warning: All the following is NOT reentrant.*/
#include "tbx.h"

typedef struct be_cell { unsigned long number; struct be_cell * next;} be_cell;

typedef be_cell *be_list;

be_list init_be_list(long nb_preallocated_cells); //Warning: To be called only
                                                  //         ONCE!!! 
void add_to_be_list(be_list *my_list, unsigned long to_add);

unsigned long remove_first_from_be_list(be_list *my_list);

int remove_if_noted(be_list *my_list, unsigned long to_remove);

void fprintf_be_list(be_list the_list);
