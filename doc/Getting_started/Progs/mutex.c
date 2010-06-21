#include "pm2.h"

#define N 100
#define NB_THREADS 4		// Should be even!

pm2_completion_t c;

volatile int counter = 0;

marcel_mutex_t mutex;

static void f(void *arg) {
  int i;
  int name = (int) arg;
  int forward = ((name % 2) == 0);
  volatile int tmp;

  for (i = 0; i < N; i++) {
    marcel_mutex_lock(&mutex);	/* Here! */

    tmp = counter;
    marcel_yield();		/* Here! */
    if (forward)
      tmp++;
    else
      tmp--;
    counter = tmp;

    marcel_mutex_unlock(&mutex);	/* Here! */
  }
  pm2_completion_signal(&c);
}

int pm2_main(int argc, char *argv[]) {
  int i;
  pm2_init(argc, argv);
  if ((pm2_self()) == 0) {
    pm2_completion_init(&c, NULL, NULL);
    tprintf("Initial value: %d\n", counter);

    marcel_mutex_init(&mutex, NULL);	/* Here! */

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
