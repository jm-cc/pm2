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
$Log: marcel_debug.h,v $
Revision 1.4  2000/04/28 18:33:36  vdanjean
debug actsmp + marcel_key

Revision 1.2  2000/04/11 09:07:17  rnamyst
Merged the "reorganisation" development branch.

Revision 1.1.2.1  2000/03/31 09:04:12  rnamyst
This is the old "debug.h" file.

______________________________________________________________________________
*/

#ifndef MARCEL_DEBUG_EST_DEF
#define MARCEL_DEBUG_EST_DEF

#include "marcel_stdio.h"

#undef mdebug
#ifdef DEBUG
#define mdebug(fmt, args...)  marcel_fprintf(stderr, fmt, ##args)
#define try_mdebug(fmt, args...) \
   (void)(preemption_enabled() ? marcel_fprintf(stderr, fmt, ##args) : 0)
#else
#define mdebug(fmt, args...)     (void)0
#define try_mdebug(fmt, args...) (void)0
#endif


#endif




