#include "pm2.h"

char hostname[128];

static int service_id;

void f(void *arg) {
  int i, proc;
  pm2_completion_t my_c;

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &my_c);	/* Here! */
  pm2_rawrpc_waitdata();

  pm2_enable_migration();

  proc = pm2_self();
  for (i = 0; i < (2 * pm2_config_size() + 1); i++) {
    tprintf("Hop %d: I am on node %d, host %s...\n",
	    i, pm2_self(), hostname);

    proc = (proc + 1) % pm2_config_size();
    tprintf("Hop %d: Leaving to node %d\n", i, proc);

    pm2_migrate_self(proc);
  }
  pm2_completion_signal(&my_c);
}

void service(void) {
  pm2_service_thread_create(f, NULL);
}

int pm2_main(int argc, char *argv[]) {
  gethostname(hostname, 128);
  pm2_rawrpc_register(&service_id, service);

  pm2_init(argc, argv);

  if (pm2_self() == 0) {
    /* Master process */
    pm2_completion_t c;
    pm2_completion_init(&c, NULL, NULL);

    pm2_rawrpc_begin(1, service_id, NULL);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);	/* Here! */
    pm2_rawrpc_end();

    pm2_completion_wait(&c);
    pm2_halt();
  }

  pm2_exit();
  return 0;
}
