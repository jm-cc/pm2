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

static exception MY_ERROR = "MY_ERROR";

#define NB	2

static char *mess[NB] = {
			"Hi guys !",
			"Hi girls !"
			};

typedef struct {
	marcel_t victime;
	int destination;
} migr_args;

any_t migreur(any_t arg)
{
  migr_args *args = (migr_args *)arg;

  marcel_detach(marcel_self());

  pm2_printf("Migration de %p sur [t%x]\n", args->victime, args->destination);

  pm2_freeze();
  pm2_migrate(args->victime, args->destination);

  return NULL;
}

BEGIN_SERVICE(LRPC_SAMPLE)
   int i, ind, nb;

   pm2_enable_migration();

   ind = 0;
   nb = nb_modules;

   for(i=0; i<3*nb; i++) {

      pm2_printf("%s\n", req.tab);

      marcel_delay(100);
      if(i+1 != 3*nb && (i+1) % 3 == 0) {
         ind++;
	 BEGIN
	   {
	     migr_args args;
	     marcel_attr_t attr;
	     unsigned granted;

	     args.victime = marcel_self();
	     args.destination = les_modules[ind];

	     marcel_attr_init(&attr);
	     marcel_attr_setstackaddr(&attr, slot_general_alloc(NULL, 0, &granted, NULL));
	     marcel_attr_setstacksize(&attr, granted);

	     marcel_create(NULL, &attr, migreur, &args);
	   }

	   while(pm2_self() != les_modules[ind])
	     /* attente active */ ;

	   fprintf(stderr, "C'est bon, j'ai migre...\n");

	   RAISE(MY_ERROR);
         EXCEPTION
            WHEN(MY_ERROR)
               pm2_printf("Ca y est, j'y suis !\n");
         END
      }
   }

END_SERVICE(LRPC_SAMPLE)

void f(void)
{ LRPC_REQ(LRPC_SAMPLE) req;
  pm2_rpc_wait_t att[NB];
  int i;

   LOOP(bcle)

      for(i=0; i<NB; i++) {
         strcpy(req.tab, mess[i]);
         LRP_CALL(pm2_self(), LRPC_SAMPLE, STD_PRIO, DEFAULT_STACK, &req, NULL, &att[i]);
	 marcel_delay(500);
      }

      for(i=0; i<NB; i++)
         LRP_WAIT(&att[i]);

      EXIT_LOOP(bcle);

   END_LOOP(bcle);
}

int pm2_main(int argc, char **argv)
{
   pm2_init_rpc();

   DECLARE_LRPC(LRPC_SAMPLE);

   pm2_init(&argc, argv, ASK_USER, &les_modules, &nb_modules);

   if(pm2_self() == les_modules[0]) { /* master process */

     malloc(100000);

      f();

      pm2_kill_modules(les_modules, nb_modules);
   }

   pm2_exit();

   tfprintf(stderr, "Main is ending\n");
   return 0;
}
