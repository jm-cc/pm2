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

#ifdef __ACT__

#ifdef ACT_TIMER
#ifdef CONFIG_ACT_TIMER
#undef CONFIG_ACT_TIMER
#endif
#define CONFIG_ACT_TIMER
#include "timing.h"
#endif

#include "marcel.h"
#include "marcel_alloc.h"

#include <sched.h>
#include <stdlib.h>
#include <sys/act_spinlock.h>

#ifdef DEBUG
#include <stdio.h>
#endif

//#define SHOW_UPCALL

#define STACK_SIZE 10000

void restart_thread(marcel_t next)
{
	next->state_ext=MARCEL_RUNNING;
	longjmp(next->jb, NORMAL_RETURN);
}

static act_spinlock_t act_spinlock=ACT_SPIN_LOCK_UNLOCKED;

void act_lock(marcel_t self)
{
	ACTDEBUG(printf("act_lock (%p)\n", self));
	act_spin_lock(&act_spinlock);
}

void act_unlock(marcel_t self)
{
	ACTDEBUG(printf("act_unlock(%p)\n", self));
	act_spin_unlock(&act_spinlock);
}

void upcall_new(act_proc_t proc)
{
 	marcel_t next;

#ifdef SHOW_UPCALL
	printf("upcall_new(%i)\n", proc);
#else
	ACTDEBUG(printf("upcall_new(%i)\n", proc));
#endif

	//self=marcel_self();
	do {
		act_lock(NULL);
		
		next=cur_lwp->__first[0];
		if (next->state_ext != MARCEL_READY) {
			do {
				next=next->next;
			} while ( (next->state_ext != MARCEL_READY) && 
				  (next->next != cur_lwp->__first[0]));
			if (next->next == cur_lwp->__first[0])
				goto something_wrong;
		}
		
		//self->state_ext=MARCEL_READY;
		next->state_ext=MARCEL_RUNNING;
		ACTDEBUG(printf("upcall_new launch %p\n", next));  
		//restart_thread(act_update(next));
		restart_thread(next);
		
		/** Never there normally */
	something_wrong:
		act_unlock(NULL);
		
		mdebug("No new thread to launch !!!\n");
	} while (1);
 

#ifdef DEBUG
	printf("Error : come to end of act_new\n");
#endif
}

void upcall_unblock(act_proc_t interrupted_proc, 
		    act_proc_t unblocked_proc,
		    act_state_buf_t *interrupted_state,
		    act_state_buf_t *unblocked_state)
{
	/* cur = marcel thread unblocked */
	marcel_t cur=marcel_self();

#ifdef SHOW_UPCALL
	printf("act_unblock(%i, %i, %p, %p)...\n", 
	       interrupted_proc, unblocked_proc,
	       interrupted_state, unblocked_state);
#else
	ACTDEBUG(printf("act_unblock(%i, %i, %p, %p)...\n", 
			interrupted_proc, unblocked_proc,
			interrupted_state, unblocked_state));
#endif
	
	if(setjmp(cur->jb) == NORMAL_RETURN) {
#ifdef DEBUG
		breakpoint();
#endif
		MTRACE("Preemption is act_unblock", cur);
		ACTDEBUG(printf("act_unblock(%i, %i, %p, %p)... resuming blocked\n", 
				interrupted_proc, unblocked_proc,
				interrupted_state, unblocked_state));
		cur->state_ext=MARCEL_RUNNING;
		//unlock_task(); // Pas de unlock : On n'en avait pas pris.
		// Mais on libère qd même le verrou pris avant le longjmp
		act_unlock(cur);
		act_resume(unblocked_state, NULL, 0);
		//return;
	}
	ACTDEBUG(printf("act_unblock(%i, %i, %p, %p)... continuing\n", 
			interrupted_proc, unblocked_proc,
			interrupted_state, unblocked_state));
	act_resume(interrupted_state, &(cur->state_ext), 
		   ACT_RESTART_SLEEPING_ACT);
}

void upcall_change_proc(act_proc_t new_proc)
{
	printf("Tiens ! un upcall_change_proc(%i)\n", new_proc);
}

void init_upcalls(int nb_act)
{
	act_param_t param;
	void *stack;
  
	if (nb_act<=0)
		nb_act=0;

	ACTDEBUG(printf("init_upcalls(%i)\n", nb_act));
  
	stack=malloc(STACK_SIZE+512);
	param.nb_proc_wanted=nb_act;
	param.upcall_new_sp[0]=stack+STACK_SIZE;
	param.upcall_new=upcall_new;
	param.upcall_unblock=upcall_unblock;
	param.upcall_change_proc=upcall_change_proc;
	
	ACTDEBUG(printf("mem_base_sp=%p, sp=%p, new = %p, unbk=%p, chp=%p\n",
			stack, param.upcall_new_sp[0], 
			upcall_new, upcall_unblock, upcall_change_proc));
	
	act_cntl(ACT_CNTL_INIT, &param);
	
	ACTDEBUG(printf("Fin act_init\n"));
}


#endif /* __ACT__ */
