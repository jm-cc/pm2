
/* timeout.c */

#include "marcel.h"

marcel_sem_t sem;

any_t f(any_t arg)
{
  LOOP(b)
    BEGIN
      marcel_sem_timed_P(&sem, 500);
      EXIT_LOOP(b);
    EXCEPTION
      WHEN(TIME_OUT)
        tprintf("What a boring job, isn't it ?\n");
    END
  END_LOOP(b)
  tprintf("What a relief !\n");

  return 0;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t pid;

  marcel_init(&argc, argv);

  marcel_sem_init(&sem, 0);

  marcel_create(&pid, NULL, f,  NULL);

  marcel_delay(2100);
  marcel_sem_V(&sem);

  marcel_join(pid, &status);

  marcel_end();
  return 0;
}
