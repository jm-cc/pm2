
#include "pm2.h"

#define NB	2
static char *mess[NB] = { "Hi guys !", "Hi girls !" };

static marcel_sem_t sem;

void thread_func(void *arg)
{
  int i, proc;

  pm2_enable_migration();

  proc = 0;

  for(i=0; i<3*pm2_config_size(); i++) {
    pm2_printf("%s\n", (char *)arg);

    marcel_delay(100);
    if((i+1) % 3 == 0) {

      proc = (proc+1) % pm2_config_size();

      pm2_printf("I'm now going on node [%d]\n", proc);

      pm2_migrate_self(proc);

      pm2_printf("Here I am!\n");
    }
  }

  marcel_sem_V(&sem);
}

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_init(&argc, argv);

  if(pm2_self() == 0) { /* master process */

    marcel_sem_init(&sem, 0);

    for(i=0; i<NB; i++)
      pm2_thread_create(thread_func, mess[i]);

    for(i=0; i<NB; i++)
      marcel_sem_P(&sem);

    pm2_halt();
  }

   pm2_exit();

   tfprintf(stderr, "Main is ending\n");
   return 0;
}
