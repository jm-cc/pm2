
#include <stdio.h>

#include "pm2.h"



#define COUNTER 

int DSM_SERVICE;

dsm_mutex_t L;

pm2_completion_t c;

BEGIN_DSM_DATA
dsm_lock_t L2;
DSM_NEWPAGE
int toto1 = 0;
END_DSM_DATA


void f()
{
  int i, n = 100;

  dsm_lock(L2);
  for (i = 0; i < n; i++) {
    toto1++;
  }
  dsm_unlock(L2);
  pm2_completion_signal(&c); 
}


static void DSM_func(void)
{
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata(); 
  pm2_thread_create(f, NULL);
}


int pm2_main(int argc, char **argv)
{
  int i, j;

  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  //dsm_set_default_protocol(MIGRATE_THREAD);
  dsm_set_default_protocol(LI_HUDAK);

  pm2_set_dsm_page_distribution(DSM_BLOCK, 16);

  dsm_lock_init(&L2, NULL);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: simple <number of threads per node>\n");
      exit(1);
    }

  dsm_display_page_ownership();

  if(pm2_self() == 0) { /* master process */
    pm2_completion_init(&c, NULL, NULL);

    /* create local threads */
    for (i=0; i< atoi(argv[1]) ; i++)
      pm2_thread_create(f, NULL); 

    /* create remote threads */
    for (j=1; j < pm2_config_size(); j++)
      for (i=0; i< atoi(argv[1]) ; i++) {
	pm2_rawrpc_begin(j, DSM_SERVICE, NULL);
	pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
	pm2_rawrpc_end();
      }

    for (i = 0 ; i < atoi(argv[1]) * pm2_config_size(); i++)
      pm2_completion_wait(&c);

      tfprintf(stderr, "toto1=%d\n", toto1);
    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
