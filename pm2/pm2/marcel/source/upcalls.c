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

#include "sys/marcel_flags.h"
#ifdef MA__ACTIVATION

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
#include <sys/upcalls.h>

#ifdef MA__DEBUG
#include <stdio.h>
#endif

//#define SHOW_UPCALL

#define STACK_SIZE 100000
#define ACT_NEW_WITH_LOCK 1

static marcel_t marcel_next[ACT_NB_MAX_CPU];

//volatile boolean has_new_tasks=0;
volatile int act_nb_unblocked=0;

void act_goto_next_task(marcel_t pid, int from)
{
	marcel_next[GET_LWP_NUMBER(marcel_self())]=pid;
	
	mdebug("\t\tcall to ACT_CNTL_RESTART_UNBLOCKED\n");
	act_cntl(ACT_CNTL_RESTART_UNBLOCKED, (void*)from);
	mdebug("\t\tcall to ACT_CNTL_RESTART_UNBLOCKED aborted\n");

	if (pid) {
		MA_THR_LONGJMP((pid), NORMAL_RETURN);
	}
}

void locked_start() {}

extern int hack_restart_func (act_proc_t new_proc, int return_value, 
		  int param, long eip, long esp) asm ("hack_restart_func");
extern void restart_func (act_proc_t new_proc, int return_value, 
		  int param, long eip, long esp) asm ("restart_func");
/*  void restart_func(act_proc_t new_proc, int return_value,  */
/*  		  int param, long eip, long esp); */

__asm__ (".align 16 \n\
	.globl restart_func \n\
	.type	 restart_func,@function \n\
restart_func: \n\
	addl $4, %esp \n\
	call hack_restart_func \n\
	movl 16(%esp), %esp \n\
	ret \n\
.Lmye: \n\
	.size	 restart_func,.Lmye-restart_func \n\
") ;

int hack_restart_func(act_proc_t new_proc, int return_value, 
		      int param, long eip, long esp)
{
	marcel_t current = marcel_self();
	
	SET_LWP(current, GET_LWP_BY_NUM(proc));
#ifdef SHOW_UPCALL
	marcel_printf("\trestart_func (proc %i, ret %i, param 0x%8x,"
		      " ip 0x%8x, sp 0x%8x)\n",
		      new_proc, return_value, param, eip, esp);
#else
	mdebug("\trestart_func (proc %i, ret %i, param 0x%8x,"
	       " ip 0x%8x, sp 0x%8x)\n",
	       new_proc, return_value, param, eip, esp);
#endif
	if (IS_TASK_TYPE_IDLE(current)) {
		if (param != ACT_RESCHEDULE) {
			marcel_printf("\trestart_func in idle task %p !! \n",
				      current);
		}
	}
  	if (param & ACT_UNBLK_RESTART_UNBLOCKED) {
		switch (param & 0xFF) {
		case ACT_RESTART_FROM_SCHED:
			MTRACE("Restarting", current);
			MA_THR_RESTARTED(current, "Syscall");
			SET_STATE_READY(marcel_next[GET_LWP_NUMBER(current)]);
			unlock_task();
			break;
		case ACT_RESTART_FROM_IDLE:
			/* we comme from the idle task */
			MTRACE("Restarting", current);
			mdebug("\t\tunchaining idle %p\n",
			       GET_LWP(current)->prev_running);
			SET_FROZEN(GET_LWP(current)->prev_running);
			UNCHAIN_TASK(GET_LWP(current)->prev_running);
			unlock_task();
			break;
		case ACT_RESTART_FROM_UPCALL_NEW:
			MTRACE("Restarting", current);
			break;
		default:
			RAISE("invalid parameter");
		}
  	} else if (param & ACT_UNBLK_IDLE) {
		if ((param & 0xFF) == ACT_NEW_WITH_LOCK) {
			mdebug("Ouf ouf ouf : On semble s'en sortir\n");
		}
		SET_FROZEN(GET_LWP(current)->prev_running);
		UNCHAIN_TASK(GET_LWP(current)->prev_running);
		MTRACE("Restarting", current);
		unlock_task();
	} else if (param & ACT_RESCHEDULE) {
		if(!locked() && preemption_enabled()) {
			MTRACE("ActSched", current);
			marcel_yield();
		}
	}
	
	*(&esp) = esp-4;  /* on veut empiler l'adresse de retour ret */
	*((int*)esp) = eip; /* C'est fait :-) */
	/* PS : Ça marche car le noyau descend suffisamment la pile :
	 * il y a une vingtaine d'octets entre esp et &esp
	 * */
	
	return return_value;
}

void act_lock(marcel_t self)
{
	//ACTDEBUG(printf("act_lock (%p)\n", self));
	//act_spin_lock(&act_spinlock);
}

