#include <stdio.h>
#include <unistd.h>
#include "pm2.h"

char hostname[MAXHOSTNAMELEN];

static int service_id;

void
service_thread (void *arg)
{
  int proc;

  int x = 1234;

  pm2_rawrpc_waitdata ();

  pm2_enable_migration ();
  proc = pm2_self ();

  tprintf ("First, I am on node %d, host %s... &x = %p, x = %d\n",
	   pm2_self (), hostname, &x, x);

  proc = (proc + 1) % pm2_config_size ();
  pm2_migrate_self (proc);

  tprintf ("Now, I am on node %d, host %s... &x = %p, x = %d\n",
	   pm2_self (), hostname, &x, x);

  pm2_halt ();
}

void
service (void)
{
  pm2_thread_create (service_thread, NULL);
}

int
pm2_main (int argc, char *argv[])
{
  gethostname (hostname, MAXHOSTNAMELEN);
  pm2_rawrpc_register (&service_id, service);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {				/* master process */
      pm2_rawrpc_begin (1, service_id, NULL);
      pm2_rawrpc_end ();
    }

  pm2_exit ();
  return 0;
}
