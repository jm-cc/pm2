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

#include <madeleine.h>

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
    mad_unpack_int(MAD_IN_HEADER, &taille, 1);
    mad_unpack_byte(MODE, (char *)buffer, taille);
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
     mad_pack_int(MAD_IN_HEADER, &taille, 1);
     mad_pack_byte(MODE, (char *)buffer, taille);
     mad_sendbuf_send();

     printf("Message sent : size = %d\n", taille);

   }

   mad_exit();

   return 0;
}
