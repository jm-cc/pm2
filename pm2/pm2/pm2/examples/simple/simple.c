#include "pm2.h"

#define __ALIGNED__       __attribute__ ((aligned (sizeof(int))))

#define STRING_SIZE  16

static char msg[STRING_SIZE] __ALIGNED__;

static unsigned SAMPLE;

static void SAMPLE_service(void)
{
  char msg[STRING_SIZE];

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
  pm2_rawrpc_waitdata();

  fprintf(stderr,"%s\n", msg);

  pm2_halt();
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);

#ifdef PROFILE
  profile_activate(FUT_ENABLE, FUT_KEYMASKALL);
  //profile_activate(FUT_ENABLE, PM2_PROF_MASK | DSM_PROF_MASK);
#endif

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
      fprintf(stderr,
	      "This program requires at least two processes.\n"
	      "Please rerun pm2conf.\n");
      exit(1);
  }

  if(pm2_self() == 0) {
    strcpy(msg, "Hello world!   ");
    pm2_rawrpc_begin(1, SAMPLE, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
    pm2_rawrpc_end();
  }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

   fprintf(stderr, "Main ended\n");
  return 0;
}
