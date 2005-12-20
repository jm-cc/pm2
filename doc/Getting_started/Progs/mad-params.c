#include <pm2.h>

void sendData() {
  char s[] = "A la recherche du temps perdu.";
  int len;

  len = strlen (s) + 1;

  mad_begin_packing(...);
  mad_pack(..., &len, sizeof(int), mad_send_CHEAPER, mad_receive_EXPRESS);	/* Here! */
  mad_pack(..., s, len, mad_send_CHEAPER, mad_receive_CHEAPER);	/* Here! */
  mad_end_packing(...);
}

void receiveData() {
  int len;
  char *s;

  mad_begin_unpacking(...);
  mad_unpack(..., &len, sizeof(int), mad_send_CHEAPER, mad_receive_EXPRESS);	/* Here! */
  s = malloc(len);	/* Here! */
  mad_unpack(..., s, len, mad_send_CHEAPER, mad_receive_CHEAPER);	/* Here! */
  mad_end_unpacking(...);
}
