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


#include "tbx_topology.h"
#include "sys/marcel_hwlocapi.h"
#include "marcel.h"
#include <errno.h>


#ifdef MA__LWPS
static MA_DEFINE_NOTIFIER_CHAIN(lwp_chain, "LWP");

/* Need to know about LWPs going up/down? */
int ma_register_lwp_notifier(struct ma_notifier_block *nb)
{
	return ma_notifier_chain_register(&lwp_chain, nb);
}

void ma_unregister_lwp_notifier(struct ma_notifier_block *nb)
{
	ma_notifier_chain_unregister(&lwp_chain, nb);
}

static int ma_call_lwp_notifier(unsigned long val, ma_lwp_t lwp)
{
	return ma_notifier_call_chain(&lwp_chain, val, (void *) lwp);
}

#else

#define ma_call_lwp_notifier(val, lwp) (MA_NOTIFY_DONE)

#endif

marcel_lwp_t __main_lwp = MA_LWP_INITIALIZER;


TBX_FAST_LIST_HEAD(ma_list_lwp_head);
#ifdef MARCEL_GDB
TBX_FAST_LIST_HEAD(ma_list_lwp_head_dead);
#endif

#ifdef MA__LWPS
// Verrou protegeant la liste chainee des LWPs
ma_rwlock_t __ma_lwp_list_lock = MA_RW_LOCK_UNLOCKED;

marcel_lwp_t *ma_vp_lwp[MA_NR_VPS] = { &__main_lwp, };

#if defined(MA__SELF_VAR) && (!defined(MA__LWPS) || !defined(MARCEL_DONT_USE_POSIX_THREADS))
__thread marcel_lwp_t *ma_lwp_self = &__main_lwp;
#endif

#endif


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion des LWPs                                               */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

unsigned marcel_nb_vp = 0;

/* Fonction ex�cut�e dans le thread run_task au d�marrage d'un LWP
 * quelque soit ce LWP, sauf le premier.
 */
#ifdef MA__LWPS
static void marcel_lwp_start(marcel_lwp_t * lwp)
{
	int ret;

	MARCEL_LOG_IN();

	MA_BUG_ON(!ma_in_irq());

#if defined(PM2DEBUG) && defined(MARCEL_DONT_USE_POSIX_THREADS) && defined(MA__NUMA)
	int vpnum = ma_vpnum(lwp);
	MARCEL_LOG("process %i (%s): LWP %u on core %i, node %i\n",
		   getpid(), program_invocation_name,
		   lwp->pid,
		   marcel_vp_core_level(vpnum) ? marcel_vp_core_level(vpnum)->os_core : -1,
		   marcel_vp_node_level(vpnum) ? marcel_vp_node_level(vpnum)->os_node : -1);
#endif

	ret = ma_call_lwp_notifier(MA_LWP_ONLINE, lwp);
	if (ret == MA_NOTIFY_BAD) {
		PM2_LOG("%s: attempt to bring up LWP %p failed\n", __FUNCTION__, lwp);
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}

	__ma_get_lwp_var(online) = 1;
#ifdef MARCEL_SIGNALS_ENABLED
	/* unprotect the baby: update sigmask */
	marcel_sigemptyset(&__ma_get_lwp_var(curmask));
	ma_update_lwp_blocked_signals();
#endif

	MA_BUG_ON(!preemption_enabled());
}
#endif

#ifdef MA__LWPS
/* Fonction du thread run_task pour le d�marrage d'un LWP
 */
static void *TBX_NORETURN lwp_start_func(void *arg TBX_UNUSED)
{
	marcel_lwp_start(MA_LWP_SELF);
	marcel_exit(0);
}
#endif

#ifdef MA__LWPS
/* Fonction ex�cut�e par les kthreads lanc�es
 *
 * La pile de run_task est utilis�e pour l'ex�cution de ce code
 */
