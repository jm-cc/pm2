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
$Log: pm2_timing.h,v $
Revision 1.4  2000/11/15 21:32:38  rnamyst
Removed 'timing' and 'safe_malloc' : all modules now use the toolbox for timing & safe malloc

Revision 1.3  2000/02/28 11:18:04  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:49:46  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef PM2_TIMING_EST_DEF
#define PM2_TIMING_EST_DEF

#ifdef PM2_TIMING

#include "tbx.h"

#define TIMING_EVENT(name)   TIME(name)

#else

#define TIMING_EVENT(name)   /* nothing */

#endif

#endif
