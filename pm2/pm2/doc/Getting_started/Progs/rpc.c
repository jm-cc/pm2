#include <stdio.h>
#include <pm2.h>

static int service_id;

static void
service (void)
{
  pm2_rawrpc_waitdata ();
  tprintf ("Hello, World!\n");
}

int
pm2_main (int argc, char *argv[])
{
  pm2_rawrpc_register (&service_id, service);
  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {
      pm2_rawrpc_begin (1, service_id, NULL);
      pm2_rawrpc_end ();

      pm2_halt ();
    }

  pm2_exit ();
  return 0;
}