static void *lwp_kthread_start_func(void *arg)
{
	struct marcel_topo_level *level;
	marcel_lwp_t *lwp = (marcel_lwp_t *) arg;
	int vpnum;

#if defined(IA64_ARCH) && !defined(MA__PROVIDE_TLS)
	/* save tls segment in lwp data structure */
	register unsigned long reg asm("r13");
	lwp->ma_tls_tp = reg;
#endif

#if defined(MA__PROVIDE_TLS) || defined(MA__SELF_VAR)
	marcel_t self = ma_per_lwp(run_task, lwp);
#endif

	marcel_ctx_set_tls_reg(self);
#if defined(MA__SELF_VAR)
#if !defined(MA__LWPS) || !defined(MARCEL_DONT_USE_POSIX_THREADS)
	ma_lwp_self = lwp;
#endif
	__ma_self = self;
#endif

	PROF_NEW_LWP(ma_vpnum(lwp), ma_per_lwp(run_task, lwp));

	MARCEL_LOG_IN();

	/* Le unlock du changement de contexte */
	ma_preempt_enable();

	MARCEL_LOG("\t\t\t<LWP %d started (self == %lx)>\n",
		   ma_per_lwp(vpnum, lwp), (unsigned long) marcel_kthread_self());

	marcel_lwp_start(lwp);

	/* wait for being notified to stop the kernel thread */
	marcel_sem_P(&lwp->kthread_stop);

	ma_local_bh_disable();
	ma_preempt_disable();

	vpnum = ma_vpnum(lwp);
	MARCEL_LOG("\t\t\t<LWP %d exiting>\n", vpnum);

	MA_BUG_ON(vpnum == -1); // lwp is powered by a vp during termination
	MARCEL_LOG("we were the active LWP of VP %d\n", vpnum);
	level = ma_lwp_vpaffinity_level(lwp);
	marcel_kthread_mutex_lock(&level->kmutex);
	ma_clr_lwp_vpnum(MA_LWP_SELF);
	while (level->blocked || level->spare) {
		MARCEL_LOG("and there are still %d spare and %u blocked LWPs\n", 
			   level->spare, level->blocked);
		if (level->needed == -1) {
			MARCEL_LOG("waking one\n");
#ifdef MARCEL_BLOCKING_ENABLED
			level->blocked--;
#endif
			level->needed = vpnum;
			marcel_kthread_cond_signal(&level->kneed);
			break;
		}
		MARCEL_LOG("VP %d is already waiting for its spare LWP, wait for that to complete\n",
			   level->needed);
		marcel_kthread_cond_wait(&level->kneeddone, &level->kmutex);
	}
	marcel_kthread_mutex_unlock(&level->kmutex);

	ma_lwp_list_lock_write();
	tbx_fast_list_del(&lwp->lwp_list);
#ifdef MARCEL_GDB
	tbx_fast_list_add(&lwp->lwp_list, &ma_list_lwp_head_dead);
#endif
	ma_smp_wmb();
	ma_lwp_list_unlock_write();

	marcel_kthread_exit(NULL);
	MARCEL_LOG_RETURN(NULL);
}

