#include "pm2.h"

#define NB_THREADS_PER_NODE 3
#define NB_NODES (pm2_config_size())
#define NB_ITERATIONS 20

int service_id;

pm2_completion_t c;

dsm_mutex_t L;

BEGIN_DSM_DATA int shvar = 0;
END_DSM_DATA void
f (void *arg)
{
  int i;
  int my_name;
  int my_node = pm2_self ();
  pm2_completion_t my_c;
  int initial, final;

  pm2_unpack_int (SEND_CHEAPER, RECV_CHEAPER, &my_name, 1);
  pm2_unpack_completion (SEND_CHEAPER, RECV_CHEAPER, &my_c);
  pm2_rawrpc_waitdata ();

  tprintf ("Thread %d on node %d\n", my_name, my_node);

  dsm_mutex_lock (&L);
  initial = shvar;
  for (i = 0; i < NB_ITERATIONS; i++)
    shvar++;
  final = shvar;
  dsm_mutex_unlock (&L);

  tfprintf (stderr, "Thread %d from node %d finished on node %d: "
	    "from %d to %d!\n",
	    my_name, my_node, pm2_self (), initial, final);
  pm2_completion_signal (&my_c);
}


static void
service ()
{
  pm2_thread_create (f, NULL);
}

/********************* Cut here ********************/


int
pm2_main (int argc, char **argv)
{
  int i, j;
  int thread_id = 0;

  pm2_rawrpc_register (&service_id, service);

  dsm_set_default_protocol (MIGRATE_THREAD);
  // dsm_set_default_protocol(LI_HUDAK);

  dsm_mutex_init (&L, NULL);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {				/* master process */

      pm2_completion_init (&c, NULL, NULL);

      for (j = 1; j < NB_NODES; j++)
	for (i = 0; i < NB_THREADS_PER_NODE; i++)
	  {
	    thread_id++;
	    pm2_rawrpc_begin (j, service_id, NULL);
	    pm2_pack_int (SEND_CHEAPER, RECV_CHEAPER, &thread_id, 1);
	    pm2_pack_completion (SEND_CHEAPER, RECV_CHEAPER, &c);
	    pm2_rawrpc_end ();
	  }

      for (i = 0; i < (NB_THREADS_PER_NODE * (NB_NODES - 1)); i++)
	pm2_completion_wait (&c);

      tprintf ("shvar=%d\n", shvar);

      pm2_halt ();
    }

  pm2_exit ();

  tprintf ("Main is ending\n");
  return 0;
}
