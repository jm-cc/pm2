#include <pm2.h>

static int service_id;		/* Here! */

static void			/* Here! */
service (void)			/* Here! */
{
  pm2_rawrpc_waitdata ();
  tprintf ("Hello, World!\n");
}

int
pm2_main (int argc, char *argv[])
{
  pm2_rawrpc_register (&service_id, service);	/* Here! */
  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {
      pm2_rawrpc_begin (1, service_id, NULL);	/* Here! */
      pm2_rawrpc_end ();	/* Here! */

      pm2_halt ();
    }

  pm2_exit ();
  return 0;
}
