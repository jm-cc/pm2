#include "pm2.h"

#define NB_ITERATIONS 10000

int service_create_writer_id;
int service_create_reader_id;
int service_stop_reader_id;

pm2_completion_t c;

BEGIN_DSM_DATA int shvar = 0;
END_DSM_DATA void
writer (void *arg)
{
  int i;
  int my_node = pm2_self ();
  pm2_completion_t my_c;

  pm2_unpack_completion (SEND_CHEAPER, RECV_CHEAPER, &my_c);
  pm2_rawrpc_waitdata ();

  tprintf ("Writer starts on node %d\n", my_node);

  for (i = 0; i < NB_ITERATIONS; i++)
    shvar = i;

  tprintf ("Writer ends on node %d\n", my_node);
  pm2_completion_signal (&my_c);
}


static void
service_create_writer ()
{
  pm2_thread_create (writer, NULL);
}

int barrier = 0;

void
reader (void *arg)
{
  int my_node = pm2_self ();
  int initial, final;

  tprintf ("Reader starts on node %d with value %d\n", my_node, in);

  initial = shvar;
  while (barrier == 0)
    marcel_yield ();
  final = shvar;

  tprintf ("Reader ends on node %d: %d -> %d\n", my_node, initial, final);
}

static void
service_create_reader ()
{
  pm2_thread_create (reader, NULL);
}

static void
service_stop_reader ()
{
  i = 1;
}

int
pm2_main (int argc, char **argv)
{
  int i;
  int thread_id = 0;

  pm2_rawrpc_register (&service_create_writer_id, service_create_writer);
  pm2_rawrpc_register (&service_create_reader_id, service_create_reader);
  pm2_rawrpc_register (&service_stop_reader_id, service_stop_reader);

  dsm_set_default_protocol (LI_HUDAK);

  pm2_init (&argc, argv);

  if (pm2_self () == 0)
    {				/* master process */

      pm2_completion_init (&c, NULL, NULL);

      for (i = 1; i < NB_NODES; i++)
	{
	  thread_id++;
	  pm2_rawrpc_begin (i, service_id, NULL);
	  pm2_pack_int (SEND_CHEAPER, RECV_CHEAPER, &thread_id, 1);
	  pm2_pack_completion (SEND_CHEAPER, RECV_CHEAPER, &c);
	  pm2_rawrpc_end ();
	}

      for (i = 0; i < (NB_NODES - 1); i++)
	pm2_completion_wait (&c);

      tprintf ("shvar=%d\n", shvar);

      pm2_halt ();
    }

  pm2_exit ();

  tprintf ("Main is ending\n");
  return 0;
}
