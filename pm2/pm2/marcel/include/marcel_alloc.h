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

______________________________________________________________________________
$Log: marcel_alloc.h,v $
Revision 1.3  2000/02/28 10:26:37  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:56:19  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_ALLOC_EST_DEF
#define MARCEL_ALLOC_EST_DEF

#include "sys/isomalloc_archdep.h"

void marcel_slot_init(void);

void *marcel_slot_alloc(void);

void marcel_slot_free(void *addr);

void marcel_slot_exit(void);

#endif
