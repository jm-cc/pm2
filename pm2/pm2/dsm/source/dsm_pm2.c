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

#include <stdio.h>
#include <signal.h>
#include "dsm_page_manager.h"
#include "dsm_sysdep.h"
#include "dsm_protocol_policy.h"
#include "dsm_protocol_lib.h"
#include "marcel.h" /* tfprintf */
#include "dsm_pm2.h"

  //
  // dsm_pagefault - signal handling routine for page fault access (BUS/SEGV)
  //
static void dsm_pagefault_handler(int sig, void *addr, dsm_access_t access)
{
  unsigned long index = dsm_page_index(addr);
  sigset_t signals;
#define HYP_INSTRUMENT
#ifdef HYP_INSTRUMENT
  extern int dsm_pf_handler_calls;
  dsm_pf_handler_calls++;
#endif

#ifdef SIGSEGV_TRACE
  tfprintf(stderr, "Starting handler: sig = %d addr = %p index = %ld (I am %p)\n", sig, addr, index, marcel_self() );
#endif
      // Unblock SIGALARM to enable thread preemption and SIGSEGV to enable
      // concurrent page faults.
  
  sigemptyset(&signals);
  sigaddset(&signals, MARCEL_TIMER_SIGNAL);
  sigaddset(&signals, SIGSEGV);
  sigprocmask(SIG_UNBLOCK, &signals, NULL);

  dsm_lock_page(index);

  //call the apropriate handler 
 if (access == UNKNOWN_ACCESS)
   {
     if (dsm_get_access(index) == NO_ACCESS) // read fault
       {
#ifdef SIGSEGV_TRACE
	 tfprintf(stderr, "read fault: (I am %p)\n", marcel_self());
#endif
	 (*dsm_get_read_fault_action(dsm_get_page_protocol(index)))(index);
       }
     else if (dsm_get_access(index) == READ_ACCESS)// write fault
       {
#ifdef SIGSEGV_TRACE
	 tfprintf(stderr, "write fault: (I am %p)\n", marcel_self());
#endif
	 
	 (*dsm_get_write_fault_action(dsm_get_page_protocol(index)))(index);
       }
     // else nothing to do; pagefault processed in the meantime
   }
  else
    switch(access){
    case READ_ACCESS: (*dsm_get_read_fault_action(dsm_get_page_protocol(index)))(index);break;
    case WRITE_ACCESS:  (*dsm_get_write_fault_action(dsm_get_page_protocol(index)))(index);break;
    default: RAISE(PROGRAM_ERROR); break;
				     }
  dsm_unlock_page(index);
}


void dsm_pm2_init(int my_rank, int confsize)
{  
  dsm_init_protocol_table();
  dsm_page_table_init(my_rank, confsize);
  dsm_install_pagefault_handler((dsm_pagefault_handler_t)dsm_pagefault_handler);
}

void dsm_pm2_exit()
{
  dsm_uninstall_pagefault_handler();
}

