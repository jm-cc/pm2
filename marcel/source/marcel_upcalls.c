
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

#define MA_FILE_DEBUG upcall
#include "marcel.h"

#ifdef MA__ACTIVATION

#include <sched.h>
#include <stdlib.h>

#define SHOW_UPCALL

#define ACT_NEW_WITH_LOCK 1

marcel_t marcel_next[ACT_NB_MAX_CPU];
//volatile tbx_bool_t has_new_tasks=0;
volatile int act_nb_unblocked=0;


#define DEFINE_UPCALL(type) \
  extern int upcall_##type(act_proc_t new_proc, act_id_t old, \
		           act_id_t restarted, act_event_t event, \
		           act_register_placeholder_t regs) \
  asm ("upcall_" #type); \
  extern void stub_upcall_##type(act_proc_t new_proc, act_id_t old, \
			        act_id_t restarted, act_event_t event, \
			        act_register_placeholder_t regs) \
  asm ("stub_upcall_"#type); \
  __asm__ (".text \n\
        .align 16 \n\
.globl stub_upcall_"#type" \n\
        .type    stub_upcall_"#type", @function \n\
stub_upcall_"#type": \n\
        addl $4, %esp \n\
        movl %ebx, 40(%esp) \n\
        movl %ecx, 44(%esp) \n\
        movl %edx, 48(%esp) \n\
        call upcall_"#type" \n\
        movl 48(%esp), %edx \n\
        movl 44(%esp), %ecx \n\
        movl 40(%esp), %ebx \n\
        movl 20(%esp), %esp \n\
        ret \n\
        .size    stub_upcall_"#type", .-stub_upcall_"#type" \n\
")

DEFINE_UPCALL(new);
DEFINE_UPCALL(restart);
DEFINE_UPCALL(event);

#define DISP_UPCALL(name) mdebug("upcall " #name " (proc:%i, old:%p, restart:%p," \
				 " {ev:%x, old_dis_ev:%i, nb_unbk:%i}, ...)\n", \
				 new_proc, (void*)old, (void*)restarted, \
				 event.events, \
				 event.old_disallowed_events, \
				 event.nb_unblocked)
#define DISP_INFO(p) mdebug("info for 0x%p : {%i, 0x%8x, 0x%p}\n", (p), \
                            (p)->disallowed_events, (p)->current_act_id, \
                            (p)->critical_section);

int upcall_new(act_proc_t new_proc, act_id_t old,
	       act_id_t restarted, act_event_t event,
	       act_register_placeholder_t regs)
{
	DISP_UPCALL(new);
	MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
}

int upcall_restart(act_proc_t new_proc, act_id_t old,
		   act_id_t restarted, act_event_t event,
		   act_register_placeholder_t regs)
{
	DISP_UPCALL(restart);
	MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);


	/* on veut empiler l'adresse de retour ret */
        event.restart_info.esp -= 4;
	/* C'est fait :-) */
        *((int*)event.restart_info.esp) = event.restart_info.eip;
        /* PS : Ça marche car le noyau descend suffisamment la pile :
         * il y a une vingtaine d'octets entre esp et &esp
         * */
        return event.restart_info.eax;
}

int upcall_event(act_proc_t new_proc, act_id_t old,
		 act_id_t restarted, act_event_t event,
		 act_register_placeholder_t regs)
{
	act_proc_info_t *cur_info=&__ma_get_lwp_var(act_info);

	DISP_UPCALL(event);
	DISP_INFO(cur_info);

	//handle unblocked
	if (event.events & (ACT_EVENT_NEW | ACT_EVENT_RESTART |
			    ACT_EVENT_BLOCKED | ACT_EVENT_UNBLOCKED)) {
		MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
	}
	// On restaure l'ancien masque
	cur_info->disallowed_events=event.old_disallowed_events;		
	// On traite le timer si nécessaire
	if (event.events & ACT_EVENT_RESCHED) {
		ma_raise_softirq_from_hardirq(MA_TIMER_HARDIRQ);
	}
	// On resort de la section critique.
	/* On utilise le fait ici que la section critique est le champ
         * preempt_count et que la valeur est celle d'une interruption
         * matérielle */
	ma_irq_exit();

	/* on veut empiler l'adresse de retour ret */
        event.restart_info.esp -= 4;
	/* C'est fait :-) */
        *((int*)event.restart_info.esp) = event.restart_info.eip;
        /* PS : Ça marche car le noyau descend suffisamment la pile :
         * il y a une vingtaine d'octets entre esp et &esp
         * */
        return event.restart_info.eax;
}

#if 0

void locked_start() {}
void locked_end(); /* Déclaration simplement */

extern int real_restart_func (act_proc_t new_proc, int return_value, 
		  int k_param, long eip, volatile long esp, int u_param) 
     asm ("real_restart_func");
