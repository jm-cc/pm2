
#include "rpc_defs.h"

#include <stdlib.h>
#include <sys/time.h>

#define ESSAIS 3

#define ONLY_ASYNC
/* #define ONLY_QUICK */

#define min(a, b) ((a) < (b) ? (a) : (b))

static unsigned autre;

static unsigned nb;
static marcel_sem_t fin_async_lrpc;

#ifndef ONLY_ASYNC
#ifndef ONLY_QUICK
static FILE *f1;
#endif
static FILE *f2;
#endif
#ifndef ONLY_QUICK
static FILE *f3;
#endif

BEGIN_SERVICE(INFO)
     /* voir stubs */
END_SERVICE(INFO)

BEGIN_SERVICE(LRPC_PING)
     /* voir stubs */
END_SERVICE(LRPC_PING)

BEGIN_SERVICE(LRPC_PING_ASYNC)
     if(NB_PING--)
       ASYNC_LRPC(autre, LRPC_PING_ASYNC, STD_PRIO, DEFAULT_STACK, NULL);
     else
       marcel_sem_V(&fin_async_lrpc);
END_SERVICE(LRPC_PING_ASYNC)

void informer(unsigned size, unsigned nbping)
{
  BUF_SIZE = size;
  NB_PING = nbping;

  QUICK_LRPC(autre, INFO, NULL, NULL);
}

void f(int bytes)
{
  tbx_tick_t t1, t2;
  unsigned long temps;
  unsigned n;
#ifndef ONLY_ASYNC
  unsigned i;
#endif

  if(bytes % 1024 == 0)
    tfprintf(stderr, "%d Koctets transférés :\n", bytes/1024);
  else
    tfprintf(stderr, "%d octets transférés :\n", bytes);

#ifndef ONLY_ASYNC

#ifndef ONLY_QUICK
    /************************* LRPC *************************/
    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {
      informer(bytes, nb/2);
      TBX_GET_TICK(t1);
      for(i=0; i<nb/2; i++)
	LRPC(autre, LRPC_PING, STD_PRIO, DEFAULT_STACK,
	     NULL, NULL);
      TBX_GET_TICK(t2);
      temps = min(TBX_TIMING_DELAY(t1, t2), temps);
    }

    tfprintf(stderr, "temps LRPC = %d.%03dms\n",
	     temps/1000, temps%1000);
    fprintf(f1, "%d %ld\n", bytes, temps/1000);
#endif /* ONLY_QUICK */

    /************************ QUICK_LRPC ***********************/
    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {
      informer(bytes, nb/2);
      TBX_GET_TICK(t1);
      for(i=0; i<nb/2; i++)
	QUICK_LRPC(autre, LRPC_PING, NULL, NULL);
      TBX_GET_TICK(t2);
      temps = min(TBX_TIMING_DELAY(t1, t2), temps);
    }

    tfprintf(stderr, "temps QUICK_LRPC = %d.%03dms\n",
	     temps/1000, temps%1000);
    fprintf(f2, "%d %ld\n", bytes, temps/1000);
#endif /* ONLY_ASYNC */

#ifndef ONLY_QUICK
    /********************** ASYNC_LRPC **********************/

    marcel_sem_init(&fin_async_lrpc, 0);
    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {
      informer(bytes, nb/2);
      NB_PING--;
      TBX_GET_TICK(t1);
      ASYNC_LRPC(autre, LRPC_PING_ASYNC, STD_PRIO, DEFAULT_STACK, NULL);
      marcel_sem_P(&fin_async_lrpc);
      TBX_GET_TICK(t2);

      temps = min(TBX_TIMING_DELAY(t1, t2), temps);
    }

    tfprintf(stderr, "temps ASYNC_LRPC = %ld.%03ldms\n",
	     temps/1000, temps%1000);
    fprintf(f3, "%d %ld\n", bytes, temps/1000);
#endif /* ifndef ONLY_QUICK */
}

static void set_data_dir(char *buf, char *suffix)
{
  char *s = getenv("PM2_ROOT");

  if(s)
    sprintf(buf, "%s/examples/rpc/data/%s_%s", s, mad_arch_name(), suffix);
  else
    sprintf(buf, "./%s_%s", mad_arch_name(), suffix);
}

static void startup_func(int argc, char *argv[], void *arg)
{
  autre = (pm2_self() == 0) ? 1 : 0;
}

int pm2_main(int argc, char **argv)
{
  int i;
  char name[256];

  pm2_rpc_init();

  DECLARE_LRPC(INFO);
  DECLARE_LRPC(LRPC_PING);
  DECLARE_LRPC(LRPC_PING_ASYNC);

  pm2_push_startup_func(startup_func, NULL);

  pm2_init(&argc, argv);

  if(pm2_self() == 0) { /* master process */

    tprintf("*** Performances des différents LRPC sous %s ***\n",
	    mad_arch_name());
#ifndef ONLY_ASYNC
#ifndef ONLY_QUICK
    set_data_dir(name, "lrpc");
    f1 = fopen(name, "w");
#endif
    set_data_dir(name, "quick_lrpc");
    f2 = fopen(name, "w");
#endif
#ifndef ONLY_QUICK
    set_data_dir(name, "async_lrpc");
    f3 = fopen(name, "w");
#endif

    if(argc > 1)
      nb = atoi(argv[1]);
    else {
      tprintf("Combien de rpc ? ");
      scanf("%d", &nb);
    }

    f(0); 
    f(512);
    for(i=1; i<=16; i++)
      f(i*1024);

#ifndef ONLY_ASYNC
#ifndef ONLY_QUICK
    fclose(f1);
#endif
    fclose(f2);
#endif
#ifndef ONLY_QUICK
    fclose(f3);
#endif

    pm2_halt();
   }

   pm2_exit();

   return 0;
}
