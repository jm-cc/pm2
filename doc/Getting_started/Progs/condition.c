#include "pm2.h"

#define N 100
#define NB_THREADS 4		// Should be even!
#define MAX_COUNTER 2
#define MIN_COUNTER -2

pm2_completion_t c;
volatile int counter = 0;
marcel_mutex_t mutex;
marcel_sem_t sem;
marcel_cond_t forward_enabled, backward_enabled;	/* Here! */

static void f(void *arg) {
  int i;
  int name = (int) arg;
  int forward = ((name % 2) == 0);

  for (i = 0; i < N; i++) {
    marcel_mutex_lock(&mutex);

    if (forward) {
      while (counter >= MAX_COUNTER)
	marcel_cond_wait(&forward_enabled, &mutex);	/* Here! */
      counter++;
      tprintf("%d\t%d\n", counter, name);
      marcel_cond_signal(&backward_enabled);
    }
    else {
      while (counter <= MIN_COUNTER)
	marcel_cond_wait(&backward_enabled, &mutex);	/* Here! */
      counter--;
      tprintf("%d\t%d\n", counter, name);
      marcel_cond_signal(&forward_enabled);
    }

    marcel_mutex_unlock(&mutex);
  }
  marcel_sem_V(&sem);
}

int pm2_main(int argc, char *argv[]) {
  int i;
  pm2_init(argc, argv);
  if ((pm2_self()) == 0) {
    marcel_sem_init(&sem, 0);

    tprintf("Initial value: %d\n", counter);
    marcel_mutex_init(&mutex, NULL);
    marcel_cond_init(&forward_enabled, NULL);	/* Here! */
    marcel_cond_init(&backward_enabled, NULL);	/* Here! */

    for (i = 0; i < NB_THREADS; i++) {
      tprintf("Creating thread %d\n", i);
      pm2_thread_create(f, (void *) i);
    }

    for (i = 0; i < NB_THREADS; i++)
      marcel_sem_P(&sem);

    tprintf("Final value: %d\n", counter);
    pm2_halt();
  }

  pm2_exit();
  return 0;
}
