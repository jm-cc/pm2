#include <stdio.h>
#include <unistd.h>
#include "pm2.h"

char hostname[MAXHOSTNAMELEN];

void
service_thread (void *arg)
{
  int *p;
  int proc;

  p = (int *) malloc (sizeof (*p));

  *p = 1234;

  pm2_enable_migration ();
  proc = pm2_self ();

  tprintf ("First, I am on node %d, host %s...\n"
	   "p = %p, *p = %d\n", pm2_self (), hostname, p, *p);

  proc = (proc + 1) % pm2_config_size ();
  pm2_migrate_self (proc);

  tprintf ("Then, I am on node %d, host %s...\n"
	   "p = %p, *p = %d\n", pm2_self (), hostname, p, *p);

  free (p);

  pm2_halt ();
}


int
pm2_main (int argc, char *argv[])
{
  gethostname (hostname, MAXHOSTNAMELEN);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {				/* master process */
      pm2_thread_create (service_thread, NULL);
    }

  pm2_exit ();
  return 0;
}
