
#include "pm2.h"

#define ESSAIS 5
#define N      1000

static unsigned SAMPLE, SAMPLE_THR;
static unsigned autre;

static marcel_sem_t sem;

static tbx_tick_t t1, t2;

static int glob_n;

// #define MORE_PACKS
// #define BUSY_NODES

#ifdef MORE_PACKS
static unsigned dummy;
#endif

void SAMPLE_service(void)
{
#ifdef MORE_PACKS
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
  pm2_rawrpc_waitdata();

  marcel_sem_V(&sem);
}

static void threaded_rpc(void *arg)
{
#ifdef MORE_PACKS
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
  pm2_rawrpc_waitdata();

  if(pm2_self() == 1) {
     pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
#ifdef MORE_PACKS
     pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
     pm2_rawrpc_end();
  } else {
    if(glob_n) {
      glob_n--;
      pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
#ifdef MORE_PACKS
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
      pm2_rawrpc_end();
    } else
      marcel_sem_V(&sem);
  }
}

void SAMPLE_THR_service(void)
{
  pm2_thread_create(threaded_rpc, NULL);
}

void f(void)
{
  int i, j;
  unsigned long temps;

  fprintf(stderr, "Non-threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    TBX_GET_TICK(t1);

    for(j=N/2; j; j--) {
      pm2_rawrpc_begin(autre, SAMPLE, NULL);
#ifdef MORE_PACKS
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
      pm2_rawrpc_end();

      marcel_sem_P(&sem);
    }

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }

  fprintf(stderr, "Threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    TBX_GET_TICK(t1);

    glob_n = N/2-1;
    pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
#ifdef MORE_PACKS
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
    pm2_rawrpc_end();

    marcel_sem_P(&sem);

    TBX_GET_TICK(t2);

    temps = TBX_TIMING_DELAY(t1, t2);
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }
}

void g(void)
{
  int i, j;

  for(i=0; i<ESSAIS; i++) {
    for(j=N/2; j; j--) {
      marcel_sem_P(&sem);

      pm2_rawrpc_begin(autre, SAMPLE, NULL);
#ifdef MORE_PACKS
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &dummy, 1);
#endif
      pm2_rawrpc_end();
    }
  }
}

static void startup_func(int argc, char *argv[], void *args)
{
  autre = (pm2_self() == 0) ? 1 : 0;
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);
  pm2_rawrpc_register(&SAMPLE_THR, SAMPLE_THR_service);

  pm2_push_startup_func(startup_func, NULL);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) { /* master process */

    marcel_sem_init(&sem, 0);

    f();

    pm2_halt();

  } else
    g();

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
