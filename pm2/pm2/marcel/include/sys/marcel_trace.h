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
$Log: marcel_trace.h,v $
Revision 1.4  2000/04/11 09:07:18  rnamyst
Merged the "reorganisation" development branch.

Revision 1.3.2.6  2000/03/31 18:38:38  vdanjean
Activation mono OK

Revision 1.3.2.5  2000/03/31 11:20:29  vdanjean
debug ACT

Revision 1.3.2.4  2000/03/31 09:39:47  vdanjean
debug ACT

Revision 1.3.2.3  2000/03/30 16:57:28  rnamyst
Introduced TOP_STACK_FREE_AREA...

Revision 1.3.2.2  2000/03/29 14:24:39  rnamyst
Added the marcel_stdio.h that provides the marcel_printf functions.
=> MTRACE now uses marcel_fprintf.

Revision 1.3.2.1  2000/03/15 15:54:55  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.3  2000/01/31 15:56:50  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_TRACE_EST_DEF
#define MARCEL_TRACE_EST_DEF

#ifdef MAR_TRACE

#include "marcel_stdio.h"

#define MTRACE(msg, pid) \
  (__mar_trace ? \
    (void)marcel_fprintf(stderr, \
            "[P%02d][%-11s:%3ld (pid=%p:%X)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            ((pid)->lwp ? (pid)->lwp->number : -1), \
            msg, (pid)->number, (pid), (pid)->special_flags, \
            marcel_activethreads(), \
            marcel_sleepingthreads(), \
            marcel_blockedthreads(), \
            marcel_frozenthreads(), \
            marcel_nbthreads() + 1) \
   : (void)0)



#else

#define MTRACE(msg, pid) (void)0

#endif

extern unsigned __mar_trace;

void marcel_trace_on(void);

void marcel_trace_off(void);

#endif
