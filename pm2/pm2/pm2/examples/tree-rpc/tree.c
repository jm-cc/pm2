
#include "rpc_defs.h"
#include <sys/time.h>


unsigned cur_proc = 0;

static __inline__ int next_proc(void)
{
  lock_task();

  do {
    cur_proc = (cur_proc+1) % pm2_config_size();
  } while(cur_proc == pm2_self());

  unlock_task();
  return cur_proc;
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
   LRP_CALL(next_proc(), DICHOTOMY, STD_PRIO, DEFAULT_STACK,
	    &req1, &res1, &att[0]);

   req2.inf = mid+1; req2.sup = req.sup;
   LRP_CALL(next_proc(), DICHOTOMY, STD_PRIO, DEFAULT_STACK,
	    &req2, &res2, &att[1]);

   for(i=0; i<2; i++)
     LRP_WAIT(&att[i]);

   res.res = res1.res + res2.res;
 }
END_SERVICE(DICHOTOMY)


static void f(void)
{
  LRPC_REQ(DICHOTOMY) req;
  LRPC_RES(DICHOTOMY) res;
  tbx_tick_t t1, t2;
  unsigned long temps;

  while(1) {
    tfprintf(stderr,
	     "Entrez un entier raisonnable (0 pour terminer) : ");
    scanf("%d", &req.sup);

    if(!req.sup)
      break;

    req.inf = 1;

    TBX_GET_TICK(t1);

    LRPC(pm2_self(), DICHOTOMY, STD_PRIO, DEFAULT_STACK, &req, &res);

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);

    tfprintf(stderr, "1+...+%d = %d\n", req.sup, res.res);
  }
}

int pm2_main(int argc, char **argv)
{
  pm2_rpc_init();

  DECLARE_LRPC_WITH_NAME(DICHOTOMY, "fac", OPTIMIZE_IF_LOCAL);

  pm2_init(&argc, argv);

  if(pm2_self() == 0) { /* master process */

    f();

    pm2_halt();
  }

  pm2_exit();

  return 0;
}