extern void restart_func (act_proc_t new_proc, int return_value, 
		  int k_param, long eip, long esp, int u_param) 
     asm ("restart_func");
__asm__ (".align 16 \n\
.globl restart_func \n\
	.type	 restart_func,@function \n\
restart_func: \n\
	addl $4, %esp \n\
	movl %ebx, 24(%esp) \n\
	movl %ecx, 28(%esp) \n\
	movl %edx, 32(%esp) \n\
	call real_restart_func \n\
	movl 32(%esp), %edx \n\
	movl 28(%esp), %ecx \n\
	movl 24(%esp), %ebx \n\
	movl 16(%esp), %esp \n\
	ret \n\
.Lmye: \n\
	.size	 restart_func,.Lmye-restart_func \n\
") ;

int mysleep()
{
	volatile int i,j;
	for (i=0; i<10000; i++)
		for (j=0; j<10000; j++)
			;
	return 0;
}

int real_restart_func(act_proc_t new_proc, int return_value, 
		      int k_param, long eip, volatile long esp, int u_param)
{
	marcel_t current = marcel_self();
	
	SET_LWP(current, GET_LWP_BY_NUM(new_proc));
/*  #ifdef SHOW_UPCALL */
/*  	marcel_printf("\trestart_func (proc %i, ret %i, param 0x%8x," */
/*  		      " ip 0x%8x, sp 0x%8x)\n", */
/*  		      new_proc, return_value, param, eip, esp); */
/*  #else */
	mdebug_upcall("\trestart_func (proc %i, ret %i, k_param %p,"
		     " ip 0x%8x, sp 0x%8x, u_param %p) locked=%i\n",
		     new_proc, return_value, (void*)k_param, eip, esp,
		     (void*)u_param, locked());
	//mdebug("adr de desc = %i\n", *((int*)0xbffefdd8));
/*  #endif */
	//mysleep();
	if (IS_TASK_TYPE_IDLE(current)) {
		if (k_param != ACT_EVENT_RESCHED) {
			mdebug("\trestart_func in idle task %p !! \n",
				      current);
		}
	}
  	if (k_param & ACT_EVENT_UNBLK_RESTART_UNBLOCKED) {
		switch (u_param) {
		case ACT_RESTART_FROM_SCHED:
			MTRACE("Restarting from sched", current);
			MA_THR_RESTARTED(current, "Syscall");
			SET_STATE_READY(marcel_next[GET_LWP_NUMBER(current)]);
			break;
		case ACT_RESTART_FROM_IDLE:
			/* we come from the idle task */
			MTRACE("Restarting from idle", current);
			mdebug("\t\tunchaining idle %p\n",
			       GET_LWP(current)->idle_task);
			state_lock(GET_LWP(current)->idle_task);
			marcel_set_frozen(GET_LWP(current)->idle_task);
			UNCHAIN_TASK(GET_LWP(current)->idle_task);
			break;
		case ACT_RESTART_FROM_UPCALL_NEW:
			MTRACE("Restarting from upcall_new", current);
			break;
		default:
			MARCEL_EXCEPTION_RAISE("invalid parameter");
		}
		unlock_task();
  	} else if (k_param & ACT_EVENT_UNBLK_IDLE) {
		if (u_param == ACT_NEW_WITH_LOCK) {
			mdebug("Ouf ouf ouf : On semble s'en sortir\n");
		}
		state_lock(GET_LWP(current)->prev_running);
		marcel_set_frozen(GET_LWP(current)->prev_running);
		UNCHAIN_TASK(GET_LWP(current)->prev_running);
		MTRACE("Restarting from KERNEL IDLE", current);
		unlock_task();
	} else if (k_param & ACT_EVENT_RESCHED) {
		if(!locked() && preemption_enabled() &&
		   (eip<=(unsigned long)locked_start || (unsigned long)locked_end<=eip)) {
			MTRACE("ActSched", current);
			marcel_update_time(current);
			lock_task();
			marcel_check_polling(MARCEL_EV_POLL_AT_TIMER_SIG);
			ma__marcel_yield();
			unlock_task();
		}
	}
	
	esp = esp-4;  /* on veut empiler l'adresse de retour ret */
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
	register __lwp_t *lwp;
	register marcel_t new_task;

	/* le lock_task n'est pas pris : il n'y a pas d'appels
	 * bloquant DANS marcel.
	 *
	 * Il faut prendre le lock_task sans que le pc sorte de
	 * lock_start/lock_end
	 * */
	lwp=GET_LWP_BY_NUM(proc);
	new_task=lwp->upcall_new_task;

	atomic_inc(&GET_LWP(new_task)->_locked);
 	mdebug_upcall("\tupcall_new(%i) : task_upcall=%p, lwp=%p (%i)\n",
		     proc, new_task,lwp, 
		     lwp->number);

	MTRACE("UpcallNew", new_task);
	//LOG_PTR("*(0x40083756)", *((int**)0x40083756));
	SET_STATE_RUNNING(NULL, new_task, lwp);

	if (locked()>1) {
	        unlock_task();
		mdebug("Kai kai kai : appel bloquant dans marcel !"
		       " (locked was %i)\n", locked());
		/* C'est peut-être juste un printf, on va essayer de s'en
		   sortir */
		for (;;) {
			mdebug("\t==> waiting end of blocking syscall\n");
			act_cntl(ACT_CNTL_READY_TO_WAIT,NULL);
			act_cntl(ACT_CNTL_DO_WAIT, (void*)ACT_NEW_WITH_LOCK);
		}
	}

	marcel_give_hand_from_upcall_new(new_task, lwp);

	/** Never there normally */	
	MARCEL_EXCEPTION_RAISE("Aie, aie aie !\n");
}

