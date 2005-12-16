#include "pm2.h"

static int service_id;

#define SIZE 1024
char msg[SIZE];

static void
f (void *arg)
{
  pm2_unpack_byte (SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
  pm2_rawrpc_waitdata ();

  tprintf ("%s\n", msg);

  pm2_halt ();
}

static void
service (void)
{
  pm2_service_thread_create (f, NULL);	/* Here! */
}

int
pm2_main (int argc, char *argv[])
{
  pm2_rawrpc_register (&service_id, service);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {
      strcpy (msg, "Hello world!");

      pm2_rawrpc_begin (1, service_id, NULL);
      pm2_pack_byte (SEND_CHEAPER, RECV_CHEAPER, msg, SIZE);
      pm2_rawrpc_end ();

      /* pm2_halt(); Incorrect!!! */
    }

  pm2_exit ();

  return 0;
}
