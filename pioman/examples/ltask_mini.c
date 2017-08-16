/** mini ltask example with polling in memory
 */

#include <pioman.h>
#include <stdio.h>

static struct piom_ltask ltask;
static volatile int flag = 0;
static long long int count = 0;
static double seconds = 0.5;

void*worker(void*dummy)
{
  usleep(seconds * 1000.0 * 1000.0);
  flag = 1;
  return NULL;
}

int ltask_handler(void*arg)
{
  count++;
  if(flag)
    {
      fprintf(stderr, "poll success in handler!\n");
      piom_ltask_completed(&ltask);
    }
  return 0;
}

static void usage(char**argv)
{
  fprintf(stderr, "usage: %s [<s>]\n", argv[0]);
  fprintf(stderr, "  <s>  number of seconds to wait.\n");
}

int main(int argc, char**argv)
{
  /* init pioman */
  pioman_init(&argc, argv);

  if(argc == 2)
    {
      seconds = atof(argv[1]);
      if(seconds <= 0.0)
	{
	  usage(argv);
	  abort();
	}
    }
  else if(argc != 1)
    {
      usage(argv);
      exit(1);
    }
  
  /* create and submit an ltask that polls into memory */
  piom_ltask_create(&ltask, &ltask_handler, NULL, PIOM_LTASK_OPTION_REPEAT);
  piom_ltask_submit(&ltask);

  /* create a thread that will trigger an event in 5 seconds. */
  piom_thread_t tid;
  piom_thread_create(&tid, NULL, &worker, NULL);

  /* wait for the ltask completion */
  fprintf(stderr, "waiting for ltask completion... (%f seconds)\n", seconds);
  piom_ltask_wait(&ltask);
  fprintf(stderr, "task completed after count = %lld iterations (%lld/s; %f usec/iter).\n",
	  count, (long long int)(count/seconds), 1000000.0 * (double)seconds/(double)count);

  piom_thread_join(tid);
  
  /* finalize pioman */
  pioman_exit();
  return 0;
}

