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

#define ESSAIS 5

static unsigned SAMPLE, SAMPLE_THR;
static unsigned autre;

static marcel_sem_t sem;

static Tick t1, t2;

void SAMPLE_service(void)
{
  int n;

  old_mad_unpack_int(MAD_IN_HEADER, &n, 1);

  pm2_rawrpc_waitdata();

  if(--n) {
    pm2_rawrpc_begin(autre, SAMPLE, NULL);
    old_mad_pack_int(MAD_IN_HEADER, &n, 1);
    pm2_rawrpc_end();
  } else {
    marcel_sem_V(&sem);
  }
}

static void threaded_rpc(void *arg)
{
  int n;

  old_mad_unpack_int(MAD_IN_HEADER, &n, 1);
  pm2_rawrpc_waitdata();

  if(--n) {
     pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
     old_mad_pack_int(MAD_IN_HEADER, &n, 1);
     pm2_rawrpc_end();
   } else {
     marcel_sem_V(&sem);
   }
}

void SAMPLE_THR_service(void)
{
  pm2_thread_create(threaded_rpc, NULL);
}

void f(int n)
{
  int i;
  unsigned long temps;

  fprintf(stderr, "Non-threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    GET_TICK(t1);

    pm2_rawrpc_begin(autre, SAMPLE, NULL);
    old_mad_pack_int(MAD_IN_HEADER, &n, 1);
    pm2_rawrpc_end();

    marcel_sem_P(&sem);

    GET_TICK(t2);

    temps = timing_tick2usec(TICK_DIFF(t1, t2));
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }

  fprintf(stderr, "Threaded RPC:\n");
  for(i=0; i<ESSAIS; i++) {

    GET_TICK(t1);

    pm2_rawrpc_begin(autre, SAMPLE_THR, NULL);
    old_mad_pack_int(MAD_IN_HEADER, &n, 1);
    pm2_rawrpc_end();

    marcel_sem_P(&sem);

    GET_TICK(t2);

    temps = timing_tick2usec(TICK_DIFF(t1, t2));
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);
  }
}

static void startup_func()
{
  autre = (pm2_self() == 0) ? 1 : 0;
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&SAMPLE, SAMPLE_service);
  pm2_rawrpc_register(&SAMPLE_THR, SAMPLE_THR_service);

  pm2_set_startup_func(startup_func);

  pm2_init(&argc, argv);

  if(pm2_config_size() < 2) {
    fprintf(stderr,
	    "This program requires at least two processes.\n"
	    "Please rerun pm2conf.\n");
    exit(1);
  }

  if(pm2_self() == 0) { /* master process */

    marcel_sem_init(&sem, 0);

    f(1000);

    pm2_halt();

  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
