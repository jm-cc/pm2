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
#include <sys/time.h>

static unsigned WAKE, DICHO;

static unsigned cur_proc = 0;

static __inline__ int next_proc(void)
{
  lock_task();

  do {
    cur_proc = (cur_proc+1) % pm2_config_size();
  } while(cur_proc == pm2_self());

  unlock_task();
  return cur_proc;
}

static void wake(void)
{
  marcel_sem_t *ptr_sem;
  unsigned *ptr_res;

  mad_unpack_byte(MAD_IN_HEADER, (char *)&ptr_res, sizeof(ptr_res));
  mad_unpack_int(MAD_IN_HEADER, ptr_res, 1);
  mad_unpack_byte(MAD_IN_HEADER, (char *)&ptr_sem, sizeof(ptr_sem));

  pm2_rawrpc_waitdata();

  marcel_sem_V(ptr_sem);
}

static void thr_dicho(void *arg)
{
  unsigned inf, sup, res;
  marcel_sem_t *ptr_sem;
  unsigned *ptr_res;
  int father, self = pm2_self();

  mad_unpack_int(MAD_IN_HEADER, &inf, 1);
  mad_unpack_int(MAD_IN_HEADER, &sup, 1);
  mad_unpack_int(MAD_IN_HEADER, &father, 1);
  mad_unpack_byte(MAD_IN_HEADER, (char *)&ptr_res, sizeof(ptr_res));
  mad_unpack_byte(MAD_IN_HEADER, (char *)&ptr_sem, sizeof(ptr_sem));

  pm2_rawrpc_waitdata();

   if(inf == sup) {
      res = inf;
   } else {
      int mid = (inf + sup)/2;
      unsigned res1, *ptr_res1 = &res1, res2, *ptr_res2 = &res2;
      marcel_sem_t sem, *ptr_sem = &sem;

      marcel_sem_init(&sem, 0);

      pm2_rawrpc_begin(next_proc(), DICHO, NULL);
      mad_pack_int(MAD_IN_HEADER, &inf, 1);
      mad_pack_int(MAD_IN_HEADER, &mid, 1);
      mad_pack_int(MAD_IN_HEADER, &self, 1);
      mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_res1, sizeof(ptr_res1));
      mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_sem, sizeof(ptr_sem));
      pm2_rawrpc_end();

      mid++;

      pm2_rawrpc_begin(next_proc(), DICHO, NULL);
      mad_pack_int(MAD_IN_HEADER, &mid, 1);
      mad_pack_int(MAD_IN_HEADER, &sup, 1);
      mad_pack_int(MAD_IN_HEADER, &self, 1);
      mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_res2, sizeof(ptr_res2));
      mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_sem, sizeof(ptr_sem));
      pm2_rawrpc_end();

      marcel_sem_P(&sem); marcel_sem_P(&sem);

      res = res1 + res2;
   }

   pm2_rawrpc_begin(father, WAKE, NULL);
   mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_res, sizeof(ptr_res));
   mad_pack_int(MAD_IN_HEADER, &res, 1);
   mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_sem, sizeof(ptr_sem));
   pm2_rawrpc_end();
}

static void dicho(void)
{
  pm2_thread_create(thr_dicho, NULL);
}

static void f(void)
{
  Tick t1, t2;
  unsigned long temps;
  unsigned inf, sup, res, *ptr_res = &res;
  marcel_sem_t sem, *ptr_sem = &sem;
  int self = pm2_self();

  while(1) {

    marcel_sem_init(&sem, 0);

    tfprintf(stderr, "Entrez un entier raisonnable "
	     "(0 pour terminer) : ");
    scanf("%d", &sup);

    if(!sup)
      break;

    inf = 1;

    GET_TICK(t1);

    pm2_rawrpc_begin(next_proc(), DICHO, NULL);
    mad_pack_int(MAD_IN_HEADER, &inf, 1);
    mad_pack_int(MAD_IN_HEADER, &sup, 1);
    mad_pack_int(MAD_IN_HEADER, &self, 1);
    mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_res, sizeof(ptr_res));
    mad_pack_byte(MAD_IN_HEADER, (char *)&ptr_sem, sizeof(ptr_sem));
    pm2_rawrpc_end();

    marcel_sem_P(ptr_sem);

    GET_TICK(t2);

    temps = timing_tick2usec(TICK_DIFF(t1, t2));
    fprintf(stderr, "temps = %ld.%03ldms\n", temps/1000, temps%1000);

    tfprintf(stderr, "1+...+%d = %d\n", sup, res);
  }
}

int pm2_main(int argc, char **argv)
{
  pm2_rawrpc_register(&WAKE, wake);
  pm2_rawrpc_register(&DICHO, dicho);

  pm2_init(&argc, argv);

  if(pm2_self() == 0) { /* master process */

    if(pm2_config_size() < 2) {
      fprintf(stderr,
	      "This program requires at least two processes.\n"
	      "Please rerun pm2conf.\n");
      exit(1);
    }

    f();

    pm2_halt();
  }

  pm2_exit();

  return 0;
}
