/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

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
