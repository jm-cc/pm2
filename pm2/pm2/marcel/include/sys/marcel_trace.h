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

#ifndef MARCEL_TRACE_EST_DEF
#define MARCEL_TRACE_EST_DEF

#ifdef MAR_TRACE

#define MTRACE(msg, pid) \
  (__mar_trace ? \
    (void)fprintf(stderr, \
            "[P%02d][%-11s:%3ld (pid=%p)." \
            " %3d A,%3d S,%3d B,%3d F /%3d T]\n", \
            ((pid)->lwp ? (pid)->lwp->number : -1), \
            msg, (pid)->number, (pid), \
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
