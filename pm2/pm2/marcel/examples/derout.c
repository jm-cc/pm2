
#include "marcel.h"
#include "marcel_sem.h"

marcel_sem_t sem;

void deviation1(void *arg)
{
   marcel_fprintf(stdout, "This is a deviation : %s\n", arg);
}

void deviation2(void *arg)
{
   marcel_fprintf(stdout, "Deviation of a blocked thread : %s\n", arg);
}

void deviation3(void *arg)
{
   marcel_fprintf(stdout, "Deviation in order to wake one's self : ");
   marcel_sem_V(&sem);
}

any_t func(any_t arg)
{ int i;

   for(i=0; i<10; i++) {
      marcel_delay(100);
      marcel_printf("Hi %s!\n", arg);
   }

   marcel_sem_V(&sem);

   marcel_printf("Will block...\n");
   marcel_sem_P(&sem);
   marcel_printf("OK !\n");

   return NULL;
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;
  any_t status;

   marcel_init(&argc, argv);

   marcel_sem_init(&sem, 0);

   marcel_create(&pid, NULL, func, (any_t)"boys");

   marcel_delay(500);

   marcel_deviate(pid, deviation1, "OK !");

   marcel_sem_P(&sem);

   marcel_delay(500);

   marcel_deviate(pid, deviation2, "OK !");

   marcel_delay(500);

   marcel_deviate(pid, deviation3, NULL);

   marcel_join(pid, &status);

   marcel_end();
   return 0;
}
