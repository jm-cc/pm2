#include <stdio.h>
#include "pm2.h"

#define SIZE 32

int sample_id;
char msg[SIZE];

void
sample_thread (void *arg)
{
  pm2_completion_t c;

  pm2_unpack_byte (SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
  pm2_unpack_completion (SEND_CHEAPER, RECV_CHEAPER, &c);
  pm2_rawrpc_waitdata ();

  tprintf ("%s\n", msg);

  pm2_completion_signal (&c);
}

static void
sample_service (void)
{
  pm2_thread_create (sample_thread, NULL);
}

int
pm2_main (int argc, char *argv[])
{
  pm2_rawrpc_register (&sample_id, sample_service);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {

      pm2_completion_t c;
      pm2_completion_init (&c, NULL, NULL);

      strcpy (msg, "Hello world!");

      pm2_rawrpc_begin (1, sample_id, NULL);
      pm2_pack_byte (SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
      pm2_pack_completion (SEND_CHEAPER, RECV_CHEAPER, &c);
      pm2_rawrpc_end ();

      pm2_completion_wait (&c);

      pm2_halt ();		/* Correct!!! */
    }

  pm2_exit ();

  return 0;
}
