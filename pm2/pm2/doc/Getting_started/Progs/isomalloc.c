#include <stdio.h>
#include <unistd.h>
#include "pm2.h"

char hostname[MAXHOSTNAMELEN];

void
service_thread (void *arg)
{
  int **p, *q;
  int proc;

  p = (int **) pm2_isomalloc (sizeof (*p));
  q = (int *) pm2_isomalloc (sizeof (*p));

  *p = q;
  *q = 1234;

  pm2_enable_migration ();
  proc = pm2_self ();

  tprintf ("First, I am on node %d, host %s...\n"
	   "p = %p, *p = %p, **p = %d\n", pm2_self (), hostname, p, *p, **p);

  proc = (proc + 1) % pm2_config_size ();
  pm2_migrate_self (proc);

  tprintf ("Then, I am on node %d, host %s...\n"
	   "p = %p, *p = %p, **p = %d\n", pm2_self (), hostname, p, *p, **p);

  pm2_isofree (p);
  pm2_isofree (q);

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
