/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 2.0

             Gabriel Antoniu, Luc Bouge, Christian Perez,
                Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 8512 CNRS-INRIA
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
#include <stdio.h>
#include <signal.h>

#define DEBUG

#define X86SOL2
#define MAX_THREADS 10

#ifdef LINUX
#include <asm/sigcontext.h>
#endif

#if !defined(SA_SIGINFO)
#define SA_SIGINFO 0
#endif


typedef void ( *dsm_pagefault_handler_t)(int sig, void *addr);

int *les_modules, nb_modules;


static void pagefault_handler(int sig, void *addr)
{
   unsigned nb;
   marcel_t pids[MAX_THREADS];

      tfprintf(stderr, "Starting handler: sig = %d addr = %p, I am %p \n", sig, addr, marcel_self() );
      pm2_freeze();
      pm2_threads_list(MAX_THREADS, pids, &nb, MIGRATABLE_ONLY);
      pm2_printf("migratable threads on module 0 : %d\n",nb);
      pm2_migrate(marcel_self(), les_modules[1]);
      pm2_printf("migration ended\n");
}


#ifdef LINUX
static void _internal_sig_handler(int sig, struct sigcontext_struct context)
{
#ifdef DEBUG
  fprintf(stderr, "entering handler, sig = %d addr = %p\n", sig, context.cr2);
#endif
  pagefault_handler(sig, (char *)context.cr2);
}
#elif defined(SUN4SOL2)|| defined(X86SOL2)
static void _internal_sig_handler(int sig, siginfo_t *siginfo , void *p)
{
#ifdef DEBUG
  fprintf(stderr, "entering handler, sig = %d, addr = %p\n", sig, siginfo->si_addr);
#endif
 pagefault_handler(sig, (char *)siginfo->si_addr);
}
#endif


void install_pagefault_handler(dsm_pagefault_handler_t handler)
{
  struct sigaction act;

  act.sa_handler = _internal_sig_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;

  if (sigaction(SIGBUS, &act, NULL) != 0)
    {
      fprintf(stderr, "ERROR: Could not install the handler for SIGBUS\n");
      exit(1);
    }

  if (sigaction(SIGSEGV, &act, NULL) != 0)
    {
      fprintf(stderr, "ERROR: Could not install the handler for SIGSEGV\n");
      exit(1);
    }
}

BEGIN_SERVICE(PAGE_FAULT)
     //  char *add = (char *)0xaffb0040; //X86SOL2
     char *add = (char *)0xdffc0040; //SUNMP

  install_pagefault_handler((dsm_pagefault_handler_t)_internal_sig_handler);
  pm2_enable_migration();
  marcel_delay(500);
  fprintf(stderr, "pagefault handler installed. I am %p and I'am going to cause a page fault\n", marcel_self());
  fprintf(stderr, "value = %d\n", *add);
  fprintf(stderr, "The End\n");
END_SERVICE(PAGE_FAULT)

int pm2_main(int argc, char **argv)
{
  pm2_init_rpc();
  DECLARE_LRPC(PAGE_FAULT);
  pm2_init(&argc, argv, 2, &les_modules, &nb_modules);

   if(pm2_self() == les_modules[0]) { /* master process */
      ASYNC_LRPC(les_modules[0], PAGE_FAULT, STD_PRIO, DEFAULT_STACK, NULL);
      marcel_delay(1000);
      pm2_kill_modules(les_modules, nb_modules);
   }
   else
     {
       int *ptr = (int *)pm2_isomalloc(100);
      
       ptr[0] = 7;
       pm2_printf("ptr = %p\n", ptr);
     }

   pm2_exit();

   tfprintf(stderr, "Main is ending\n");
   return 0;
}