void marcel_lwp_add_lwp(marcel_vpset_t vpset, int vpnum)
{
	unsigned int i;
	marcel_lwp_t *lwp;
	struct marcel_topo_level *level;
	marcel_lwp_t initlwp = MA_LWP_INITIALIZER;
	unsigned found = 0;
#ifdef MM_HEAP_ENABLED
	signed os_node = 0;
#endif

	MARCEL_LOG_IN();

	/* Find the 'lowest' topo_level that match the vpset */
	level = (vpnum) ? &marcel_topo_vp_level[vpnum] : marcel_topo_levels[0];
	while (! marcel_vpset_isequal(&level->vpset, &vpset)) {
		found = 0;
#ifdef MM_HEAP_ENABLED
		if(level->type < MARCEL_LEVEL_NODE)
			os_node = level->os_node;
#endif
		for(i=0; i<level->arity; i++)
			if(marcel_vpset_isincluded(&level->children[i]->vpset, &vpset)) {
				level = level->children[i];
				found = 1;
				break;	
			}
		if(!found)
			break;
	}

#ifdef MA__NUMA
	lwp = marcel_malloc_node(sizeof(*lwp), os_node);
#else
	lwp = ma_malloc_nonuma(sizeof(*lwp));
#endif
	/* initialiser le lwp *avant* de l'enregistrer */
	*lwp = initlwp;
	ma_init_lwp_vpnum(vpnum, lwp);
	lwp->vp_level = level;

	// Initialisation de la structure marcel_lwp_t
	ma_call_lwp_notifier(MA_LWP_UP_PREPARE, lwp);

	// Lancement du thread noyau "propulseur". Il faut d�sactiver les
	// signaux 'SIGALRM' pour que le kthread 'fils' h�rite d'un masque
	// correct.
	__ma_sig_disable_interrupts();

	{
		void *initial_sp, *stack_base;
		const marcel_task_t *run_task;

		run_task = ma_per_lwp(run_task, lwp);

		if (run_task != NULL) {
			initial_sp = (void *) THREAD_GETMEM(run_task, initial_sp);
			stack_base = THREAD_GETMEM(run_task, stack_base);
		} else
			initial_sp = stack_base = NULL;

#ifdef MARCEL_SIGNALS_ENABLED
		/* protect the baby */
		sigset_t ss;
		sigfillset(&ss);
		marcel_sigfillset(&lwp->curmask);
		marcel_sigfillset(&__ma_get_lwp_var(curmask));
		marcel_kthread_sigmask(SIG_SETMASK, &ss, NULL);
#endif

		// Ajout dans la liste globale des LWP
		ma_lwp_list_lock_write();
		marcel_kthread_create(&lwp->pid,
				      initial_sp, stack_base,
				      lwp_kthread_start_func, lwp);
		tbx_fast_list_add_tail(&lwp->lwp_list, &ma_list_lwp_head);
		ma_smp_wmb();
		ma_lwp_list_unlock_write();

#ifdef MARCEL_SIGNALS_ENABLED
		/* father get his old sigmask:
		 *   recompute sigmask with ma_update_lwp_blocked_signals because signal
		 *   mask may change between marcel_kthread_sigmask & *here* */
		ma_update_lwp_blocked_signals();
#endif

	}

	__ma_sig_enable_interrupts();
}

ma_atomic_t ma__last_vp = MA_ATOMIC_INIT(0);
unsigned marcel_lwp_add_vp(marcel_vpset_t vpset)
{
	unsigned num;

	num = ma_atomic_inc_return(&ma__last_vp);

	if (num >= marcel_nbvps() + MARCEL_NBMAXVPSUP)
		MARCEL_EXCEPTION_RAISE("Too many supplementary vps\n");

	if (num >= MA_NR_VPS)
		MARCEL_EXCEPTION_RAISE("Too many vps\n");

	marcel_lwp_add_lwp(vpset, num);

	return num;
}
#endif				// MA__LWPS

#ifdef MA__LWPS
/* TODO: the stack of the lwp->sched_task is currently *NOT FREED* as
   run_task, and other system threads.

   This should be fixed! 
*/
void marcel_lwp_stop_lwp(marcel_lwp_t * lwp)
{
	MARCEL_LOG_IN();
	MARCEL_LOG("stopping LWP %p\n", lwp);

	if (ma_is_first_lwp(lwp)) {
		PM2_LOG("Arghh, trying to kill main_lwp\n");
		MARCEL_LOG_OUT();
		return;
	}
	marcel_sem_V(&lwp->kthread_stop);

	MARCEL_LOG("joining LWP %p\n", lwp);
	
	marcel_kthread_join(&lwp->pid);
	{
		/* La structure devrait �tre lib�r�e ainsi que
		 * les piles des threads r�sidents... 
		 */
#ifndef MARCEL_GDB
		if (lwp->cpuset)
			hwloc_bitmap_free(lwp->cpuset);
		marcel_free_node(lwp, sizeof(marcel_lwp_t), ma_lwp_os_node(lwp));
#endif
	}

	MARCEL_LOG_OUT();
}

