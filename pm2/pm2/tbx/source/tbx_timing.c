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
$Log: tbx_timing.c,v $
Revision 1.1  2000/01/13 14:51:34  oaumage
Inclusion de la toolbox dans le repository

______________________________________________________________________________
*/

/*
 * Tbx_timing.c
 * ============
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <tbx.h>

static double   scale = 0.0;
long long       tbx_residual;
tbx_tick_t      tbx_new_event ;
tbx_tick_t      tbx_last_event;

void tbx_timing_init()
{
  static tbx_tick_t t1, t2;
  int i;
  
  tbx_residual = (unsigned long long)1 << 32;
  for(i=0; i<5; i++) {
    TBX_GET_TICK(t1);
    TBX_GET_TICK(t2);
    tbx_residual = min(tbx_residual, TBX_TICK_RAW_DIFF(t1, t2));
  }

#ifdef X86_ARCH
  {
    struct timeval tv1,tv2;

    TBX_GET_TICK(t1);
    gettimeofday(&tv1,0);
    usleep(50000);
    TBX_GET_TICK(t2);
    gettimeofday(&tv2,0);
    scale = ((tv2.tv_sec*1e6 + tv2.tv_usec) -
	     (tv1.tv_sec*1e6 + tv1.tv_usec)) / 
             (double)(TBX_TICK_DIFF(t1, t2));
  }
#else
  scale = 1.0;
#endif

  TBX_GET_TICK(tbx_last_event);
}

double tbx_tick2usec(long long t)
{
  return (double)(t)*scale;
}
