#include <pm2.h>

static int service_id;

static void
service (void)
{
  int len;
  char *s;

  pm2_unpack_int (SEND_CHEAPER, RECV_EXPRESS, &len, 1);	/* Here! */
  s = malloc (len);		/* Here! */
  pm2_unpack_byte (SEND_CHEAPER, RECV_CHEAPER, s, len);	/* Here! */
  pm2_rawrpc_waitdata ();

  tprintf ("The sentence is: %s\n", s);
}

int
pm2_main (int argc, char *argv[])
{
  pm2_rawrpc_register (&service_id, service);
  pm2_init (&argc, argv);
  if (pm2_self () == 0)
    {
/* Warning: this may not work on BIP and SCI *//* Here! */
      char s[] = "A la recherche du temps perdu.";
      int len;

      len = strlen (s) + 1;
      pm2_rawrpc_begin (1, service_id, NULL);
      pm2_pack_int (SEND_CHEAPER, RECV_EXPRESS, &len, 1);	/* Here! */
      pm2_pack_byte (SEND_CHEAPER, RECV_CHEAPER, s, len);	/* Here! */
      pm2_rawrpc_end ();

      pm2_halt ();
    }
  pm2_exit ();

  return 0;
}
