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

#include "rpc_defs.h"

int *les_modules, nb_modules;

#define NB	3

static char *mess[NB] = {
  "Hi Guys !",
  "Hi Girls !",
  "Hello world !"
};

static unsigned sched_policy[NB] = {
  MARCEL_SCHED_FIXED(2),
  MARCEL_SCHED_FIXED(1),
  MARCEL_SCHED_FIXED(0)
};

BEGIN_SERVICE(SAMPLE)
   int i, j;

   for(i=0; i<10; i++) {
      pm2_printf("%s (I am %p on LWP %d)\n",
		 req.tab, marcel_self(), marcel_current_vp());
      for(j=5000000; j; j--) ;
   }
END_SERVICE(SAMPLE)

void f(void)
{
  pm2_rpc_wait_t att[NB];
  int i;
  LRPC_REQ(SAMPLE) req;
  pm2_attr_t attr;

  pm2_attr_init(&attr);
  for(i=0; i<NB; i++) {
    strcpy(req.tab, mess[i]);
    pm2_attr_setschedpolicy(&attr, sched_policy[i]);
    pm2_rpc_call(les_modules[1], SAMPLE, &attr,
		 &req, NULL, &att[i]);
  }

  for(i=0; i<NB; i++)
    pm2_rpc_wait(&att[i]);
}

int pm2_main(int argc, char **argv)
{
  pm2_init_rpc();

  DECLARE_LRPC(SAMPLE);

  pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

  {
    int i;

    for(i=1; i<argc; i++)
      fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
  }

  if(pm2_self() == les_modules[0]) { /* master process */

    f();

    pm2_kill_modules(les_modules, nb_modules);
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
