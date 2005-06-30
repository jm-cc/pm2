#include "pm2.h"

#define NB_NODES (pm2_config_size())
#define NB_ITERATIONS 20

static int service_id;

dsm_mutex_t mutex;

BEGIN_DSM_DATA;
int shvar = 0;
END_DSM_DATA;

void
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

  tprintf ("Thread %d started on node %d\n", my_name, my_node);

  // dsm_mutex_lock (&mutex);           /* Here! */
  initial = shvar;
  for (i = 0; i < NB_ITERATIONS; i++)
    shvar++;
  final = shvar;
  // dsm_mutex_unlock (&mutex);         /* Here! */

  tprintf ("Thread %d from node %d finished on node %d: "
	   "from %d to %d!\n",
	   my_name, my_node, pm2_self (), initial, final);
  pm2_completion_signal (&my_c);
}


static void
service ()
{
  pm2_service_thread_create (f, NULL);
}

/********************* Cut here ********************/


int
pm2_main (int argc, char **argv)
{
  int i;
  int thread_id = 0;
  pm2_completion_t c;

  pm2_rawrpc_register (&service_id, service);
  pm2_completion_init (&c, NULL, NULL);

  dsm_set_default_protocol (LI_HUDAK);
  dsm_mutex_init (&mutex, NULL);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {				/* Master process */
      for (i = 1; i < NB_NODES; i++)
	{
	  thread_id++;
	  pm2_rawrpc_begin (i, service_id, NULL);
	  pm2_pack_int (SEND_CHEAPER, RECV_CHEAPER,
			&thread_id, 1);
	  pm2_pack_completion (SEND_CHEAPER, RECV_CHEAPER, &c);
	  pm2_rawrpc_end ();
	}

      for (i = 1; i < NB_NODES; i++)
	pm2_completion_wait (&c);

      tprintf ("shvar=%d\n", shvar);
      pm2_halt ();
    }
  pm2_exit ();
  return 0;
}
