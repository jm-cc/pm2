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

#include "rpc_defs.h"
#include <stdlib.h>
#include <sys/time.h>

#define ESSAIS 3

#define ONLY_ASYNC
/* #define ONLY_QUICK */

#define min(a, b) ((a) < (b) ? (a) : (b))

/* ****************** fonctions de mesure du temps ************** */

static struct timeval _t1, _t2;
static struct timezone _tz;
static unsigned long _temps_residuel = 0;

#define top1() gettimeofday(&_t1, &_tz)
#define top2() gettimeofday(&_t2, &_tz)

void init_cpu_time(void)
{
   top1(); top2();
   _temps_residuel = 1000000L * _t2.tv_sec + _t2.tv_usec - 
                     (1000000L * _t1.tv_sec + _t1.tv_usec );
}

unsigned long cpu_time(void) /* retourne des microsecondes */
{
   return 1000000L * _t2.tv_sec + _t2.tv_usec - 
           (1000000L * _t1.tv_sec + _t1.tv_usec ) - _temps_residuel;
}

/* ************************************************************** */

int *les_modules, nb_modules, autre;

static unsigned nb;
static marcel_sem_t fin_async_lrpc;

#ifndef ONLY_ASYNC
#ifndef ONLY_QUICK
static FILE *f1;
#endif
static FILE *f2;
#endif
#ifndef ONLY_QUICK
static FILE *f3;
#endif

BEGIN_SERVICE(INFO)
     /* voir stubs */
END_SERVICE(INFO)

BEGIN_SERVICE(LRPC_PING)
     /* voir stubs */
END_SERVICE(LRPC_PING)

BEGIN_SERVICE(LRPC_PING_ASYNC)
     if(NB_PING--)
       ASYNC_LRPC(autre, LRPC_PING_ASYNC, STD_PRIO, DEFAULT_STACK, NULL);
     else
       marcel_sem_V(&fin_async_lrpc);
END_SERVICE(LRPC_PING_ASYNC)

void informer(unsigned size, unsigned nbping)
{
  BUF_SIZE = size;
  NB_PING = nbping;

  QUICK_LRPC(autre, INFO, NULL, NULL);
}

void f(int bytes)
{
  unsigned long temps;
  unsigned n;
#ifndef ONLY_ASYNC
  unsigned i;
#endif

  if(bytes % 1024 == 0)
    tprintf("%d Koctets transférés :\n", bytes/1024);
  else
    tprintf("%d octets transférés :\n", bytes);

#ifndef ONLY_ASYNC

#ifndef ONLY_QUICK
    /************************* LRPC *************************/
    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {
      informer(bytes, nb/2);
      top1();
      for(i=0; i<nb/2; i++)
	LRPC(autre, LRPC_PING, STD_PRIO, DEFAULT_STACK,
	     NULL, NULL);
      top2();
      temps = min(cpu_time(), temps);
    }

    tfprintf(stderr, "temps LRPC = %d.%03dms\n",
	     temps/1000, temps%1000);
    fprintf(f1, "%d %ld\n", bytes, temps/1000);
#endif /* ONLY_QUICK */

    /************************ QUICK_LRPC ***********************/
    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {
      informer(bytes, nb/2);
      top1();
      for(i=0; i<nb/2; i++)
	QUICK_LRPC(autre, LRPC_PING, NULL, NULL);
      top2();
      temps = min(cpu_time(),temps);
    }

    tfprintf(stderr, "temps QUICK_LRPC = %d.%03dms\n",
	     temps/1000, temps%1000);
    fprintf(f2, "%d %ld\n", bytes, temps/1000);
#endif /* ONLY_ASYNC */

#ifndef ONLY_QUICK
    /********************** ASYNC_LRPC **********************/

    marcel_sem_init(&fin_async_lrpc, 0);
    temps = ~0L;
    for(n=0; n<ESSAIS; n++) {
      informer(bytes, nb/2);
      NB_PING--;
      top1();
      ASYNC_LRPC(autre, LRPC_PING_ASYNC, STD_PRIO, DEFAULT_STACK, NULL);
      marcel_sem_P(&fin_async_lrpc);
      top2();

      temps = min(cpu_time(), temps);
    }

    tfprintf(stderr, "temps ASYNC_LRPC = %ld.%03ldms\n",
	     temps/1000, temps%1000);
    fprintf(f3, "%d %ld\n", bytes, temps/1000);
#endif /* ifndef ONLY_QUICK */
}

static void set_data_dir(char *buf, char *suffix)
{
  char *s = getenv("PM2_ROOT");

  if(s)
    sprintf(buf, "%s/examples/simple/data/%s_%s", s, mad_arch_name(), suffix);
  else
    sprintf(buf, "./%s_%s", mad_arch_name(), suffix);
}

int pm2_main(int argc, char **argv)
{
  int i;
  char name[256];

   init_cpu_time();

   pm2_init_rpc();

   DECLARE_LRPC(INFO);
   DECLARE_LRPC(LRPC_PING);
   DECLARE_LRPC(LRPC_PING_ASYNC);

   pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

   if(pm2_self() == les_modules[0]) { /* master process */

     autre = les_modules[1];

     tprintf("*** Performances des différents LRPC sous %s ***\n", mad_arch_name());
#ifndef ONLY_ASYNC
#ifndef ONLY_QUICK
     set_data_dir(name, "lrpc");
     f1 = fopen(name, "w");
#endif
     set_data_dir(name, "quick_lrpc");
     f2 = fopen(name, "w");
#endif
#ifndef ONLY_QUICK
     set_data_dir(name, "async_lrpc");
     f3 = fopen(name, "w");
#endif

     if(argc > 1)
       nb = atoi(argv[1]);
     else {
       tprintf("Combien de rpc ? ");
       scanf("%d", &nb);
     }

     f(0); 
     f(512);
     for(i=1; i<=16; i++)
       f(i*1024);

#ifndef ONLY_ASYNC
#ifndef ONLY_QUICK
     fclose(f1);
#endif
     fclose(f2);
#endif
#ifndef ONLY_QUICK
     fclose(f3);
#endif

     pm2_kill_modules(les_modules, nb_modules);
   } else {
     autre = les_modules[0];
   }

   pm2_exit();

   return 0;
}
