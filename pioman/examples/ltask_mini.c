/** mini ltask example with polling in memory
 */

#include <pioman.h>
#include <marcel.h>
#include <stdio.h>

static struct piom_ltask ltask;
static volatile int flag = 0;

void*worker(void*dummy)
{
  marcel_sleep(3);
  flag = 1;
  return NULL;
}

int ltask_handler(void*arg)
{
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
  piom_ltask_create(&ltask, &ltask_handler, NULL, PIOM_LTASK_OPTION_REPEAT, piom_vpset_full);
  piom_ltask_submit(&ltask);

  /* create a thread that will generate an event in 3 seconds. */
  marcel_t tid;
  marcel_create(&tid, NULL, &worker, NULL);
  marcel_detach(tid);

  /* wait for the ltask completion */
  fprintf(stderr, "waiting for ltask completion...\n");
  piom_ltask_wait(&ltask);
  fprintf(stderr, "task completed.\n");

  /* finalize pioman */
  pioman_exit();
  return 0;
}

