
#include "madeleine.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define MODE            MAD_IN_PLACE

#define MAX_MESSAGE     (256*1024)
#define MAGIC_CHAR      'R'

static char buffer[MAX_MESSAGE] __MAD_ALIGNED__;

int main(int argc, char **argv)
{
  int i, les_modules[2], nb_modules, moi, autre, taille;

  mad_init(&argc, argv, 2, les_modules, &nb_modules, &moi);
  mad_buffers_init();

  if(argc > 1)
    taille = atoi(argv[1]);
  else {
    fprintf(stderr, "Usage: %s <msg_size>\n", argv[0]);
    exit(1);
  }

  if(moi == les_modules[0]) {

    autre = les_modules[1];

    printf("*** Simple message sending with Madeleine over %s ***\n", mad_arch_name());

    mad_receive();
    old_mad_unpack_int(MAD_IN_HEADER, &taille, 1);
    old_mad_unpack_byte(MODE, (char *)buffer, taille);
    mad_recvbuf_receive();

    printf("Message received : size = %d\n", taille);

    for(i=0; i<taille; i++)
      if(buffer[i] != MAGIC_CHAR) {
	fprintf(stderr, "Data corrupted at offet %d\n", i);
	exit(1);
      }

   } else {
     autre = les_modules[0];

     memset(buffer, MAGIC_CHAR, taille);

     mad_sendbuf_init(autre);
     old_mad_pack_int(MAD_IN_HEADER, &taille, 1);
     old_mad_pack_byte(MODE, (char *)buffer, taille);
     mad_sendbuf_send();

     printf("Message sent : size = %d\n", taille);

   }

   mad_exit();

   return 0;
}
