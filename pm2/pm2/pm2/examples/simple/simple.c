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

#include <pm2.h>

static unsigned SAMPLE;

static void SAMPLE_service(void)
{
  char msg[1024];
  pm2_completion_t c;

  mad_unpack_str(MAD_IN_HEADER, msg);
  pm2_completion_unpack(&c);
  pm2_rawrpc_waitdata();

  pm2_printf("%s\n", msg);

  pm2_completion_signal(&c);
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

  if(pm2_self() == 0) { /* first process */
    pm2_completion_t c;

    pm2_completion_init(&c);

    pm2_rawrpc_begin(1, SAMPLE, NULL);
    mad_pack_str(MAD_IN_HEADER, "Hello world!");
    pm2_completion_pack(&c);
    pm2_rawrpc_end();

    pm2_completion_wait(&c);

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
