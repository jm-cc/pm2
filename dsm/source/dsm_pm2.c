
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/* Options: SIGSEGV_TRACE */
//#define SIGSEGV_TRACE

#include <stdio.h>
#include <signal.h>
#include "dsm_page_manager.h"
#include "dsm_comm.h"
#include "dsm_sysdep.h"
#include "dsm_protocol_policy.h"
#include "dsm_protocol_lib.h"
#include "marcel.h" /* tfprintf */
#include "dsm_pm2.h"
#include "hierarch_topology.h"
#include "token_lock.h"
#include "hierarch_protocol.h"


  //
  // dsm_pagefault - signal handling routine for page fault access (BUS/SEGV)
  //
static void dsm_pagefault_handler(int sig, void *addr, dsm_access_t access)
{
  dsm_page_index_t index = dsm_page_index(addr);
  sigset_t signals;
#ifdef INSTRUMENT
  extern int dsm_pf_handler_calls;
  dsm_pf_handler_calls++;
#endif

  LOG_IN();

#ifdef SIGSEGV_TRACE
  tfprintf(stderr, "Starting handler: sig = %d addr = %p index = %ld (I am %p)\n", sig, addr, index, marcel_self() );
#endif
      // Unblock SIGALARM to enable thread preemption and SIGSEGV to enable
      // concurrent page faults.
  
  sigemptyset(&signals);
  sigaddset(&signals, MARCEL_TIMER_SIGNAL);
  sigaddset(&signals, MARCEL_RESCHED_SIGNAL);
  sigaddset(&signals, SIGSEGV);
  sigprocmask(SIG_UNBLOCK, &signals, NULL);


  //call the apropriate handler 
 if (access == UNKNOWN_ACCESS)
   {
      dsm_access_t pg_access;

      dsm_lock_page(index);
      pg_access = dsm_get_access(index);
      dsm_unlock_page(index);

     if (pg_access == NO_ACCESS) // read fault
       {
#ifdef SIGSEGV_TRACE
	 tfprintf(stderr, "read fault: (I am %p)\n", marcel_self());
#endif
	 (*dsm_get_read_fault_action(dsm_get_page_protocol(index)))(index);
       }
     else if (pg_access == READ_ACCESS)// write fault
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
    default: MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR); break;
				     }

 LOG_OUT();
}

/*   GA 28/10/2002: Major change : dsm_pm2_init has been split into two */
/*   init functions: dsm_pm2_init_before_startup_funcs() and */
/*   dsm_pm2_init_after_startup_funcs()  */

#if 0
void dsm_pm2_init(int my_rank, int confsize)
{
   LOG_IN();

   dsm_init_protocol_table();
   dsm_page_table_init(my_rank, confsize);
   dsm_comm_init();
   dsm_install_pagefault_handler((dsm_pagefault_handler_t)dsm_pagefault_handler);
   token_lock_initialization();
   topology_initialization();

   LOG_OUT();
}
#endif


void dsm_pm2_exit()
{
  LOG_IN();

  /* the following function should be registered by
   * dsm_create_protocol() and dsm_pm2_exit() should call all the
   * registered protocol finalization functions */
  hierarch_proto_finalization();

  topology_finalization();
  token_lock_finalization();
  dsm_uninstall_pagefault_handler();

  LOG_OUT();
}

void dsm_pm2_init_before_startup_funcs(int my_rank, int confsize)
{
  LOG_IN();
  
  dsm_init_protocol_table();
  
  LOG_OUT();
}

void dsm_pm2_init_after_startup_funcs(int my_rank, int confsize)
{
  LOG_IN();
  
  dsm_page_table_init(my_rank, confsize);
  dsm_comm_init();
  dsm_install_pagefault_handler((dsm_pagefault_handler_t)dsm_pagefault_handler);
  token_lock_initialization();
  topology_initialization();

  LOG_OUT();
}

