
#include "pm2.h"

static unsigned SAMPLE;

#define __ALIGNED__       __attribute__ ((aligned (sizeof(int))))

#define STRING_SIZE  16

static char msg[STRING_SIZE] __ALIGNED__;

void sample_thread(void *arg)
{
  pm2_completion_t c;

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata();

  pm2_printf("%s\n", msg);

  pm2_completion_signal(&c);
}

static void SAMPLE_service(void)
{
  pm2_thread_create(sample_thread, NULL);
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) {
    pm2_completion_t c;

    strcpy(msg, "Hello world!");

    pm2_completion_init(&c, NULL, NULL);

    pm2_rawrpc_begin(1, SAMPLE, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
    pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
    pm2_rawrpc_end();

    pm2_completion_wait(&c);

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
