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
#include <sys/time.h>


/* ****************** fonctions de mesure du temps ************** */

static struct timeval _t1, _t2;
static struct timezone _tz;
static unsigned long _temps_residuel = 0;

#define top1() gettimeofday(&_t1, &_tz)
#define top2() gettimeofday(&_t2, &_tz)

void init_cpu_time(void)
{
   top1(); top2();
   _temps_residuel = 1000000L * _t2.tv_sec + _t2.tv_usec - 
                     (1000000L * _t1.tv_sec + _t1.tv_usec );
}

unsigned long cpu_time(void) /* retourne des microsecondes */
{
   return 1000000L * _t2.tv_sec + _t2.tv_usec - 
           (1000000L * _t1.tv_sec + _t1.tv_usec ) - _temps_residuel;
}

/* ************************************************************** */

int *les_modules, nb_modules, module_courant = 0;

int next_module(void)
{ int res;

   lock_task();
   module_courant = (module_courant+1) % (nb_modules);
   res = les_modules[module_courant];
   unlock_task();
   return res;
}

BEGIN_SERVICE(DICHOTOMY)
 int i;

   if(req.inf == req.sup) {
      res.res = req.inf;
   } else {
      int mid = (req.inf + req.sup)/2;
      LRPC_REQ(DICHOTOMY) req1, req2;
      LRPC_RES(DICHOTOMY) res1, res2;
      pm2_rpc_wait_t att[2];

      req1.inf = req.inf; req1.sup = mid;
      LRP_CALL(next_module(), DICHOTOMY, STD_PRIO, DEFAULT_STACK,
		&req1, &res1, &att[0]);

      req2.inf = mid+1; req2.sup = req.sup;
      LRP_CALL(next_module(), DICHOTOMY, STD_PRIO, DEFAULT_STACK,
		&req2, &res2, &att[1]);

      for(i=0; i<2; i++)
         LRP_WAIT(&att[i]);

      res.res = res1.res + res2.res;
   }
END_SERVICE(DICHOTOMY)


BEGIN_SERVICE(LRPC_START)
  unsigned long temps;
  LRPC_REQ(DICHOTOMY) req;
  LRPC_RES(DICHOTOMY) res;

   LOOP(bcle)
      tfprintf(stderr, "Entrez un entier raisonnable "
		"(0 pour terminer) : ");
      scanf("%d", &req.sup);

      if(!req.sup)
         EXIT_LOOP(bcle);

      req.inf = 1;

      top1();
      LRPC(pm2_self(), DICHOTOMY, STD_PRIO, DEFAULT_STACK, &req, &res);
      top2();
      temps = cpu_time();

      tfprintf(stderr, "temps = %d.%03dms\n", temps/1000, temps%1000);

      tfprintf(stderr, "1+...+%d = %d\n", req.sup, res.res);

   END_LOOP(bcle);

END_SERVICE(START)

int pm2_main(int argc, char **argv)
{
   init_cpu_time();

   pm2_init_rpc();

   DECLARE_LRPC_WITH_NAME(LRPC_START, "start", OPTIMIZE_IF_LOCAL);
   DECLARE_LRPC_WITH_NAME(DICHOTOMY, "fac", OPTIMIZE_IF_LOCAL);

   pm2_init(&argc, argv, ASK_USER, &les_modules, &nb_modules);

   if(pm2_self() == les_modules[0]) { /* master process */

      LRPC(pm2_self(), LRPC_START, STD_PRIO, DEFAULT_STACK, NULL, NULL);

      pm2_kill_modules(les_modules, nb_modules);
   }

   pm2_exit();

   fprintf(stderr, "Main is ending\n");
   return 0;
}
