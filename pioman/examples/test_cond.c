#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <pioman.h>
#include <tbx.h>

/* This program tests the behavior of piom_cond*
 * it performs a pingpong between 2 threads
 */
#define DEFAULT_LOOPS 10000

static unsigned loops = DEFAULT_LOOPS;

static void usage(void)
{
  fprintf(stderr, "-L loops - number of loops [%d]\n", DEFAULT_LOOPS);
}

static piom_cond_t cond1[DEFAULT_LOOPS], cond2[DEFAULT_LOOPS];

/* compute for usec microsecondes  */
static void work(unsigned usec)
{
  tbx_tick_t t1, t2;
  TBX_GET_TICK(t1);
  do
    {
      TBX_GET_TICK(t2);
    }
  while(TBX_TIMING_DELAY(t1, t2) < usec);
}

static void*th_func(void*arg)
{
  int i;
  for(i = 0 ; i < loops ; i++)
    {
      piom_cond_wait(&cond1[i], 1);
      /* as soon as the thread is unblocked, the condition may be re-used,
       *  for instance, we could call piom_cond_init.
       */
      memset(&cond1[i], 0x42, sizeof(cond1[i]));
    }
  printf("cond_wait ok\n");

  for(i = 0 ; i < loops ; i++)
    {
      while(!piom_cond_test(&cond2[i], 1))
	;
      memset(&cond2[i], 0x42, sizeof(cond2[i]));
    }
  printf("cond_test ok\n");
  return NULL;
}

int main(int argc, char**argv)
{
  tbx_init(&argc, &argv);
  pioman_init(&argc, argv);
  int i;
  
  if (argc > 1 && !strcmp(argv[1], "--help"))
    {
      usage();
      goto out;
    }
  for(i = 1 ; i < argc ; i += 2)
    {
      if (!strcmp(argv[i], "-L"))
	{
	  loops = atoi(argv[i+1]);
	}
      else
	{
	  fprintf(stderr, "Illegal argument %s\n", argv[i]);
	  usage();
	  goto out;
	}
    }
  for(i = 0 ; i < loops ; i++)
    {
      piom_cond_init(&cond1[i], 0);
      piom_cond_init(&cond2[i], 0);
    }
  
  piom_thread_t tid;
  piom_thread_create(&tid, NULL, th_func, NULL);
  
  for(i = 0 ; i < loops ; i++)
    {
      piom_cond_signal(&cond1[i], 1);
    }

  for(i = 0 ; i < loops ; i++)
    {
      piom_cond_signal(&cond2[i], 1);
    }

  piom_thread_join(tid);
 out:
  return 0;
}

