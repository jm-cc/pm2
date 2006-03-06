#include "pm2_common.h"

static int service_id;

#define SIZE 1024

static void f(void *arg) {
  char rec_msg[SIZE];
  char sent_msg[SIZE];

  int next = (pm2_self() + 1) % pm2_config_size();

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, rec_msg, SIZE);	/* Here! */
  pm2_rawrpc_waitdata();	/* Here! */

  if (pm2_self() == 0) {
    tprintf("Received back string: %s\n", rec_msg);
    pm2_halt();
  }
  else {
    sprintf(sent_msg, "%s %d", rec_msg, pm2_self ());
    tprintf("Passing on string: %s\n", sent_msg);

    pm2_rawrpc_begin(next, service_id, NULL);	/* Here! */
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, sent_msg, SIZE);	/* Here! */
    pm2_rawrpc_end();	/* Here! */
  }
}

static void service(void) {
  pm2_service_thread_create(f, NULL);
}

int pm2_main(int argc, char *argv[]) {
  pm2_rawrpc_register(&service_id, service);
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  if (pm2_self() == 0) {
    char msg[SIZE];
    strcpy(msg, "Init");

    tprintf("Sending string: %s\n", msg);
    pm2_rawrpc_begin(1, service_id, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
    pm2_rawrpc_end();
  }

  common_exit(NULL);
  exit(EXIT_SUCCESS);
}
