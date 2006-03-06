#include "pm2_common.h"

static int service_id;		/* Here! */

static void service (void) {			/* Here! */
  pm2_rawrpc_waitdata ();
  tprintf ("Hello, World!\n");
}

int pm2_main (int argc, char *argv[]) {
  pm2_rawrpc_register (&service_id, service);	/* Here! */

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  if (pm2_self () == 0) {
    pm2_rawrpc_begin (1, service_id, NULL);	/* Here! */
    pm2_rawrpc_end ();	/* Here! */

    pm2_halt ();
  }

  common_exit(NULL);
  exit(EXIT_SUCCESS);
}
