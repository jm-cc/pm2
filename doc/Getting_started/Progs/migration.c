#include "pm2.h"

char hostname[128];

void f(void *arg) {
  int i, proc;

  pm2_enable_migration();

  proc = pm2_self();
  for (i = 0; i < (2 * pm2_config_size() + 1); i++) {
    tprintf("Hop %d: I am on node %d, host %s...\n",
	    i, pm2_self(), hostname);

    proc = (proc + 1) % pm2_config_size();
    tprintf("Hop %d: Leaving to node %d\n", i, proc);

    pm2_migrate_self(proc);	/* Here! */
  }
  pm2_halt();			/* Here! */
}

int pm2_main(int argc, char *argv[]) {
  gethostname(hostname, 128);

  pm2_init(argc, argv);

  if (pm2_self() == 0) {
    /* Master process */
    pm2_thread_create(f, NULL);
  }

  pm2_exit();
  return 0;
}