void act_unlock(marcel_t self)
{
	//ACTDEBUG(printf("act_unlock(%p)\n", self));
	//act_spin_unlock(&act_spinlock);
}

void upcall_new(act_proc_t proc)
{
 	marcel_t next;
	//marcel_t new_task=GET_LWP_BY_NUM(proc)->upcall_new_task;

#ifdef SHOW_UPCALL
	marcel_printf("\tupcall_new(%i) : task_upcall=%p, lwp=%p (%i)\n",
		      proc, marcel_self(),GET_LWP_BY_NUM(proc), 
		      GET_LWP_BY_NUM(proc)->number );
#else
	mdebug("\tupcall_new(%i) : task_upcall=%p, lwp=%p (%i)\n",
	       proc, marcel_self(),GET_LWP_BY_NUM(proc), 
	       GET_LWP_BY_NUM(proc)->number );
#endif
	MTRACE("UpcallNew", marcel_self());
	SET_STATE_RUNNING(NULL, marcel_self(), GET_LWP_BY_NUM(proc));

	if (locked()) {
		mdebug("Kai kai kai : appel bloquant dans marcel !"
		       " (locked=%i)\n", locked());
		/* C'est peut-être juste un printf, on va essayer de s'en
		   sortir */
		for (;;) {
			mdebug("\t==> waiting end of blocking syscall\n");
			act_cntl(ACT_CNTL_READY_TO_WAIT,NULL);
			act_cntl(ACT_CNTL_DO_WAIT, (void*)ACT_NEW_WITH_LOCK);
		}
	}

	if (act_nb_unblocked) {
#ifdef SHOW_UPCALL
		marcel_printf("\t\tupcall_new(%i) : restart unblocked(%i)\n",
			      proc, act_nb_unblocked);
#endif
		act_cntl(ACT_CNTL_RESTART_UNBLOCKED, 
			 (void*)ACT_RESTART_FROM_UPCALL_NEW);
	}

	/* le lock_task n'est pas pris : il n'y a pas d'appels
	 * bloquant DANS marcel.
	 * */
	lock_task();
	//sched_lock(cur_lwp);
	
	next=marcel_radical_next_task();
	mdebug("\tupcall_new next=%p (state %i)\n", next, next->ext_state);
	
	//ACTDEBUG(printf("upcall_new launch %p\n", next));  

	/* On ne veut pas être mis en non_running */
	GET_LWP(self)->prev_running=NULL;
	MA_THR_LONGJMP(next, NORMAL_RETURN);
	

	/** Never there normally */	
	RAISE("Aie, aie aie !\n");
}

void locked_end() {}

static void init_act(int proc, act_param_t *param)
{
	void *stack;

	stack=(void*)GET_LWP_BY_NUM(proc)->upcall_new_task->initial_sp;
	param->upcall_new_sp[proc]=stack;
	param->locked[proc]=NULL; //TODO
	mdebug("upcall_new on proc %i use stack %p\n", proc, stack);

}

void init_upcalls(int nb_act)
{
	act_param_t param;
#ifdef MA__LWPS
	int proc;
#endif

	if (nb_act<=0)
		nb_act=0;

	//ACTDEBUG(printf("init_upcalls(%i)\n", nb_act));

#ifdef MA__LWPS
	for(proc=0; proc<ACT_NB_MAX_CPU; proc++) {
		/* WARNING : value 32 hardcoded : max of processors */
		init_act(proc, &param);
	}
#else
	init_act(0, &param);
#endif
	param.magic_number=ACT_MAGIC_NUMBER;
	param.nb_proc_wanted=nb_act;
	param.reset_count=0;
	/* param->upcall_new_sp[proc] */
	param.upcall_new=upcall_new;
	param.restart_func=restart_func;
	param.resched_func=restart_func;
	param.nb_act_unblocked=(int*)&act_nb_unblocked;
	param.locked_start=(unsigned long)&locked_start;
	param.locked_end=(unsigned long)&locked_end;
	/* param->locked[proc] */

//	param.upcall_unblock=upcall_unblock;
//	param.upcall_change_proc=upcall_change_proc;
	
	//ACTDEBUG(printf("sp=%p, new = %p, restart=%p, nb_unblk=%p\n",
	//		param.upcall_new_sp[0], 
	//		upcall_new, restart_func, &act_nb_unblocked));
	
       
	if (act_cntl(ACT_CNTL_INIT, &param)) {
		perror("Bad activations init");
		exit(1);
	}
	
	mdebug("Initialisation upcall done\n");
	//ACTDEBUG(printf("Fin act_init\n"));
	//scanf("%i", &proc);
}

#endif /* MA__ACTIVATION */
