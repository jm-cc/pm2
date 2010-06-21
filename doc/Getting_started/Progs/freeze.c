#include "pm2.h"

#define N 1000000
#define NB_THREADS 4		// Should be even!

pm2_completion_t c;

volatile int counter = 0;

static void f(void *arg) {
  int i;
  int name = (int) arg;
  int forward = ((name % 2) == 0);
  volatile int tmp;
  for (i = 0; i < N; i++) {
    pm2_freeze();		/* Here! */

    tmp = counter;
    if (forward)
      tmp++;
    else
      tmp--;
    counter = tmp;

    pm2_unfreeze();		/* Here! */
  }
  pm2_completion_signal(&c);
}

int pm2_main(int argc, char *argv[]) {
  int i;

  pm2_init(argc, argv);

  if ((pm2_self()) == 0) {
    pm2_completion_init(&c, NULL, NULL);

    tprintf("Initial value: %d\n", counter);

    for (i = 0; i < NB_THREADS; i++) {
      tprintf("Creating thread %d\n", i);
      pm2_thread_create(f, (void *) i);
    }

    for (i = 0; i < NB_THREADS; i++)
      pm2_completion_wait(&c);

    tprintf("Final value: %d\n", counter);

    pm2_halt();
  }

  pm2_exit();
  return 0;
}
