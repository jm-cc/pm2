
#include "marcel.h"

exception USER_ERROR = "USER_ERROR";

void deviation(void *arg)
{
   RAISE(USER_ERROR);
}

any_t func(any_t arg)
{ int i;

   BEGIN
     for(i=0; i<10; i++) {
       marcel_delay(100);
       tprintf("Hi %s!\n", arg);
     }
   EXCEPTION
     WHEN(USER_ERROR)
       tprintf("God! I have been disturbed!\n");
       return NULL;
   END
   tprintf("Ok, I was not disturbed.\n");
   return NULL;
}

int marcel_main(int argc, char *argv[])
{ marcel_t pid;
  any_t status;

   marcel_init(&argc, argv);

   marcel_create(&pid, NULL, func, (any_t)"boys");

   marcel_delay(500);
   marcel_deviate(pid, deviation, NULL);

   marcel_join(pid, &status);

   return 0;
}

