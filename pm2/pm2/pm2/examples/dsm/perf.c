
#include <stdio.h>
#include <stdlib.h>
#include "pm2.h"
#include "dsm_comm.h"
#include "tbx_timing.h"
/*
BEGIN_DSM_DATA
int c = 0;
END_DSM_DATA
*/
int it, req_counter, page_counter, me, other;
marcel_sem_t sem;
tbx_tick_t t1, t2, t3;
double t_req, t_page;


void startup_func(int argc, char *argv[], void *arg)
{
  me = pm2_self();
  other = (me == 0) ? 1 : 0;
  it = req_counter = page_counter = atoi(argv[1]);
  dsm_set_access(0, WRITE_ACCESS);
}


void read_server()
{
 if (req_counter--)
  dsm_send_page_req(other, 0, me, READ_ACCESS, 0);
else
  marcel_sem_V(&sem);
//fprintf(stderr,"rs\n");
}


void receive_page_server()
{
 if (page_counter--)
  dsm_send_page(other, 0, READ_ACCESS, 0);
else
  marcel_sem_V(&sem);
//fprintf(stderr,"rps\n");
}


int pm2_main(int argc, char **argv)
{
  int prot;

#ifdef PROFILE
  profile_activate(FUT_ENABLE, PM2_PROF_MASK | DSM_PROF_MASK);
#endif
  pm2_push_startup_func(startup_func, NULL);
  pm2_set_dsm_page_protection(DSM_UNIFORM_PROTECT, WRITE_ACCESS);
  pm2_set_dsm_page_distribution(DSM_CENTRALIZED, 1);

  prot = dsm_create_protocol(NULL,
			     NULL,
                             read_server,
                             NULL,
                             NULL,
                             receive_page_server,
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL 
                             );
  dsm_set_default_protocol(prot);
  marcel_sem_init(&sem, 0);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: perf <iterations> \n");
      exit(1);
    }


  if(pm2_self() == 0) { /* master process */

  TBX_GET_TICK(t1);

  read_server();
  marcel_sem_P(&sem);

  TBX_GET_TICK(t2);

  receive_page_server();
  marcel_sem_P(&sem);

  TBX_GET_TICK(t3);

  pm2_halt();

  t_req = TBX_TIMING_DELAY(t1, t2);
  t_page = TBX_TIMING_DELAY(t2, t3); 

  printf("Sending page request: %5.3f usecs\n", t_req/it/2);
  printf("Sending page: %5.3f usecs\n", t_page/it/2);
  }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
