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

static unsigned SAMPLE, COMPLETED;
static marcel_sem_t sem;

#define NB	3

static char *mess[NB] = {
  "Hi Guys !",
  "Hi Girls !",
  "Hello world !"
};

static void SAMPLE_thread(void *arg)
{
  int i;
  char str[64];

  mad_unpack_str(MAD_IN_HEADER, str);

  pm2_rawrpc_waitdata();

  for(i=0; i<10; i++) {
    pm2_printf("%s (I am %p on LWP %d)\n",
	       str, marcel_self(), marcel_current_vp());
    marcel_delay(100);
  }

  pm2_rawrpc_begin(0, COMPLETED, NULL);
  pm2_rawrpc_end();
}

static void SAMPLE_service(void)
{
  pm2_thread_create(SAMPLE_thread, NULL);
}

static void COMPLETED_service(void)
{
  pm2_rawrpc_waitdata();

  marcel_sem_V(&sem);
}

int pm2_main(int argc, char **argv)
{
  int i;

  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);
  pm2_rawrpc_register(&COMPLETED, COMPLETED_service);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }


  if(pm2_self() == 0) { /* master process */

    marcel_sem_init(&sem, 0);

    for(i=0; i<NB; i++) {
      pm2_rawrpc_begin(1, SAMPLE, NULL);
      mad_pack_str(MAD_IN_HEADER, mess[i]);
      pm2_rawrpc_end();
    }

    for(i=0; i<NB; i++)
      marcel_sem_P(&sem);

    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
