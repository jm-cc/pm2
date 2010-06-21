#include "pm2.h"

#define N 100
#define NB_THREADS 4		// Should be even!

pm2_completion_t c;
volatile int counter = 0;
marcel_mutex_t mutex;
marcel_sem_t sem;		/* Here! */

static void f(void *arg) {
  int i;
  int name = (int) arg;
  int forward = ((name % 2) == 0);
  volatile int tmp;

  for (i = 0; i < N; i++) {
    marcel_mutex_lock(&mutex);

    tmp = counter;
    marcel_yield();
    if (forward)
      tmp++;
    else
      tmp--;
    counter = tmp;

    marcel_mutex_unlock(&mutex);
  }

  marcel_sem_V(&sem);		/* Here! */
}

int pm2_main(int argc, char *argv[]) {
  int i;
  pm2_init(argc, argv);
  if ((pm2_self()) == 0) {
    marcel_sem_init(&sem, 0);	/* Here! */

    tprintf("Initial value: %d\n", counter);
    marcel_mutex_init(&mutex, NULL);

    for (i = 0; i < NB_THREADS; i++) {
      tprintf("Creating thread %d\n", i);
      pm2_thread_create(f, (void *) i);
    }

    for (i = 0; i < NB_THREADS; i++)
      marcel_sem_P(&sem);	/* Here! */

    tprintf("Final value: %d\n", counter);
    pm2_halt();
  }

  pm2_exit();
  return 0;
}
