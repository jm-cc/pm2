
#include <pm2.h>
#include "regul.h"
#include "rpc_defs.h"

/* Comment the following line to disable dynamic thread balancing */
/* #define DO_REGULATE */

#define MAX_THREADS	  1024

#define FREQ_OBS          5

#define GROSSE_VARIATION  1

#define abs(x)  (((x) < 0) ? -(x) : (x))

static unsigned local_load = 0, other_load = 0;
static int module_precedent, module_suivant;
static int *les_mod, nb_mod;

static marcel_t spy_pid;
static marcel_sem_t spy_ok;

static unsigned charge_locale()
{
  unsigned my_prio;

  marcel_getprio(marcel_self(), &my_prio);
  if(!strcmp(mad_arch_name(), "pvm"))
    return marcel_priosum_np() - my_prio - MAX_PRIO;
  else
    return marcel_priosum_np() - my_prio;
}

static void informer()
{
  LRPC_REQ(LOAD) req;

  req.load = local_load;
  
  QUICK_ASYNC_LRPC(module_precedent, LOAD, &req);
}

#ifdef DO_REGULATE
static void reguler()
{ 
  /* liste de threads par ordre de priorité décroissante : */
  static marcel_t pids[MAX_THREADS], to_migrate[MAX_THREADS];

  int i = 0, n = 0, nb;
  unsigned quantite_a_migrer, p;

   if(local_load > other_load) {

      pm2_freeze();

      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      if(nb >= MAX_THREADS)
         RAISE(CONSTRAINT_ERROR);

      quantite_a_migrer = (local_load - other_load) / 2;

      while(quantite_a_migrer && i < nb) {
         marcel_getprio(pids[i], &p);
         if(p <= quantite_a_migrer) {
            quantite_a_migrer -= p;
            to_migrate[n++] = pids[i];
         }
         i++;
      }

      pm2_migrate_group(to_migrate, n, module_suivant);
#ifdef DEBUG
      if(n > 0)
	printf("Départ de %d threads (ch'suis trop chargé...)\n", n);
#endif
   }
}
#endif

/* *** SERVICES *** */

BEGIN_SERVICE(LOAD)
  other_load = req.load;
#ifdef DEBUG
  printf("Charge locale : %d, charge du suivant : %d\n", local_load, other_load);
#endif
END_SERVICE(LOAD)

BEGIN_SERVICE(QUIT)
  marcel_cancel(spy_pid);
END_SERVICE(QUIT)

BEGIN_SERVICE(SPY)

  int charge;

  spy_pid = marcel_self();
  marcel_sem_V(&spy_ok);

  while(1) {
    marcel_delay(FREQ_OBS);

    charge = charge_locale();
    if(abs(charge - local_load) >= GROSSE_VARIATION) {
      local_load = charge;
      informer();
    } else
      local_load = charge;
#ifdef DO_REGULATE    
    reguler();
#endif
  }
END_SERVICE(SPY)


void regul_init_rpc()
{
  DECLARE_LRPC_WITH_NAME(SPY, "spy", OPTIMIZE_IF_LOCAL);
  DECLARE_LRPC(LOAD);
  DECLARE_LRPC(QUIT);
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

void regul_exit()
{
  if(pm2_self() == les_mod[0])
    MULTI_QUICK_ASYNC_LRPC(les_mod, nb_mod, QUIT, NULL);
}

