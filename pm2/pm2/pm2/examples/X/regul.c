
#include "pm2.h"
#include "regul.h"

#define MAX_THREADS	  1024
#define FREQ_OBS          100
#define GROSSE_VARIATION  1

#define abs(x)  (((x) < 0) ? -(x) : (x))

static unsigned local_load = 0, other_load = 0;
static int module_precedent, module_suivant;

static boolean do_regulate = FALSE, finished = FALSE;

static unsigned LOAD;

static unsigned charge_locale(void)
{
  unsigned nb;

  pm2_freeze();
  pm2_threads_list(0, NULL, &nb, MIGRATABLE_ONLY);
  pm2_unfreeze();

  return nb;
}

static void informer(void)
{
  pm2_rawrpc_begin(module_precedent, LOAD, NULL);
  old_mad_pack_int(MAD_IN_HEADER, &local_load, 1);
  pm2_rawrpc_end();
}

static void reguler(void)
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

void load_service(void)
{
  old_mad_unpack_int(MAD_IN_HEADER, &other_load, 1);
  pm2_rawrpc_waitdata();
}

void spy(void *arg)
{
  int charge;

  while(!finished) {
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
}

void regul_rpc_init(void)
{
  pm2_rawrpc_register(&LOAD, load_service);
}

void regul_init(void)
{
  module_precedent = (pm2_self() + pm2_config_size() - 1) % pm2_config_size();
  module_suivant = (pm2_self() + 1) % pm2_config_size();

  pm2_thread_create(spy, NULL);
}

void regul_start(void)
{
  do_regulate = TRUE;
}

void regul_stop(void)
{
  do_regulate = FALSE;
}

void regul_exit()
{
  finished = TRUE;
}

