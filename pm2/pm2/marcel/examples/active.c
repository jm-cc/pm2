
/* active.c */

#include "marcel.h"

volatile boolean have_to_work = TRUE;

any_t sample(any_t arg)
{
   tprintf("thread will sleep during 2 seconds...\n");

   marcel_delay(2000);

   tprintf("thread will work a little bit...\n");

   while(have_to_work) /* active wait */ ;

   tprintf("thread termination\n");
   return NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t pid;

   marcel_init(&argc, argv);

   tprintf("active threads = %d\n", marcel_activethreads());

   tprintf("thread creation\n");

   marcel_create(&pid, NULL, sample, NULL);

   marcel_delay(500);

   tprintf("active threads = %d\n", marcel_activethreads());

   marcel_delay(1600);

   tprintf("active threads = %d\n", marcel_activethreads());

   have_to_work = FALSE;

   marcel_join(pid, &status);

   tprintf("thread termination catched\n");

   marcel_delay(10);

   tprintf("active threads = %d\n", marcel_activethreads());
   return 0;
}