/* wait for ourself to maybe become the active LWP of a VP */
void ma_lwp_wait_active(void)
{
	struct marcel_topo_level *level;
	int vpnum;
	/* TODO: bind */
	ma_local_bh_disable();
	ma_preempt_disable();
	vpnum = ma_vpnum(MA_LWP_SELF);
	if (vpnum == -1) {
		MARCEL_LOG("we are a spare LWP (%p)\n", MA_LWP_SELF);
		level = ma_lwp_vpaffinity_level(lwp);
		marcel_kthread_mutex_lock(&level->kmutex);
		level->spare++;
		MARCEL_LOG("now %d spare and %u blocked LWPs\n",
			   level->spare, level->blocked);
		while ((vpnum = level->needed) == -1) {
			MARCEL_LOG("lwp (%p) waiting\n", MA_LWP_SELF);
			marcel_kthread_cond_wait(&level->kneed, &level->kmutex);
			MARCEL_LOG("lwp (%p) waking up\n", MA_LWP_SELF);
		}
		MARCEL_LOG("becoming VP %d\n", vpnum);
		ma_set_lwp_vpnum(vpnum, MA_LWP_SELF);
		level->spare--;
		level->needed = -1;
		MARCEL_LOG("now %d spare and %u blocked LWPs\n",
			   level->spare, level->blocked);
		marcel_kthread_cond_signal(&level->kneeddone);
		marcel_kthread_mutex_unlock(&level->kmutex);
	}
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

/* become a blocked LWP
 *  NB: we increment 'blocked' if only require_spare == tbx_true
 *      means that caller will create a new lwp if start_one == 1 */
int ma_lwp_block(tbx_bool_t require_spare)
{
	int vpnum;
	struct marcel_topo_level *level;
	int start_one = 0;

	ma_preempt_disable();
	vpnum = ma_vpnum(MA_LWP_SELF);
	MARCEL_LOG("we are the active LWP of VP %d\n", vpnum);
	MA_BUG_ON(vpnum == -1);
	level = ma_lwp_vpaffinity_level(MA_LWP_SELF);

	MARCEL_LOG("we get blocked, and wake another LWP\n");
	marcel_kthread_mutex_lock(&level->kmutex);
	ma_clr_lwp_vpnum(MA_LWP_SELF);
	while (level->needed != -1) {
		MARCEL_LOG("Oops, some other CPU already needed an LWP, signaling");
		marcel_kthread_cond_signal(&level->kneed);
		marcel_kthread_cond_wait(&level->kneeddone, &level->kmutex);
	}

	/** NB: level->needed must be set to unblock idle
	 *  thread which can be blocked in ma_lwp_wait_active */
	level->needed = vpnum;
	if (level->spare) {
		marcel_kthread_cond_signal(&level->kneed);
	}
	else {
		start_one = 1;
		if (require_spare)
			level->blocked++;
	}

	/* PB: il y a toutes les chances que ceci ordonnance tout de suite l'autre... */
	marcel_kthread_mutex_unlock(&level->kmutex);
	return start_one;
}

#ifdef MARCEL_BLOCKING_ENABLED
void marcel_enter_blocking_section(void)
{
	if (ma_lwp_block(tbx_true)) {
		MARCEL_LOG("need to start another LWP\n");
		marcel_vpset_t vpset = MARCEL_VPSET_FULL;
		marcel_lwp_add_lwp(vpset, -1);
	}
}

/* become a spare LWP */
void marcel_leave_blocking_section(void)
{
	MARCEL_LOG("we get spare\n");
	ma_set_current_state(MA_TASK_MOVING);
	ma_preempt_enable();
	ma_schedule();
	MARCEL_LOG("we're on an active LWP again\n");
}
#endif

/* wait for at least one VP to become active, lwp is a hint of a still running LWP */
marcel_lwp_t *ma_lwp_wait_vp_active(void)
{
	struct marcel_topo_level *level;
	marcel_lwp_t *lwp = NULL;
	unsigned vp;

	for (vp = 0; vp < marcel_nballvps(); vp++) {
		lwp = ma_get_lwp_by_vpnum(vp);
		if (lwp)
			return lwp;
	}

	ma_preempt_disable();

	/* TODO: walk vpaffinity levels */
	level = ma_lwp_vpaffinity_level(NULL);
	marcel_kthread_mutex_lock(&level->kmutex);
	for (vp = 0; vp < marcel_nballvps(); vp++) {
		lwp = ma_get_lwp_by_vpnum(vp);
		if (lwp)
			break;
	}
	if (!lwp && (level->blocked || level->spare) && level->needed != -1) {
		marcel_kthread_cond_wait(&level->kneeddone, &level->kmutex);
		for (vp = 0; vp < marcel_nballvps(); vp++) {
			lwp = ma_get_lwp_by_vpnum(vp);
			if (lwp)
				break;
		}
		MA_BUG_ON(!lwp);
	}
	marcel_kthread_mutex_unlock(&level->kmutex);

	ma_preempt_enable();

	return lwp;
}

marcel_vpset_t marcel_disabled_vpset = MARCEL_VPSET_ZERO;
static marcel_mutex_t disabled_vpset_mutex = MARCEL_MUTEX_INITIALIZER;

/* User/supervisor/whatever requested stopping using a given set of VPs.
 * No need to take much care, it dosen't happen so often.  */
void marcel_disable_vps(const marcel_vpset_t * vpset)
{
	ma_lwp_t lwp;
	struct marcel_sched_param param = {.sched_priority = MA_SYS_RT_PRIO + 1 };
	marcel_vpset_t old_vpset;
	int vp;

	PROF_EVENTSTR(sched_status, "disabling %d VPs", marcel_vpset_weight(vpset));
	marcel_mutex_lock(&disabled_vpset_mutex);

	/* If we are on a disabled VP, do not let us get preempted by idle, or
	 * else we won't have the time to push entities */
	ma_preempt_disable();

	/* First make sure nobody puts things on it first */
	/* FIXME: someone may have already started putting something on it.
	 * For now, let's assume (and check) it doesn't happen. */
	old_vpset = marcel_disabled_vpset;
	marcel_vpset_orset(&marcel_disabled_vpset, vpset);
	if (marcel_vpset_isequal(&old_vpset, &marcel_disabled_vpset))
		goto out;
	MA_BUG_ON(marcel_vpset_isincluded(&marcel_disabled_vpset, &marcel_machine_level->vpset));

	ma_push_entities(vpset);
	ma_disable_topology_vps(vpset);

	/* Give idle a high priority so it doesn't get overriden by application threads */
	marcel_vpset_foreach_begin(vp, vpset) {
		lwp = ma_get_lwp_by_vpnum(vp);
		if (tbx_likely(lwp))
			marcel_sched_setparam(ma_per_lwp(idle_task, lwp), &param);
	} marcel_vpset_foreach_end(vp, vpset);

	/* And reschedule VPs so they really become idle. */
	ma_resched_vpset(vpset);

	/* Tell the bubble scheduler to redistribute threads on the remaining VPs */
	marcel_bubble_shake();

out:
	ma_preempt_enable();
	marcel_mutex_unlock(&disabled_vpset_mutex);
}

void marcel_enable_vps(const marcel_vpset_t * vpset)
{
	ma_lwp_t lwp;
	struct marcel_sched_param param = {.sched_priority = MA_IDLE_PRIO };
	marcel_vpset_t old_vpset;
	int vp;

	PROF_EVENTSTR(sched_status, "enabling %d VPs", marcel_vpset_weight(vpset));
	marcel_mutex_lock(&disabled_vpset_mutex);

	old_vpset = marcel_disabled_vpset;
	marcel_vpset_clearset(&marcel_disabled_vpset, vpset);
	if (marcel_vpset_isequal(&old_vpset, &marcel_disabled_vpset))
		goto out;

	ma_enable_topology_vps(vpset);

	/* Give idle back a low priority so gets overriden by application threads again */
	marcel_vpset_foreach_begin(vp, vpset) {
		lwp = ma_get_lwp_by_vpnum(vp);
		if (tbx_likely(lwp))
			marcel_sched_setparam(ma_per_lwp(idle_task, lwp), &param);
	} marcel_vpset_foreach_end(vp, vpset);

	/* Tell the bubble scheduler to take profit of the extra VPs */
	marcel_bubble_shake();

	/* And reschedule VPs so they start picking up threads again. */
	ma_resched_vpset(vpset);
out:
	marcel_mutex_unlock(&disabled_vpset_mutex);
}
#endif				// MA__LWPS

#ifndef MA__LWPS
/* mono: no idle thread, but on interrupts we need to when whether we're idling. */
tbx_bool_t ma_currently_idle;
#endif

tbx_bool_t marcel_vp_is_idle(int vpnum LWPS_VAR_UNUSED)
{
#ifdef MA__LWPS
	ma_lwp_t lwp;

	if (marcel_vp_is_disabled(vpnum))
		return tbx_false;

	lwp = ma_get_lwp_by_vpnum(vpnum);
	if (lwp && ma_lwp_curr(lwp) == ma_per_lwp(idle_task, lwp))
		return tbx_true;

	return tbx_false;
#else
	return 	ma_currently_idle;
#endif
}

/* Initialisation d'une structure LWP, y compris la principale */
/* On cr�e deux threads : run_task et idle_task
  
   - run_task sera la t�che ordonnanc�e en premier sur ce LWP. Il
     cr�era d'autres threads si n�cessaire (postexit, ...) puis
     dispara�tra.
 
   - idle est la t�che idle classique. Elle est cr��e ici (ie pas sur
     le bon LWP). Peut-�tre faudrait-t-il d�l�guer sa cr�ation �
     run_task pour allouer de la m�moire propre au LWP... Pb: malloc
     est utilis� => verrou avec contention possible (d�s qu'il y a
     deux LWPs). Donc bof dans run_task. Autre solution, faire un
     malloc respectant le placement du prochain LWP. (Ces pb
     n'interviennent que pour les NUMA o� la m�moire n'est pas
     identique sur tous les noeuds)

 */
static void lwp_init(ma_lwp_t lwp)
{
#ifdef MA__LWPS
	marcel_attr_t attr;
#endif
	char name[MARCEL_MAXNAMESIZE];
	int vpnum;

	MARCEL_LOG_IN();

	vpnum = ma_vpnum(lwp);
	if (ma_is_first_lwp(lwp))
		/* This is already set by marcel_lwp_add_lwp for non-main LWPs */
		lwp->vp_level = &marcel_topo_vp_level[vpnum];

#ifdef MA__NUMA
	lwp->cpuset = hwloc_bitmap_alloc();
	ma_cpuset_to_hwloc(&lwp->vp_level->cpuset, lwp->cpuset);
#endif

#ifdef MA__LWPS
	marcel_sem_init(&lwp->kthread_stop, 0);
#endif
	marcel_snprintf(name, sizeof(name), "lwp%d", vpnum);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWLWPRQ, 2), vpnum, ma_lwp_rq(lwp));
	ma_init_lwp_rq(ma_lwp_rq(lwp), name, lwp);


	lwp->polling_list = NULL;

	if (ma_is_first_lwp(lwp)) {
		ma_per_lwp(run_task, lwp) = MARCEL_SELF;

		ma_lwp_list_lock_write();
		tbx_fast_list_add_tail(&lwp->lwp_list, &ma_list_lwp_head);
		ma_lwp_list_unlock_write();

#if defined(PM2DEBUG) && defined(MARCEL_DONT_USE_POSIX_THREADS) && defined(MA__NUMA)
		MARCEL_LOG("process %i (%s): LWP %u on core %i, node %i\n",
			   getpid(), program_invocation_name,
			   lwp->pid,
			   marcel_vp_core_level(vpnum) ? marcel_vp_core_level(vpnum)->os_core : -1,
			   marcel_vp_node_level(vpnum) ? marcel_vp_node_level(vpnum)->os_node : -1);
#endif

		MARCEL_LOG_OUT();
		return;
	}
