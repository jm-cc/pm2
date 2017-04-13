/** mini ltask example with polling in memory
 */

#include <pioman.h>
#include <stdio.h>

static struct piom_ltask ltask;
static volatile int flag = 0;
static long long int count = 0;
static const int seconds = 5;

void*worker(void*dummy)
{
  sleep(seconds);
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

int main(int argc, char**argv)
{
  /* init pioman */
  pioman_init(&argc, argv);
  
  /* create and submit an ltask that polls into memory */
  piom_ltask_create(&ltask, &ltask_handler, NULL, PIOM_LTASK_OPTION_REPEAT);
  piom_ltask_submit(&ltask);

  /* create a thread that will trigger an event in 5 seconds. */
  piom_thread_t tid;
  piom_thread_create(&tid, NULL, &worker, NULL);

  /* wait for the ltask completion */
  fprintf(stderr, "waiting for ltask completion... (%d seconds)\n", seconds);
  piom_ltask_wait(&ltask);
  fprintf(stderr, "task completed after count = %lld iterations (%lld/s; %f usec/iter).\n",
	  count, count/seconds, 1000000.0 * (double)seconds/(double)count);

  piom_thread_join(tid);
  
  /* finalize pioman */
  pioman_exit();
  return 0;
}