void locked_end() {}

#endif

void marcel_upcalls_disallow(void)
{
	__ma_get_lwp_var(act_info).disallowed_events=-1;
}

static void init_act(int proc, act_param_t *param)
{
	unsigned long stack;

	stack=ma_per_lwp(upcall_new_task, GET_LWP_BY_NUM(proc))
		->initial_sp;
	param->upcall_new_sp[proc]=stack;
	mdebug("upcall_new on proc %i use stack 0x%8lx\n", proc, stack);	
	param->upcall_infos[proc]=&ma_per_lwp(act_info, GET_LWP_BY_NUM(proc));
	mdebug("upcalls on proc %i use infos at 0x%p\n", proc, &ma_per_lwp(act_info, GET_LWP_BY_NUM(proc)));	
	DISP_INFO(&ma_per_lwp(act_info, GET_LWP_BY_NUM(proc)));
}

void __marcel_init init_upcalls(void)
{
	act_param_t param;
	int nb_act=marcel_nbvps();
#ifdef MA__LWPS
	int proc;
#endif

	LOG_IN();
	if (nb_act<=0)
		nb_act=0;

	mdebug("init_upcalls(%i)\n", nb_act);

#ifdef MA__LWPS
	for(proc=0; proc<nb_act; proc++) {
		/* WARNING : value 32 hardcoded : max of processors */
		init_act(proc, &param);
	}
#else
	init_act(0, &param);
#endif

	param.magic_number=ACT_MAGIC_NUMBER;
	param.nb_proc_wanted=nb_act;
	param.reset_count=20;
	//param.reset_count=-1; /* No preemption */
	param.critical_section=MA_HARDIRQ_OFFSET;

	/* param->upcall_new_sp[proc] */
	param.upcall_new=stub_upcall_new;
	param.upcall_restart=stub_upcall_restart;
	param.upcall_event=stub_upcall_event;

	//ACTDEBUG(printf("sp=%p, new = %p, restart=%p, nb_unblk=%p\n",
	//		param.upcall_new_sp[0], 
	//		upcall_new, restart_func, &act_nb_unblocked));
	
       	mdebug("Enabling upcalls\n");
	if (act_cntl(ACT_CNTL_INIT, &param)) {
		perror("Bad activations init");
		exit(1);
	}
	//mdebug("Initialisation nearly done\n");
	//mysleep();
	mdebug("Initialisation upcall done\n");
	//ACTDEBUG(printf("Fin act_init\n"));
	//scanf("%i", &proc);
	LOG_OUT();
}

__ma_initfunc_prio(init_upcalls, MA_INIT_UPCALL_START, MA_INIT_UPCALL_START_PRIO, "Démarrage des upcalls");

static void *upcall_no_func(void* p)
{
       MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
}

static void marcel_upcall_lwp_init(marcel_lwp_t* lwp)
{
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
	LOG_IN();

	/****************************************************/
	/* Création de la tâche pour les upcalls upcall_new */
	/****************************************************/
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"upcalld/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_UPCALL_NEW | MA_SF_NORUN);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);

		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif

	// la fonction ne sera jamais exécutée, c'est juste pour avoir une
	// structure de thread marcel dans upcall_new
	marcel_attr_setinitrq(&attr, ma_dontsched_rq(lwp));
	marcel_create_special(&(ma_per_lwp(upcall_new_task, lwp)),
			      &attr, (void*)upcall_no_func, NULL);
	
	MTRACE("Upcall_Task", ma_per_lwp(upcall_new_task, lwp));
	
	/****************************************************/
	LOG_OUT();
}

MA_DEFINE_LWP_NOTIFIER_START(upcall, "Upcalls",
			     marcel_upcall_lwp_init, "Création task for New upcalls",
			     (void), "[none]");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(upcall, MA_INIT_UPCALL_TASK_NEW);


#endif /* MA__ACTIVATION */
