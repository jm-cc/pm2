#include "pm2.h"

static int service_id;
char hostname[MAXHOSTNAMELEN];

#define N 4
marcel_t threads[N];
int where = 1;

struct arg
{
  pm2_completion_t *cp;
  int i;
};

volatile int barrier = 0;

void
thread_function (void *arg)
{
  struct arg *argp;
  pm2_completion_t my_c;
  int i;

  argp = (struct arg *) arg;
  pm2_completion_copy (&my_c, argp->cp);
  i = argp->i;

  if (i % 2 == 0)
    pm2_enable_migration ();

  tprintf ("Thread %d: Created on node %d, host %s\n",
	   i, pm2_self (), hostname);
  pm2_completion_signal (&my_c);

  for (;;)
    {
      int tmp;
      tmp = barrier;
      if (tmp != 0)
	break;
      marcel_yield ();
    }

  tprintf ("Thread %d: Now on node %d, host %s\n",
	   i, pm2_self (), hostname);
}

/********************* Cut here ********************/
static void
service (void)
{
  pm2_rawrpc_waitdata ();
  tprintf ("Service activated on node %d, host %s\n",
	   pm2_self (), hostname);
  barrier = 1;
}

int
pm2_main (int argc, char *argv[])
{
  int i;
  int n;

  gethostname (hostname, MAXHOSTNAMELEN);
  pm2_rawrpc_register (&service_id, service);

  pm2_init (&argc, argv);

  tprintf ("Initialization completed on node %d\n", pm2_self ());

  if (pm2_self () == 0)
    {				/* master process */
      for (i = 0; i < N; i++)
	{
	  pm2_completion_t c;
	  struct arg arg;
	  pm2_completion_init (&c, NULL, NULL);
	  arg.cp = &c;
	  arg.i = i;
	  pm2_thread_create (thread_function, (void *) (&arg));
	  pm2_completion_wait (&c);
	}

      pm2_freeze ();
      pm2_threads_list (N, threads, &n, MIGRATABLE_ONLY);
      pm2_migrate_group (threads, n, where);
      tprintf ("%d threads among %d migrated off"
	       "to node %d\n", n, N, where);

      tprintf ("Issuing RPC to node %d\n", where);
      pm2_rawrpc_begin (where, service_id, NULL);
      pm2_rawrpc_end ();
      barrier = 1;

      tprintf ("Just halting\n");
      pm2_halt ();
    }
  pm2_exit ();
  return 0;
}
