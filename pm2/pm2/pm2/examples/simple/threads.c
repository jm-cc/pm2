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

static unsigned SAMPLE;

#define __ALIGNED__       __attribute__ ((aligned (sizeof(int))))

#define NB	3

#define STRING_SIZE  12

static char msg1[STRING_SIZE] __ALIGNED__ = "Hi Guys!   ";
static char msg2[STRING_SIZE] __ALIGNED__ = "Hi Girls!  ";
static char msg3[STRING_SIZE] __ALIGNED__ = "Hi world!  ";

static char *mess[NB];

static void SAMPLE_thread(void *arg)
{
  int i;
  char str[64];
  pm2_completion_t c;

  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, str, STRING_SIZE);
  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);

#ifdef PROFILE
  profile_activate(FUT_DISABLE, FUT_KEYMASKALL);
#endif

  pm2_rawrpc_waitdata();

  for(i=0; i<10; i++) {
    pm2_printf("%s (I am %p on LWP %d)\n",
	       str, marcel_self(), marcel_current_vp());
    marcel_delay(100);
  }

  pm2_completion_signal(&c);
}

static void SAMPLE_service(void)
{
#ifdef PROFILE
  profile_activate(FUT_ENABLE, FUT_KEYMASKALL);
#endif
  pm2_thread_create(SAMPLE_thread, NULL);
}

int pm2_main(int argc, char **argv)
{
  int i;

  mess[0] = msg1; mess[1] = msg2; mess[2] = msg3;

  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) { /* master process */
    pm2_completion_t c;

    pm2_completion_init(&c, NULL, NULL);

    for(i=0; i<NB; i++) {
      pm2_rawrpc_begin(1, SAMPLE, NULL);
      pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, mess[i], STRING_SIZE);
      pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER, &c);
      pm2_rawrpc_end();
    }

    for(i=0; i<NB; i++)
      pm2_completion_wait(&c);

    pm2_halt();
  }

  pm2_exit();

#ifdef PROFILE
  profile_stop();
#endif

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