#ifdef MA__LWPS
	/************************************************/
	/* Cr�ation de la t�che de d�marrage (run_task) */
	/************************************************/
	/* Cette t�che est lanc�e aussi t�t que possible
	 * d�s que le lwp est cr��.
	 * Elle ex�cute la fonction marcel_lwp_start
	 * gr�ce au code fournit ici
	 * (le thread noyau fait un marcel_longjmp sur cette t�che)
	 */
	marcel_attr_init(&attr);
	marcel_snprintf(name, MARCEL_MAXNAMESIZE, "run_task/%2d", vpnum);
	marcel_attr_setname(&attr, name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setflags(&attr, MA_SF_NORUN | MA_SF_RUNTASK);
	/* Elle doit prendre la main sur toute autre t�che de ce LWP */
	marcel_attr_setprio(&attr, 0);
	marcel_attr_setschedrq(&attr, ma_lwp_rq(lwp));
#ifdef PM2
	{
		char *stack = marcel_malloc(2 * THREAD_SLOT_SIZE);

		marcel_attr_setstackaddr(&attr,
					 (void *) ((unsigned long) (stack + THREAD_SLOT_SIZE) &
						   ~(THREAD_SLOT_SIZE - 1)));
	}
#endif
	/* On passe lwp_start_func() comme fonction, mais seul le LWP
	 * 0 l'ex�cutera. Les autres LWP en SMP utiliseront ce thread
	 * pour ex�cuter lwp_kthread_start_func().
	 */
	marcel_create_special(&(ma_per_lwp(run_task, lwp)), &attr, lwp_start_func, NULL);
	ma_set_task_state((ma_per_lwp(run_task, lwp)), MA_TASK_RUNNING);
	ma_set_task_lwp(ma_per_lwp(run_task, lwp), lwp);
	ma_barrier();
	MTRACE("RunTask", ma_per_lwp(run_task, lwp));

	/***************************************************/

	MARCEL_LOG_OUT();
#endif
}

