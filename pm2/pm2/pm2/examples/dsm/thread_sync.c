
#include <stdio.h>
#include "pm2.h"

#include "pm2_sync.h"

#define N 10

BEGIN_DSM_DATA
int toto = 0;
END_DSM_DATA

pm2_thread_barrier_t b;

static void startup_func(int argc, char *argv[], void *arg)
{
  pm2_thread_barrier_attr_t attr;
  attr.local =  atoi(argv[1]);

  pm2_thread_barrier_init((pm2_thread_barrier_t *)arg, &attr);
}


void f()
{
  int i;

  for (i = 0; i < N ; i++)
    {
      fprintf(stderr,"[%p] i = %d before barrier\n", marcel_self(), i);
      pm2_thread_barrier(&b);
      fprintf(stderr,"[%p] i = %d after barrier\n", marcel_self(), i);
    }
}


int pm2_main(int argc, char **argv)
{
  int i;

  pm2_push_startup_func(startup_func, (void *)&b);

  pm2_init(&argc, argv);

  if (argc != 2)
    {
      fprintf(stderr, "Usage: threaded_sync <nombre de threads>\n");
      exit(1);
    }

  if ((pm2_config_size()) < 2)
    {
      fprintf(stderr, "This program requires at least 2 nodes\n");
      exit(1);
    }

  /* create local threads */
    for (i=0; i< atoi(argv[1]) ; i++)
	pm2_thread_create(f, NULL); 

    marcel_delay(3000);

    if (pm2_self() == 0)
      pm2_halt();

    pm2_exit();
  
    tfprintf(stderr, "Main is ending\n");
    return 0;
}

