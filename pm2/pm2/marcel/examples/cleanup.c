
/* cleanup.c */

#include "marcel.h"

#define STACK_SIZE	10000

char *mess[2] = { "boys", "girls" };

void bye(any_t arg)
{
   tfprintf(stderr, "Bye bye %s!\n", arg);
}

any_t writer(any_t arg)
{ int i;

   marcel_cleanup_push(bye, arg);

   for(i=0;i<10;i++) {
      tfprintf(stderr, "Hi %s!\n", arg);
      marcel_delay(50);
   }
   return (any_t)NULL;
}

int marcel_main(int argc, char *argv[])
{ any_t status;
  marcel_t writer1_pid, writer2_pid;
  marcel_attr_t writer_attr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer_attr);

   marcel_attr_setstacksize(&writer_attr, STACK_SIZE);

   marcel_create(&writer2_pid, &writer_attr, writer, (any_t)mess[0]);
   marcel_create(&writer1_pid, &writer_attr, writer,  (any_t)mess[1]);

   marcel_join(writer1_pid, &status);
   marcel_join(writer2_pid, &status);

   marcel_end();
   return 0;
}
