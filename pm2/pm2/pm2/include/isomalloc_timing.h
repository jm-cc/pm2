/*

                          PM2 HIGH-PERF
           High Performance Parallel Multithreaded Machine
                           version 1.0

                    Raymond Namyst, Luc Bouge
            Laboratoire de l'Informatique du Parallelisme
                Ecole Normale Superieure de Lyon

 	       Yves Denneulin, Benoit Planquelle,
              Jean-Francois Mehaut, Jean-Marc Geib
         Laboratoire d'Informatique Fondamentale de Lille
         Universite des Sciences et Technologies de Lille

                    1997 All Rights Reserved


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
 about the suitability of this software for any purpose.  This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: isomalloc_timing.h,v $
Revision 1.4  2000/11/15 21:32:37  rnamyst
Removed 'timing' and 'safe_malloc' : all modules now use the toolbox for timing & safe malloc

Revision 1.3  2000/02/28 11:17:57  rnamyst
Changed #include <> into #include "".

Revision 1.2  2000/01/31 15:49:35  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef ISOMALLOC_TIMING_EST_DEF
#define ISOMALLOC_TIMING_EST_DEF

#ifdef ISOMALLOC_TIMING

#include "tbx.h"

#define TIMING_EVENT(name)  TIME(name)

#else

#define TIMING_EVENT(name)  /* nothing */

#endif

#endif
