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

/* Options: SIGSEGV_TRACE */

//#define SIGSEGV_TRACE

#undef DSM_ALTSTACK

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#ifdef LINUX_SYS
#include <asm/sigcontext.h>
#endif

#include "dsm_sysdep.h"
#include "dsm_page_manager.h"

#include "dsm_const.h"
#include "isoaddr.h"

#include "slot_alloc.h"
#include "as_pool.h"

#include "marcel.h" /* for __next_thread */


#ifndef SA_ONSTACK
#if defined(LINUX_SYS) && defined(X86_ARCH)
#define SA_ONSTACK      0x08000000
#else
#error "SA_ONSTACK must be defined !"
#endif
#endif

#define DSM_HANDLER_OFFSET (DSM_PAGE_SIZE * 6-16)

//#define DEBUG0
//#define DEBUG1
//#define DEBUG2


dsm_pagefault_handler_t pagefault_handler;
dsm_std_handler_t dsm_secondary_SIGSEGV_handler = NULL;

/*  typedef void (*std_handler_t )(int); */

/*
 * Structures to save old sigaction info. Used at restore time.
 */
static struct sigaction bus_sigact, segv_sigact;


#ifdef DSM_SHARED_STACK
void* 
#else
void
#endif
_dsm_sig_handler(int sig, char *addr, dsm_access_t access) { 

#ifdef DSM_SHARED_STACK
#ifdef DSM_ALTSTACK
    void *as_ptr;
#endif
    sigset_t signals;

    void *base0;
    void **base;

    LOG_IN();

    TIMING_EVENT("In 2nd handler");

    base0 = (void*) (((unsigned long) marcel_get_cur_thread()) & ~(SLOT_SIZE-1));
    base  = (void**) slot_get_usable_address((slot_header_t *) base0);

#ifdef DEBUG2
    fprintf(stderr, "[%p] in secondary handler with arg = %d %p %d\n",
	    marcel_self(), sig, addr, access);
#endif

    sig = (int) base[0];
    addr = (char*)  base[1];
    access= (dsm_access_t)  base[2];

#ifdef DEBUG1
    fprintf(stderr, "[%p] in secondary (bis) handler with arg = %d %p %d\n",
	    marcel_self(), sig, addr, access);
#endif

#ifdef DSM_ALTSTACK
    /* Setup new altstack */
    as_ptr = dsm_as_setup_altstack();
#endif

#ifdef DEBUG2
  fprintf(stderr, "[%p] success in setting a new handler. sp=%lx\n",
					 marcel_self(),get_sp());
#endif

#endif

    TIMING_EVENT("In 2nd handler: before pagefault handler");
    (*pagefault_handler)(sig, addr, access);
    TIMING_EVENT("In 2nd handler: after pagefault handler");

#ifdef DSM_SHARED_STACK
    /* Remove preemption : previous sigmask will be restore when exiting from the handler */

    sigemptyset(&signals);
    sigaddset(&signals, MARCEL_TIMER_SIGNAL);
    sigprocmask(SIG_BLOCK, &signals, 0);

#ifdef DSM_ALTSTACK
    /* Mark the current stack as free */
#ifdef DEBUG2
    fprintf(stderr, "[%p] freeing some memory : %p \n",marcel_self(),as_ptr);
#endif
    dsm_as_free_stack(as_ptr);
#endif

#ifdef DEBUG2
   fprintf(stderr, "[%p] trying to return...\n",marcel_self());
#endif
   TIMING_EVENT("In 2nd handler: returing");
   
   LOG_OUT();

   return (void*) base[3];
#endif
}


static void pagefault_handler_helper(int sig, char *addr, dsm_access_t access) { 

   LOG_IN();

#ifndef DSM_SHARED_STACK
    _dsm_sig_handler(sig, addr, access);
#else
    jmp_buf buf;
    void *sp_addr;
    void *base0;
    void *base;


    TIMING_EVENT("In handler helper");
#ifdef DEBUG1
    tfprintf(stderr,"in handler helper : old thread = %p (SLOT_SIZE=%x)\n",
	 marcel_get_cur_thread(),SLOT_SIZE);
#endif
    base0 = (void*) (((unsigned long) marcel_get_cur_thread()) & ~(SLOT_SIZE-1));

    base = slot_get_usable_address((slot_header_t *) base0);

    ((void**) base)[0] = (void*)sig;
    ((void**) base)[1] = (void*)addr;
    ((void**) base)[2] = (void*)access;
    ((void**) base)[3] = (void*)&buf;

    sp_addr = base + DSM_HANDLER_OFFSET;

#ifdef DEBUG1
   tfprintf(stderr,"sp_addr %p, base %p, base0 %p,OFFSET %x\n",sp_addr,base, base0, DSM_HANDLER_OFFSET);
#endif

    if (setjmp(buf)==0) {
      jmp_buf* pbuf;

//      TIMING_EVENT("In handler helper: apres setjmp");
//      *((void**) sp_addr) = 0;
//      TIMING_EVENT("In handler helper: apres touch sp");

#ifdef DEBUG2
      fprintf(stderr, "jumping to stack %p with arg = %d %p %d\n",sp_addr,
	      sig, addr, access);
#endif
      set_sp(sp_addr);
      pbuf = _dsm_sig_handler(sig, addr, access);
#ifdef DEBUG2
      fprintf(stderr, "back in handler...%p %p\n",(void*)get_sp(),pbuf);
#endif
      longjmp(*pbuf,1);
    }

#ifdef DEBUG1
  fprintf(stderr, "back in handler helper...\n");
#endif

#endif

LOG_OUT();
}

#ifdef DSM_SHARED_STACK
static volatile int _dsm_handler_oqp=0;
#endif

