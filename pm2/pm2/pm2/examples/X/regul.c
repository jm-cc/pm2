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

#include <pm2.h>
#include "regul.h"
#include "rpc_defs.h"

#define MAX_THREADS	  1024

#define FREQ_OBS          100

#define GROSSE_VARIATION  1

#define abs(x)  (((x) < 0) ? -(x) : (x))

static unsigned local_load = 0, other_load = 0;
static int module_precedent, module_suivant;
static int *les_mod, nb_mod;

static marcel_t spy_pid;
static marcel_sem_t spy_ok;

static boolean do_regulate = FALSE;

static unsigned charge_locale()
{
  unsigned nb;

  pm2_freeze();
  pm2_threads_list(0, NULL, &nb, MIGRATABLE_ONLY);
  pm2_unfreeze();

  return nb;
}

static void informer()
{
  LRPC_REQ(LOAD) req;

  req.load = local_load;
  
  QUICK_ASYNC_LRPC(module_precedent, LOAD, &req);
}

static void reguler()
{ 
  /* liste de threads par ordre de priorité décroissante : */
  static marcel_t pids[MAX_THREADS], to_migrate[MAX_THREADS];

  int  n = 0, nb;
  unsigned quantite_a_migrer;

   if(local_load > other_load + 1) {

      pm2_freeze();

      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      if(nb >= MAX_THREADS)
         RAISE(CONSTRAINT_ERROR);

      quantite_a_migrer = (local_load - other_load) / 2;
#ifdef DEBUG
      fprintf(stderr, "quantite a migrer = %d\n", quantite_a_migrer);
#endif
      for(n=0; (quantite_a_migrer && n < nb); n++) {
	quantite_a_migrer--;;
	to_migrate[n] = pids[n];
      }

      pm2_migrate_group(to_migrate, n, module_suivant);
#ifdef DEBUG
      if(n > 0)
	printf("Départ de %d threads.\n", n);
#endif
   }
}

/* *** SERVICES *** */

BEGIN_SERVICE(LOAD)
  other_load = req.load;
#ifdef DEBUG
  printf("Charge locale : %d, charge du suivant : %d\n", local_load, other_load);
#endif
END_SERVICE(LOAD)


BEGIN_SERVICE(SPY)

  int charge;

  spy_pid = marcel_self();
  marcel_sem_V(&spy_ok);

  while(1) {
    marcel_delay(FREQ_OBS);

    if(do_regulate) {
      charge = charge_locale();
      if(abs(charge - local_load) >= GROSSE_VARIATION) {
	local_load = charge;
	informer();
      } else
	local_load = charge;
      reguler();
    }
  }
END_SERVICE(SPY)


void regul_init_rpc()
{
  DECLARE_LRPC_WITH_NAME(SPY, "spy", OPTIMIZE_IF_LOCAL);
  DECLARE_LRPC(LOAD);
}

void regul_init(int *les_modules, int nb_modules)
{
  int i;

  for(i=0; les_modules[i] != pm2_self(); i++) ;
  module_precedent = les_modules[(i+nb_modules-1)%nb_modules];
  module_suivant = les_modules[(i+1)%nb_modules];

  marcel_sem_init(&spy_ok, 0);

  ASYNC_LRPC(pm2_self(), SPY, STD_PRIO, DEFAULT_STACK, NULL);

  marcel_sem_P(&spy_ok);

  les_mod = les_modules; nb_mod = nb_modules;
}

void regul_start()
{
  do_regulate = TRUE;
}

void regul_stop()
{
  do_regulate = FALSE;
}

void regul_exit()
{
  marcel_cancel(spy_pid);
}

