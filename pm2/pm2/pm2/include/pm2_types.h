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
$Log: pm2_types.h,v $
Revision 1.5  2000/09/22 08:45:12  rnamyst
PM2 startup funcs now use argc+argv. Modified the programs accordingly + fixed a bug in the TSP example.

Revision 1.4  2000/02/28 11:18:05  rnamyst
Changed #include <> into #include "".

Revision 1.3  2000/02/14 10:00:37  rnamyst
Modified to propose a new interface to the "pm2_completion" facilities.

Revision 1.2  2000/01/31 15:49:47  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef PM2_TYPES_EST_DEF
#define PM2_TYPES_EST_DEF

#include "marcel.h"

typedef void (*pm2_startup_func_t)(int argc, char *argv[]);

typedef void (*pm2_rawrpc_func_t)(void);

typedef void (*pm2_completion_handler_t)(void *);

typedef void (*pm2_pre_migration_hook)(marcel_t pid);
typedef any_t (*pm2_post_migration_hook)(marcel_t pid);
typedef void (*pm2_post_post_migration_hook)(any_t key);


_PRIVATE_ typedef struct pm2_completion_struct_t {
  marcel_sem_t sem;
  unsigned proc;
  struct pm2_completion_struct_t *c_ptr;
  pm2_completion_handler_t handler;
  void *arg;
} pm2_completion_t;

_PRIVATE_ typedef struct {
  void *stack_base;
  marcel_t task;
  unsigned long depl, blk;
} pm2_migr_ctl;

#endif
