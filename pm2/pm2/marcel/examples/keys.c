
/* keys.c */

#include "marcel.h"

#define STACK_SIZE	10000

marcel_key_t key;

void imprime(void)
{
  int i;
  char *str = (char *)marcel_getspecific(key);

   for(i=0;i<10;i++) {
      printf(str);
      marcel_delay(50);
   }
}

any_t writer1(any_t arg)
{
   marcel_setspecific(key, (any_t)"Hi boys!\n");

   imprime();

   return NULL;
}

any_t writer2(any_t arg)
{
   marcel_setspecific(key, (any_t)"Hi girls!\n");

   imprime();

   return NULL;
}

int marcel_main(int argc, char *argv[])
{
  any_t status;
  marcel_t writer1_pid, writer2_pid;
  marcel_attr_t writer1_attr, writer2_attr;

   marcel_init(&argc, argv);

   marcel_attr_init(&writer1_attr);
   marcel_attr_init(&writer2_attr);

   marcel_attr_setstacksize(&writer1_attr, STACK_SIZE);
   marcel_attr_setstacksize(&writer2_attr, STACK_SIZE);

   marcel_key_create(&key, NULL);

   marcel_create(&writer2_pid, &writer2_attr, writer1, NULL);
   marcel_create(&writer1_pid, &writer1_attr, writer2, NULL);

   marcel_join(writer1_pid, &status);
   marcel_join(writer2_pid, &status);

   return 0;
}
