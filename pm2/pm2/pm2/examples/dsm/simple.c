
#include <stdio.h>
#include "pm2.h"

#define N 20

int DSM_SERVICE;

dsm_mutex_t L;

pm2_completion_t c;

BEGIN_DSM_DATA
atomic_t a = {0};
DSM_NEWPAGE
int toto1 = 0;
DSM_NEWPAGE
int toto2 = 0;
DSM_NEWPAGE
int toto3 = 0;
DSM_NEWPAGE
int toto4 = 0;
DSM_NEWPAGE
int toto5 = 0;
DSM_NEWPAGE
int toto6 = 0;
DSM_NEWPAGE
int toto7 = 0;
DSM_NEWPAGE
int toto8 = 0;
DSM_NEWPAGE
int toto9 = 0;
END_DSM_DATA


void f()
{
  int i;

  dsm_mutex_lock(&L);
  tfprintf(stderr,"user thread ! (I am %p)\n", marcel_self());
  for (i = 0; i < N; i++) {
    toto1++;
  }
  tfprintf(stderr,"user thread finished!\n");
  dsm_mutex_unlock(&L);
  pm2_completion_signal(&c); 
}


void threaded_f()
{
  int i;
  pm2_completion_t my_c;

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &my_c);
  pm2_rawrpc_waitdata(); 

  dsm_mutex_lock(&L);
  tfprintf(stderr,"user thread ! (I am %p)\n", marcel_self());
  for (i = 0; i < N; i++) {
    toto1++;
  }  
  tfprintf(stderr,"user thread finished! \n");
  dsm_mutex_unlock(&L);
  pm2_completion_signal(&my_c); 
}


static void DSM_func(void)
{
  pm2_thread_create(threaded_f, NULL);
}


int pm2_main(int argc, char **argv)
{
  int i, j;

  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  dsm_set_default_protocol(MIGRATE_THREAD);
  //dsm_set_default_protocol(LI_HUDAK);

  pm2_set_dsm_page_distribution(DSM_CYCLIC, 16);

  dsm_mutex_init(&L, NULL);

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
