
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

  LOG_IN();

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

 LOG_OUT();
}


void dsm_pm2_init(int my_rank, int confsize)
{  
  LOG_IN();

  dsm_init_protocol_table();
  dsm_page_table_init(my_rank, confsize);
  dsm_install_pagefault_handler((dsm_pagefault_handler_t)dsm_pagefault_handler);

  LOG_OUT();
}

void dsm_pm2_exit()
{
  LOG_IN();

  dsm_uninstall_pagefault_handler();

  LOG_OUT();
}

