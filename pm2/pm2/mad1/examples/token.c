
#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PACK_MODE   MAD_IN_PLACE
#define TAG_NUMBER   0xdeadbeef

static int nids[MAX_MODULES], configsize, myself, myrank;
static char message[512] __MAD_ALIGNED__;

void circulate_token()
{
  unsigned tag = TAG_NUMBER;
  static char tmp[32] __MAD_ALIGNED__;

  sprintf(tmp, "%d-", myrank);

  if(myrank == 0) { /* Create the token */

    mad_sendbuf_init(nids[(myrank+1) % configsize]);
    old_mad_pack_int(MAD_IN_HEADER, &tag, 1);
    old_mad_pack_str(PACK_MODE, tmp);

    mad_sendbuf_send();
  }

  mad_receive(); /* Receive the token */
  old_mad_unpack_int(MAD_IN_HEADER, &tag, 1);
  if(tag != TAG_NUMBER) {
    fprintf(stderr, "Tag error !!!\n");
    mad_exit(); exit(1);
  }
  old_mad_unpack_str(PACK_MODE, message);
  mad_recvbuf_receive();

  if(myrank != 0) { /*  Forward the token */

    strcat(message, tmp);

    mad_sendbuf_init(nids[(myrank+1) % configsize]);
    old_mad_pack_int(MAD_IN_HEADER, &tag, 1);
    old_mad_pack_str(PACK_MODE, message);

    mad_sendbuf_send();
  } else {
    printf("At the end, the token contains the string : %s\n", message);
  }
}

int main(int argc, char **argv)
{
  
  mad_init(&argc, argv, NETWORK_ASK_USER, nids, &configsize, &myself);
  mad_buffers_init();

  myrank = 0;
  while(nids[myrank] != myself)
    myrank++;

  printf("Hi, I'm the node of rank %d (config. size = %d)\n", myrank, configsize);

  circulate_token();

  mad_exit();
  return 0;
}
