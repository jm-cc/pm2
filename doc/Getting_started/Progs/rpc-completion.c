#include "pm2.h"

#define SIZE 32

static int service_id;
char msg[SIZE];

static void f(void *arg) {
  pm2_completion_t c;		/* Here! */

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);	/* Here! */
  pm2_rawrpc_waitdata();

  tprintf("%s\n", msg);
  pm2_completion_signal(&c);
}

static void service(void) {
  pm2_service_thread_create(f, NULL);
}

int pm2_main(int argc, char *argv[]) {
  pm2_rawrpc_register(&service_id, service);

  pm2_init(argc, argv);

  if (pm2_self() == 0) {
    pm2_completion_t c;	/* Here! */
    pm2_completion_init(&c, NULL, NULL);	/* Here! */

    strcpy(msg, "Hello world!");

    pm2_rawrpc_begin(1, service_id, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);	/* Here! */
    pm2_rawrpc_end();

    pm2_completion_wait(&c);	/* Here! */
    pm2_halt();		/* Correct!!! *//* Here! */
  }

  pm2_exit();

  return 0;
}
