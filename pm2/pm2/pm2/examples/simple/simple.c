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
#include "pm2.h"

#define __ALIGNED__       __attribute__ ((aligned (sizeof(int))))

#define STRING_SIZE  16

static char msg[STRING_SIZE] __ALIGNED__;

static unsigned SAMPLE;

static void SAMPLE_service(void)
{
  char msg[STRING_SIZE];

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
  pm2_rawrpc_waitdata();

  printf("%s\n", msg);

  pm2_halt();
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
      fprintf(stderr,
	      "This program requires at least two processes.\n"
	      "Please rerun pm2conf.\n");
      exit(1);
  }

  if(pm2_self() == 0) {
    strcpy(msg, "Hello world!   ");
    pm2_rawrpc_begin(1, SAMPLE, NULL);
    pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, msg, STRING_SIZE);
    pm2_rawrpc_end();
  }

  pm2_exit();
  return 0;
}
