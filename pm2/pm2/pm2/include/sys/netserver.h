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

#ifndef NETSERVER_EST_DEF
#define NETSERVER_EST_DEF

#include <marcel.h>
#include <madeleine.h>

enum {
   NETSERVER_END,
   NETSERVER_CRITICAL_SECTION,
   NETSERVER_PRINTF,
   NETSERVER_MIGRATION,
   NETSERVER_LRPC,
   NETSERVER_LRPC_DONE,
   NETSERVER_ASYNC_LRPC,
   NETSERVER_QUICK_LRPC,
   NETSERVER_QUICK_ASYNC_LRPC,
   NETSERVER_CLONING,
   NETSERVER_MERGE
};

typedef struct {
  long piggyback_area[PIGGYBACK_AREA_LEN];
} generic_h;

typedef generic_h end_h;
typedef generic_h critical_section_h;
typedef generic_h print_h;
typedef generic_h migration_h;
typedef generic_h lrpc_h;
typedef generic_h lrpc_done_h;
typedef generic_h async_lrpc_h;
typedef generic_h quick_lrpc_h;
typedef generic_h quick_async_lrpc_h;
typedef generic_h cloning_h;
typedef generic_h merge_h;

void netserver_start(unsigned priority);

void netserver_wait_end(void);

#endif
