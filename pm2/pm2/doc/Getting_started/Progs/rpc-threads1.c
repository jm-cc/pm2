#include <stdio.h>
#include "pm2.h"

#define SIZE 32

int sample_id;

void
sample_thread (void *arg)
{
  char rec_msg[SIZE];
  char sent_msg[SIZE];

  int next = (pm2_self () + 1) % pm2_config_size ();

  pm2_unpack_byte (SEND_CHEAPER, RECV_CHEAPER, rec_msg, SIZE);
  pm2_rawrpc_waitdata ();

  if (pm2_self () == 0)
    {
      tprintf ("Received back string: %s\n", rec_msg);
      pm2_halt ();
    }
  else
    {
      sprintf (sent_msg, "%s %d", rec_msg, pm2_self ());
      tprintf ("Passing on string: %s\n", sent_msg);

      pm2_rawrpc_begin (next, sample_id, NULL);
      pm2_pack_byte (SEND_CHEAPER, RECV_CHEAPER, sent_msg, SIZE);
      pm2_rawrpc_end ();
    }
}

/********************* Cut here ********************/

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
      char msg[SIZE];
      strcpy (msg, "Init");

      tprintf ("Sending string: %s\n", msg);
      pm2_rawrpc_begin (1, sample_id, NULL);
      pm2_pack_byte (SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
      pm2_rawrpc_end ();
    }

  pm2_exit ();

  return 0;
}