static int lwp_start(ma_lwp_t lwp LWPS_VAR_UNUSED)
{
	MARCEL_LOG_IN();

#if defined(MA__LWPS)
	int isnotbound;

	if (!marcel_use_fake_topology && lwp->cpuset) {
		long target = hwloc_bitmap_first(lwp->cpuset);
		isnotbound = hwloc_set_cpubind(topology, lwp->cpuset, HWLOC_CPUBIND_THREAD);
		if (isnotbound) {
			if (ENOSYS != errno) {
				perror("hwloc_set_cpubind");
				fprintf(stderr, "while binding LWP %d on CPU%ld\n", ma_vpnum(lwp), target);
				exit(1);
			}
			/** report error for first lwp only: do not pollute stderr **/
			if (1 == ma_vpnum(lwp))
				MA_NOT_SUPPORTED("cpu binding");
		} else
			MARCEL_LOG("LWP %u bound to processor %ld\n", ma_vpnum(lwp), target);
	}
#endif

	MARCEL_LOG_RETURN(0);
}

/* Pour prendre la main au tout d�but : � la d�claration aussi, on se
   d�p�che de remplir le champ 'number'
*/
MA_DEFINE_LWP_NOTIFIER_START_PRIO(lwp, MA_NOTIFIER_PRIO_INITLWP, "Initialisation",
				  lwp_init, "Creation de RunTask",
				  lwp_start, "PROF_NEW_LWP et bind_on_proc");

MA_LWP_NOTIFIER_CALL_UP_PREPARE_PRIO(lwp, MA_INIT_LWP_MAIN_STRUCT,
				     MA_INIT_LWP_MAIN_STRUCT_PRIO);
MA_LWP_NOTIFIER_CALL_ONLINE(lwp, MA_INIT_LWP);

static void marcel_lwp_finished(void)
{
#ifdef MA__LWPS
	MA_LWP_SELF->pid = marcel_kthread_self();
#endif
	__ma_get_lwp_var(online) = 1;
}

__ma_initfunc_prio(marcel_lwp_finished, MA_INIT_LWP_FINISHED, MA_INIT_LWP_FINISHED_PRIO,
		   "Tell __main_lwp is online");