#if defined(LINUX_SYS) && defined(X86_ARCH)
static void _internal_sig_handler(int sig, struct sigcontext_struct context)
#elif defined(SOLARIS_SYS) || defined(IRIX_SYS)
static void _internal_sig_handler(int sig, siginfo_t *siginfo , void *p)
#endif
{
  dsm_access_t access = UNKNOWN_ACCESS;
#if defined(LINUX_SYS) && defined(X86_ARCH)
  char *addr = (char *)(context.cr2);
  access = context.err & 2 ? WRITE_ACCESS : READ_ACCESS;
#elif defined(SOLARIS_SYS) || defined(IRIX_SYS)
  char *addr = siginfo->si_addr;
#else
#error DSM-PM2 is not yet implemented on that system! Sorry.
#endif


  LOG_IN();

#ifdef SIGSEGV_TRACE
  fprintf(stderr, "entering handler, sig = %d addr = %p (I am %p)\n", sig, addr, marcel_self());
#endif

#ifdef DSM_SHARED_STACK
	if (_dsm_handler_oqp) {
		fprintf(stderr,"---> ERROR : _dsm_handler_oqp = %d\n",
			_dsm_handler_oqp);
		exit(-1);
	}
	_dsm_handler_oqp++;
#endif
  if (dsm_static_addr(addr))
    {
      pagefault_handler_helper(sig, addr, access);
#ifdef DEBUG2
      fprintf(stderr, "exit from signal handler...0\n");
#endif
#ifdef DSM_SHARED_STACK
	_dsm_handler_oqp--;
#endif

      LOG_OUT();

      return;
    }

  if (isoaddr_addr(addr))
    if (isoaddr_page_get_status(isoaddr_page_index(addr)) == ISO_SHARED)
      {
#ifdef SIGSEGV_TRACE
	fprintf(stderr, "DSM page fault!\n");
#endif
	pagefault_handler_helper(sig, addr, access);
#ifdef DEBUG2
  fprintf(stderr, "exit from signal handler...1\n");
#endif
#ifdef DSM_SHARED_STACK
	_dsm_handler_oqp--;
#endif
 
        LOG_OUT();

	return;
      }
   { // regular seg fault
#ifdef SIGSEGV_TRACE
    fprintf(stderr, "Not a DSM page fault!\n");
#endif
    /*    for (i=0;i<22;i++)
	  fprintf(stderr,"%d %p \n", i, ((int *)&context)[i]);*/
    /* pjh */
    if (sig == SIGSEGV && dsm_secondary_SIGSEGV_handler != NULL)
    {
#ifdef SIGSEGV_TRACE
    fprintf(stderr, "Calling secondary sig handler...\n");
#endif
      (*dsm_secondary_SIGSEGV_handler)(sig);
    }
    else if ((sig == SIGBUS && bus_sigact.sa_handler == SIG_DFL) ||
	    (sig == SIGSEGV && segv_sigact.sa_handler == SIG_DFL))
      {
	struct sigaction act;

	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
#if defined(SOLARIS_SYS) || defined(IRIX_SYS)
	act.sa_flags = SA_SIGINFO | SA_RESTART;
#else
	act.sa_flags = SA_RESTART;
#endif
	sigaction(sig, &act, (struct sigaction *)NULL);
	kill(getpid(), sig);
      }
    else if (sig == SIGBUS && bus_sigact.sa_handler != SIG_IGN)
      {
#if defined(LINUX_SYS) && defined(X86_ARCH) 
	bus_sigact.sa_handler(sig);
#elif defined(SOLARIS_SYS) || defined(IRIX_SYS)
	bus_sigact.sa_handler(sig, siginfo, p);
#endif
      }
    else if (sig == SIGSEGV && segv_sigact.sa_handler != SIG_IGN)
      {
#if defined(LINUX_SYS) && defined(X86_ARCH) 
	segv_sigact.sa_handler(sig);
#elif defined(SOLARIS_SYS) || defined(IRIX_SYS)
	segv_sigact.sa_handler(sig, siginfo, p);
#endif
      }
  }
#ifdef DEBUG2
  fprintf(stderr, "exit from signal handler...\n");
#endif
#ifdef DSM_SHARED_STACK
	_dsm_handler_oqp--;
#endif

LOG_OUT();
}



/*
 * dsm_install_pagefault_handler - install page fault handlers.
 */

void dsm_install_pagefault_handler(dsm_pagefault_handler_t handler)
{
  struct sigaction act;

  LOG_IN();

#ifdef DSM_SHARED_STACK
  dsm_as_init();
#endif

  act.sa_handler = (void (*)(int))_internal_sig_handler;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, MARCEL_TIMER_SIGNAL);
#if defined(SOLARIS_SYS) || defined(IRIX_SYS)
  act.sa_flags = SA_SIGINFO | SA_RESTART;
#else
  act.sa_flags = SA_RESTART;
#endif

#ifdef DSM_SHARED_STACK
  act.sa_flags |= SA_ONSTACK;
#endif

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

  LOG_OUT();
}


/*
 * dsm_uninstall_pagefault_handler 
 * - remove page fault handler and restore the old one.
 */
void dsm_uninstall_pagefault_handler()
{
  LOG_IN();

  sigaction(SIGBUS, &bus_sigact, NULL);
  sigaction(SIGSEGV, &segv_sigact, NULL);
#ifdef DSM_SHARED_STACK
  dsm_as_clean();
#endif

  LOG_OUT();
}

void dsm_setup_secondary_SIGSEGV_handler(dsm_std_handler_t handler_func)
{
  LOG_IN();

  dsm_secondary_SIGSEGV_handler = handler_func;

  LOG_OUT();
}

void dsm_safe_ss(dsm_safe_ss_t h) {
  char t[DSM_PAGE_SIZE];
  (*h)(t);
}
