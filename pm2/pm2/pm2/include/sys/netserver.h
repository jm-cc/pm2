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
$Log: netserver.h,v $
Revision 1.7  2000/02/28 11:18:16  rnamyst
Changed #include <> into #include "".

Revision 1.6  2000/01/31 15:50:23  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef NETSERVER_EST_DEF
#define NETSERVER_EST_DEF

#include "marcel.h"
#include "madeleine.h"

enum {
   NETSERVER_END,
   NETSERVER_RAW_RPC  /* Must be the last one */
};

#ifdef MAD2
void netserver_start(p_mad_channel_t channel, unsigned priority);
#else
void netserver_start(unsigned priority);
#endif

void netserver_wait_end(void);

void netserver_stop(void);

#endif
