
/* timeout.c */

#include "marcel.h"

marcel_mutex_t mutex;
marcel_cond_t cond;

any_t f(any_t arg)
{
  struct timeval now;
  struct timespec ts;

  marcel_mutex_lock(&mutex);
  for(;;) {
    gettimeofday(&now, NULL);

    ts.tv_sec = now.tv_sec + 1; /* timeout d'une seconde */
    ts.tv_nsec = now.tv_usec * 1000;

    if(marcel_cond_timedwait(&cond, &mutex, &ts) != ETIMEDOUT)
      break;
    tprintf("What a boring job, isn't it ?\n");
  }
  marcel_mutex_unlock(&mutex);

  tprintf("What a relief !\n");

  return 0;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid;

  marcel_init(&argc, argv);

  marcel_mutex_init(&mutex, NULL);
  marcel_cond_init(&cond, NULL);

  marcel_create(&pid, NULL, f,  NULL);

  marcel_delay(3100);
  marcel_cond_signal(&cond);

  marcel_join(pid, &status);

  marcel_end();
  return 0;
}
