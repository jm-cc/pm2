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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#ifdef LINUX_SYS
#include <asm/sigcontext.h>
#endif

#include "dsm_sysdep.h"
#include "dsm_page_manager.h"

#include "dsm_const.h"

#if !defined(SA_SIGINFO)
#define SA_SIGINFO 0
#endif


//#define DEBUG

dsm_pagefault_handler_t pagefault_handler;
/*  typedef void (*std_handler_t )(int); */

/*
 * Structures to save old sigaction info. Used at restore time.
 */
static struct sigaction bus_sigact, segv_sigact;

#if defined(LINUX_SYS) && defined(X86_ARCH)
static void _internal_sig_handler(int sig, struct sigcontext_struct context)
#elif defined(SOLARIS_SYS)
static void _internal_sig_handler(int sig, siginfo_t *siginfo , void *p)
#endif
{
#if defined(LINUX_SYS) && defined(X86_ARCH)
#if __GLIBC__ == 2 && __GLIBC_MINOR__ == 1
  char *addr = (char *)(((int *)&context)[18]);
#else
  char *addr = (char *)(context.cr2);
#endif
#elif defined(SOLARIS_SYS)
  char *addr = siginfo->si_addr;
#endif
#ifdef DEBUG1
  fprintf(stderr, "entering handler, sig = %d addr = %p\n", sig, addr);
#endif
  // Test if DSM seg fault
  if (dsm_addr(addr))
    (*pagefault_handler)(sig, addr);
  else { // regular seg fault
    int i;
#ifdef DEBUG1
    fprintf(stderr, "Not a DSM page fault!\n");
#endif
    /*    for (i=0;i<22;i++)
	  fprintf(stderr,"%d %p \n", i, ((int *)&context)[i]);*/

    if ((sig == SIGBUS && bus_sigact.sa_handler == SIG_DFL) ||
	(sig == SIGSEGV && segv_sigact.sa_handler == SIG_DFL))
      {
	struct sigaction act;

	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, (struct sigaction *)NULL);
	kill(getpid(), sig);
      }
    else if (sig == SIGBUS && bus_sigact.sa_handler != SIG_IGN)
      {
#if defined(LINUX_SYS) && defined(X86_ARCH) 
	bus_sigact.sa_handler(sig);
#elif defined(SOLARIS_SYS)
	bus_sigact.sa_handler(sig, siginfo, p);
#endif
      }
    else if (sig == SIGSEGV && segv_sigact.sa_handler != SIG_IGN)
      {
#if defined(LINUX_SYS) && defined(X86_ARCH) 
	segv_sigact.sa_handler(sig);
#elif defined(SOLARIS_SYS)
	segv_sigact.sa_handler(sig, siginfo, p);
#endif
      }
  }
}



/*
 * dsm_install_pagefault_handler - install page fault handlers.
 */

void dsm_install_pagefault_handler(dsm_pagefault_handler_t handler)
{
  struct sigaction act;

  act.sa_handler = (void (*)(int))_internal_sig_handler;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, MARCEL_TIMER_SIGNAL);
  act.sa_flags = SA_SIGINFO;

  if (sigaction(SIGBUS, &act, &bus_sigact) != 0)
    {
      tfprintf(stderr, "ERROR: Could not install the handler for SIGBUS\n");
      exit(1);
    }

  if (sigaction(SIGSEGV, &act, &segv_sigact) != 0)
    {
      tfprintf(stderr, "ERROR: Could not install the handler for SIGSEGV\n");
      exit(1);
    }

  pagefault_handler = handler;
}


/*
 * dsm_uninstall_pagefault_handler 
 * - remove page fault handler and restore the old one.
 */
void dsm_uninstall_pagefault_handler()
{
  sigaction(SIGBUS, &bus_sigact, NULL);
  sigaction(SIGSEGV, &segv_sigact, NULL);
}
