
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

#define MA_FILE_DEBUG polling
#include "marcel.h"
#include <errno.h>

/****************************************************************
 * Voir marcel_io.c pour un exemple d'implémentation
 */

/****************************************************************
 * Compatibility stuff
 * Cette partie disparaîtra bientôt...
 */
#define MAX_POLL_IDS    16
	
static unsigned nb_poll_structs = 0;
static struct poll_struct poll_structs[MAX_POLL_IDS];

static int compat_poll_one(marcel_ev_serverid_t id, 
		    marcel_ev_op_t op,
		    marcel_ev_inst_t ev, 
		    int nb_ev, int option)
{
	marcel_pollinst_t inst=struct_up(ev, poll_cell_t, inst);
	marcel_pollid_t ps=struct_up(id, struct poll_struct, server);
	int first_call=TRUE;

	ps->cur_cell = inst;
	if (option & MARCEL_EV_OPT_EV_IS_GROUPED) {
		first_call=FALSE;
	}
	if ((*ps->fastfunc)(ps, inst->arg, first_call)) {
		MARCEL_EV_POLL_SUCCESS(id, ev);
	}
	return 0;
}

static int compat_poll_all(marcel_ev_serverid_t id, 
		    marcel_ev_op_t op,
		    marcel_ev_inst_t ev, 
		    int nb_ev, int option)
{
	marcel_pollid_t ps=struct_up(id, struct poll_struct, server);
	(*ps->func)(ps,
		    ma_nr_running(),
		    marcel_sleepingthreads(),
		    marcel_blockedthreads());	
	return 0;
}

static int compat_poll_group(marcel_ev_serverid_t id, 
		      marcel_ev_op_t op,
		      marcel_ev_inst_t ev, 
		      int nb_ev, int option)
{
	marcel_pollid_t ps=struct_up(id, struct poll_struct, server);
	(*ps->gfunc)(ps);
	return 0;
}


marcel_pollid_t marcel_pollid_create_X(marcel_pollgroup_func_t g,
				       marcel_poll_func_t f,
				       marcel_fastpoll_func_t h,
				       unsigned polling_points,
				       char* name)
{
	marcel_pollid_t id;

	LOG_IN();

	if(nb_poll_structs == MAX_POLL_IDS) {
		RAISE(CONSTRAINT_ERROR);
	}

#warning non Thread-Safe incrementation here
#warning marcel_pollid_create will be removed (and not corrected) in flavor of marcel_ev_server_init	
	id = &poll_structs[nb_poll_structs++];

	marcel_ev_server_init(&id->server, name);
	marcel_ev_server_set_poll_settings(&id->server, 
					   (polling_points
					    |MARCEL_EV_POLL_AT_IDLE)
					   & MARCEL_EV_POLL_AT_TIMER_SIG, 1);
	if (g) {
		id->gfunc = g;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_GROUP,
					      compat_poll_group);
	}
	if (f) {
		id->func = f;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_ALL,
					      compat_poll_all);
	}
	if (h) {
		id->fastfunc = h;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_ONE,
					      compat_poll_one);
	}

	marcel_ev_server_start(&id->server);

	mdebug("registering pollid %p (gr=%p, func=%p, fast=%p, pts=%x)\n",
	       id, g, f, h, id->server.poll_points);

	LOG_OUT();

	return id;
}

void marcel_poll(marcel_pollid_t id, any_t arg)
{
	poll_cell_t cell;

	cell.id=&id->server;
	cell.arg=arg;

	marcel_ev_wait_one(&id->server, &cell.inst, 0);
}

void marcel_force_check_polling(marcel_pollid_t id)
{
	LOG_IN();
	
	marcel_ev_poll_force(&id->server);
	
	LOG_OUT();
	return;
}

/****************************************************************
 * Implémentation courante
 ****************************************************************/


/* Variable contenant la liste chainée des serveurs de polling ayant
 * quelque chose à scruter.
 * Le lock permet de protéger le parcours/modification de la liste
 */
LIST_HEAD(ma_ev_list_poll);
static ma_rwlock_t ev_poll_lock = MA_RW_LOCK_UNLOCKED;

/****************************************************************
 * Gestion de l'exclusion mutuelle
 *
 * Remarque sur le locking :
 * + la plupart de la synchro est faite en s'appuyant sur les tasklets
 * + le polling régulier utilise les timers
 * + les fonctions devant accéder à des données d'un serveur doivent
 *   au choix :
 *   # être appelée depuis la tasklet de polling
 *     (exemple check_polling_for())
 *   # ou bien 
 *        . le lock du serveur doit être pris
 *        . les softirq doivent être désactivées
 *        . la tasklet doit être désactivées
 *     (exemple marcel_ev_wait())
 */

inline static int __lock_server(marcel_ev_serverid_t id, marcel_task_t *owner)
{
	LOG_IN();
	ma_spin_lock_softirq(&id->lock);
	ma_tasklet_disable(&id->poll_tasklet);
	id->lock_owner = owner;
	LOG_RETURN(0);
}

inline static int __unlock_server(marcel_ev_serverid_t id)
{
	LOG_IN();
	id->lock_owner=NULL;
	ma_tasklet_enable(&id->poll_tasklet);
	ma_spin_unlock_softirq(&id->lock);
	LOG_RETURN(0);
}

/* Renvoie MARCEL_SELF si on a le lock
   NULL sinon
*/
inline static int ensure_lock_server(marcel_ev_serverid_t id)
{
	LOG_IN();
	if (id->lock_owner == MARCEL_SELF) {
		LOG_RETURN((int)MARCEL_SELF);
	}
	__lock_server(id, MARCEL_SELF);
	LOG_RETURN(0);
}

inline static int lock_server_owner(marcel_ev_serverid_t id, int owner)
{
	LOG_IN();
	__lock_server(id, (marcel_task_t*)owner);
	LOG_RETURN(0);
}

inline static int restore_lock_server_locked(marcel_ev_serverid_t id, 
					     int old_owner)
{
	LOG_IN();
	if (!old_owner) {
		__unlock_server(id);
	}
	LOG_RETURN(0);
}

inline static int restore_lock_server_unlocked(marcel_ev_serverid_t id,
					       int old_owner)
{
	LOG_IN();
	if (old_owner) {
		__lock_server(id, (marcel_task_t*)old_owner);
	}
	LOG_RETURN(0);
}

/* Utilisé par l'application */
int marcel_ev_lock(marcel_ev_serverid_t id)
{
	LOG_IN();
#ifdef MA__DEBUG	
	/* Ce lock n'est pas réentrant ! Vérifier l'appli */
	MA_BUG_ON (id->lock_owner == MARCEL_SELF);
#endif
	__lock_server(id, MARCEL_SELF);
	LOG_RETURN(0);
}

/* Utilisé par l'application */
int marcel_ev_unlock(marcel_ev_serverid_t id)
{
	LOG_IN();
#ifdef MA__DEBUG	
	/* On doit avoir le lock pour le relâcher */
	MA_BUG_ON (id->lock_owner != MARCEL_SELF);
#endif
	__unlock_server(id);
	LOG_RETURN(0);
}

/****************************************************************
 * Gestion des événements signalés OK par les call-backs
 *
 */

marcel_ev_inst_t marcel_ev_get_success(marcel_ev_serverid_t id)
{
	marcel_ev_inst_t ev=NULL;
	LOG_IN();
	ma_spin_lock_softirq(&id->ev_success_lock);
	if (!list_empty(&id->list_ev_success)) {
		ev=list_entry(&(id->list_ev_success.next), 
			      struct marcel_ev, chain_success);
		list_del_init(&ev->chain_success);
	}
	ma_spin_unlock_softirq(&id->ev_success_lock);
	LOG_RETURN(ev);
}

inline static int remove_success_ev(marcel_ev_serverid_t id, 
				    marcel_ev_inst_t ev)
{
	LOG_IN();
	if (list_empty(&ev->chain_success)) {
		/* On n'est pas enregistré */
		LOG_RETURN(0);
	}
	ma_spin_lock(&id->ev_success_lock);
	list_del_init(&ev->chain_success);
	ma_spin_unlock(&id->ev_success_lock);
	LOG_RETURN(0);
}

inline static int add_success_ev(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	LOG_IN();
	if (!list_empty(&ev->chain_success)) {
		/* On est déjà enregistré */
		LOG_RETURN(0);
	}
	ma_spin_lock(&id->ev_success_lock);
	list_add_tail(&ev->chain_success, &id->list_ev_success);
	ma_spin_unlock(&id->ev_success_lock);
	LOG_RETURN(0);
}

struct waiter {
	struct list_head chain_wait;
	marcel_sem_t sem;
	/* 0: event
	   -ECANCELED: marcel_unregister called
	*/
	int ret;
#ifdef MA__DEBUG
	marcel_task_t *task;
#endif	
};

/* Réveils pour l'événement */
inline static int wake_ev_waiters(marcel_ev_serverid_t id, marcel_ev_inst_t ev,
				  int code)
{
	struct waiter *wait, *tmp;
	LOG_IN();
	list_for_each_entry_safe(wait, tmp, &ev->list_ev_waiters, chain_wait) {
#ifdef MA__DEBUG
		switch (code) {
		case 0:
			mdebug("Poll succeed with task %p\n", wait->task);
			break;
		default:
			mdebug("Poll awake with task %p on code %i\n", wait->task, code);
		}
#endif
		wait->ret=code;
		list_del_init(&wait->chain_wait);
		marcel_sem_V(&wait->sem);
	}
	LOG_RETURN(0);
}

/* Réveils du serveur */
inline static int wake_id_waiters(marcel_ev_serverid_t id, int code)
{
	struct waiter *wait, *tmp;
	LOG_IN();
	list_for_each_entry_safe(wait, tmp, &id->list_id_waiters, chain_wait) {
#ifdef MA__DEBUG
		switch (code) {
		case 0:
			mdebug("Poll succeed with global task %p\n", wait->task);
			break;
		default:
			mdebug("Poll awake with global task %p on code %i\n", wait->task, code);
		}
#endif
		wait->ret=code;
		list_del_init(&wait->chain_wait);
		marcel_sem_V(&wait->sem);
	}
	LOG_RETURN(0);
}

/* On gère les événements signalés OK par les call-backs
 * - retrait événtuel de la liste des ev groupés (que l'on compte)
 * - réveil des waiters sur les événements et le serveur
 */
inline static int __manage_ready(marcel_ev_serverid_t id)
{
	int nb_grouped_ev_removed = 0;
	int nb_ev_ask_wake_server = 0;
	marcel_ev_inst_t ev, tmp;

	LOG_IN();
	if (list_empty(&id->list_ev_ready)) {
		return 0;
	}

	list_for_each_entry_safe(ev, tmp, &id->list_ev_ready, chain_ev_ready) {
		ev->state |= MARCEL_EV_STATE_OCCURED;
		if ((ev->state & MARCEL_EV_STATE_GROUPED) &&
		    (ev->state & MARCEL_EV_STATE_ONE_SHOT)) {
			list_del_init(&ev->chain_ev);
			nb_grouped_ev_removed++;
			ev->state &= ~MARCEL_EV_STATE_GROUPED;
		}
		mdebug("Poll succeed with ev %p\n", ev);
		wake_ev_waiters(id, ev, 0);
		if (! (ev->state & MARCEL_EV_STATE_NO_WAKE_SERVER)) {
			add_success_ev(id, ev);
			nb_ev_ask_wake_server++;
		}
		list_del_init(&ev->chain_ev_ready);
	}
	id->ev_poll_grouped_nb -= nb_grouped_ev_removed;
	if (nb_grouped_ev_removed) {
		mdebug("Nb grouped task set to %i\n", 
		       id->ev_poll_grouped_nb);
	}
	
	if (nb_ev_ask_wake_server) {
		wake_id_waiters(id, nb_ev_ask_wake_server);
	}

#ifdef MA__DEBUG
	MA_BUG_ON(!list_empty(&id->list_ev_ready));
#endif
	LOG_RETURN(nb_grouped_ev_removed);
}

/* On groupe si nécessaire, ie :
 * s'il y a  au moins 2 requetes
 * ou s'il n'y a pas de "fast poll", alors il faut factoriser.
 *
 * Il doit y avoir au moins une tâche à grouper.
 */
inline static int __poll_group(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!id->ev_poll_grouped_nb);
#endif
	if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_GROUP] &&
	    ( id->ev_poll_grouped_nb > 1 
	      || ! id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])) {
		mdebug("Factorizing %i polling(s) with POLL_GROUP\n",
		       id->ev_poll_grouped_nb);
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_GROUP])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
			 ev, id->ev_poll_grouped_nb, 0);
		return 1;
	}
	mdebug("No need to group %i polling(s)\n",
	       id->ev_poll_grouped_nb);
	return 0;
}

/* On démarre un polling (et éventuellement le timer) en l'ajoutant
 * dans la liste
 */
inline static void __poll_start(marcel_ev_serverid_t id)
{
	ma_write_lock_softirq(&ev_poll_lock);
	MA_BUG_ON(!list_empty(&id->chain_poll));
	mdebug("Starting polling for %s\n", id->name);
	list_add(&id->chain_poll, &ma_ev_list_poll);
	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebug("Starting timer polling for %s at frequency %i\n",
		       id->name, id->frequency);
		/* ma_mod_timer des fois qu'un ancien soit toujours
		 * pending... */
		ma_mod_timer(&id->poll_timer, ma_jiffies+id->frequency);
	}
	ma_write_unlock_softirq(&ev_poll_lock);
}

/* Plus rien à scruter 
 * reste à
 * + arrêter le timer
 * + enlever le serveur de la liste des scrutations en cours
 */
inline static void __poll_stop(marcel_ev_serverid_t id)
{
	ma_write_lock_softirq(&ev_poll_lock);
	mdebug("Stopping polling for %s\n", id->name);
	list_del_init(&id->chain_poll);
	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebug("Stopping timer polling for %s\n", id->name);
		ma_del_timer(&id->poll_timer);
	}
	ma_write_unlock_softirq(&ev_poll_lock);
}

/* On vient de faire une scrutation complète (pas un POLL_ONE sur le
 * premier), on réenclanche un timer si nécessaire.
 */
inline static void __update_timer(marcel_ev_serverid_t id)
{
	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebugl(7, "Update timer polling for %s at frequency %i\n",
			id->name, id->frequency);
		ma_mod_timer(&id->poll_timer, ma_jiffies+id->frequency);
	}
}

//inline static void __ensure_timer(marcel_ev_serverid_t id)
//{
//	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
//		ma_debug(polling,
//			 "Ensuring timer polling for %s at frequency %i\n",
//			 id->name, id->frequency);
//		ma_mod_timer(&id->poll_timer, id->poll_timer.expires);
//	}
//}

// Checks to see if some polling jobs should be done.
static void check_polling_for(marcel_ev_serverid_t id)
{
	int nb=id->ev_poll_grouped_nb;
#ifdef MA__DEBUG
	static int count=0;

	mdebugl(7, "Check polling for %s\n", id->name);
	
	if (! count--) {
		mdebugl(6, "Check polling 50000 for %s\n", id->name);
		count=50000;
	}
#endif

	if (!nb) 
		return;

	id->registered_ev_not_yet_polled=0;
	if(nb == 1 && id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE, 
			 list_entry(id->list_ev_poll_grouped.next,
				    struct marcel_ev, chain_ev),
			 nb, MARCEL_EV_OPT_EV_IS_GROUPED);

	} else if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL]) {
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL])
			(id, MARCEL_EV_FUNCTYPE_POLL_ALL,
			 NULL, nb, 0);
		
	} else if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		marcel_ev_inst_t ev;
		FOREACH_EV_POLL_BASE(ev, id){
			(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
				(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
				 ev, nb,
				 MARCEL_EV_OPT_EV_IS_GROUPED
				 |MARCEL_EV_OPT_EV_ITER);
		}
	} else {
		/* Pas de méthode disponible ! */
		RAISE(PROGRAM_ERROR);
	}

	if (__manage_ready(id)) {
		/* Le nombre de tache en cours de polling a varié */
		if(!id->ev_poll_grouped_nb) {
			__poll_stop(id);
			return;
		} else {
			__poll_group(id, NULL);
		}
	}
	__update_timer(id);
	return;
}

/* call-back pour la tasklet */
void marcel_poll_from_tasklet(unsigned long hid)
{
	marcel_ev_serverid_t id=(marcel_ev_serverid_t)hid;
	mdebugl(7, "Tasklet function for %s\n", id->name);
	check_polling_for(id);
	return;
}

/* call-back pour le timer */
void marcel_poll_timer(unsigned long hid)
{
	marcel_ev_serverid_t id=(marcel_ev_serverid_t)hid;
	mdebugl(7, "Timer function for %s\n", id->name);
	ma_tasklet_schedule(&id->poll_tasklet);
	return;
}

/* Appelé par le code en divers points (yield et lib_entry
 * principalement) */
void __marcel_check_polling(unsigned polling_point)
{

	marcel_ev_serverid_t id;

	//debug("Checking polling at %i\n", polling_point);
	ma_read_lock_softirq(&ev_poll_lock);
	list_for_each_entry(id, &ma_ev_list_poll, chain_poll) {
		if (id->poll_points & polling_point) {
			mdebugl(7, "Scheduling polling for %s at point %i\n",
				id->name, polling_point);
			ma_tasklet_schedule(&id->poll_tasklet);
		}
	}
	ma_read_unlock_softirq(&ev_poll_lock);
}

/* Force une scrutation sur un serveur */
void marcel_ev_poll_force(marcel_ev_serverid_t id)
{
	LOG_IN();
	mdebug("Poll forced for %s\n", id->name);
	/* Pas très important si on loupe quelque chose ici (genre
	 * liste modifiée au même instant)
	 */
	if (!list_empty(&id->list_ev_poll_grouped)) {
		ma_tasklet_schedule(&id->poll_tasklet);
	}
	LOG_OUT();
}

/* Force une scrutation synchrone sur un serveur */
void marcel_ev_poll_force_sync(marcel_ev_serverid_t id)
{
	int lock;
	LOG_IN();
	mdebug("Sync poll forced for %s\n", id->name);

	/* On se synchronise */
	lock=ensure_lock_server(id);

	check_polling_for(id);
	
	restore_lock_server_locked(id, lock);
	LOG_OUT();
}

/****************************************************************
 * Attente d'événement
 *
 * TODO: utiliser BLOCK_ONE si on a les activations
 *
 * Remarque: facile à scinder en deux si on veut de l'attente
 * asynchrone. Ne pas oublier alors les impératifs de locking.
 */


inline static void verify_server_state(marcel_ev_serverid_t id) {
#ifdef MA__DEBUG
	MA_BUG_ON(id->state!=MA_EV_SERVER_STATE_LAUNCHED);
#endif
}

inline static void __init_ev(marcel_ev_inst_t ev)
{
	INIT_LIST_HEAD(&ev->chain_ev);
	INIT_LIST_HEAD(&ev->chain_ev_ready);
	INIT_LIST_HEAD(&ev->chain_success);
	ev->state=0;
	INIT_LIST_HEAD(&ev->list_ev_waiters);
}

int marcel_ev_init(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	LOG_IN();
	__init_ev(ev);
	LOG_RETURN(0);
}

/* Ajout d'un attribut spécifique à un événement */
int marcel_ev_attr_set(marcel_ev_inst_t ev, int attr)
{
	LOG_IN();
	if (attr & MARCEL_EV_ATTR_ONE_SHOT) {
		ev->state |= MARCEL_EV_STATE_ONE_SHOT;
	}
	if (attr & MARCEL_EV_ATTR_NO_WAKE_SERVER) {
		ev->state |= MARCEL_EV_STATE_NO_WAKE_SERVER;
	}
	if (attr & (~(MARCEL_EV_ATTR_ONE_SHOT|MARCEL_EV_ATTR_NO_WAKE_SERVER))) {
		RAISE(CONSTRAINT_ERROR);
	}
	LOG_RETURN(0);
}

inline static int __register(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{

	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	mdebug("Adding request\n");
	list_add(&ev->chain_ev, &id->list_ev_poll_grouped);
	id->ev_poll_grouped_nb++;
	ev->state |= (MARCEL_EV_STATE_GROUPED|MARCEL_EV_STATE_REGISTERED);
	__poll_group(id, ev);
	
	if (id->ev_poll_grouped_nb == 1) {
		__poll_start(id);
	}
	LOG_RETURN(0);
}

/* Enregistrement d'un événement */
int marcel_ev_register(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	int lock;
	LOG_IN();

	lock=ensure_lock_server(id);

	verify_server_state(id);
	MA_BUG_ON(ev->state & MARCEL_EV_STATE_REGISTERED);

	id->registered_ev_not_yet_polled++;

	__register(id, ev);
	
	restore_lock_server_locked(id, lock);

	LOG_RETURN(0);
}

inline static int __unregister(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	LOG_IN();

	wake_ev_waiters(id, ev, -ECANCELED);
	remove_success_ev(id, ev);

	if (ev->state & MARCEL_EV_STATE_GROUPED) {
		list_del_init(&ev->chain_ev);
		id->ev_poll_grouped_nb--;
		if (id->ev_poll_grouped_nb) {
			__poll_group(id, NULL);
		} else {
			__poll_stop(id);
		}
	}

	LOG_RETURN(0);
}

/* Abandon d'un événement et retour des threads en attente sur cet événement */
int marcel_ev_unregister(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	int lock;
	LOG_IN();

	lock=ensure_lock_server(id);

	verify_server_state(id);
	MA_BUG_ON(!(ev->state & MARCEL_EV_STATE_REGISTERED));

	__unregister(id, ev);

	restore_lock_server_locked(id, lock);

	LOG_RETURN(0);
}

inline static int __wait_ev(marcel_ev_serverid_t id, marcel_ev_inst_t ev, 
			    marcel_time_t timeout)
{
	struct waiter wait;
	LOG_IN();

	if (timeout) {
		RAISE(NOT_IMPLEMENTED);
	}

	list_add(&wait.chain_wait, &ev->list_ev_waiters);
	marcel_sem_init(&wait.sem, 0);
	wait.ret=0;
#ifdef MA__DEBUG
	wait.task=MARCEL_SELF;
#endif

	__unlock_server(id);

	marcel_sem_P(&wait.sem);

	LOG_RETURN(wait.ret);
}

/* Attente d'un thread sur un événement déjà enregistré */
int marcel_ev_wait_ev(marcel_ev_serverid_t id, marcel_ev_inst_t ev, 
		      marcel_time_t timeout)
{
	int lock;
	int ret=0;
	LOG_IN();

	lock=ensure_lock_server(id);

	verify_server_state(id);
	MA_BUG_ON(!(ev->state & MARCEL_EV_STATE_REGISTERED));

	ev->state &= ~MARCEL_EV_STATE_OCCURED;

	/* TODO: On pourrait faire un check juste sur cet événement 
	 * (au moins quand on itère avec un poll_one sur tous les ev) 
	 */
	check_polling_for(id);
	
	if (!(ev->state & MARCEL_EV_STATE_OCCURED)) {
		ret=__wait_ev(id, ev, timeout);
	}

	restore_lock_server_unlocked(id, lock);

	LOG_RETURN(ret);
}

/* Attente d'un thread sur un quelconque événement du serveur */
int marcel_ev_wait_server(marcel_ev_serverid_t id, marcel_time_t timeout)
{
	int lock;
	struct waiter wait;
	LOG_IN();

	lock=ensure_lock_server(id);
	verify_server_state(id);

	if (timeout) {
		RAISE(NOT_IMPLEMENTED);
	}
	
	list_add(&wait.chain_wait, &id->list_id_waiters);
	marcel_sem_init(&wait.sem, 0);
	wait.ret=0;
#ifdef MA__DEBUG
	wait.task=MARCEL_SELF;
#endif
	/* TODO: on pourrait ne s'enregistrer que si le poll ne fait rien */
	check_polling_for(id);

	__unlock_server(id);

	marcel_sem_P(&wait.sem);

	restore_lock_server_unlocked(id, lock);

	LOG_RETURN(wait.ret);
}

/* Enregistrement, attente et désenregistrement d'un événement */
int marcel_ev_wait_one(marcel_ev_serverid_t id, marcel_ev_inst_t ev, marcel_time_t timeout)
{
	int lock;
	int checked=0;
	int waken_up=0;
	LOG_IN();

	lock=ensure_lock_server(id);

	verify_server_state(id);
	MA_BUG_ON(ev->state & MARCEL_EV_STATE_REGISTERED);

	__init_ev(ev);
	mdebug("Marcel_poll (thread %p)...\n", marcel_self());
	mdebug("using pollid %s\n", id->name);

	ev->state |= MARCEL_EV_STATE_ONE_SHOT|MARCEL_EV_STATE_NO_WAKE_SERVER;

	if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		mdebug("Using Immediate POLL_ONE\n");
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
			 ev, id->ev_poll_grouped_nb, 0);
		waken_up=__manage_ready(id);
		checked=1;
	}
	
	if (ev->state & MARCEL_EV_STATE_OCCURED) {
		if (waken_up) {
			/* Le nombre de tache en cours de polling a varié */
			if(!id->ev_poll_grouped_nb) {
				__poll_stop(id);
				//LOG_RETURN(0);
			} else {
				__poll_group(id, NULL);
			}
		}
		/* Pas update_timer(id) car on a fait un poll_one et
		 * pas poll_all */
		restore_lock_server_locked(id, lock);
		LOG_RETURN(0);
	}

	__register(id, ev);

	if (!checked) {
		check_polling_for(id);
	}

	if (!(ev->state & MARCEL_EV_STATE_OCCURED)) {
		__wait_ev(id, ev, timeout);
		lock_server_owner(id, lock);
	}
	
	__unregister(id, ev);

	restore_lock_server_locked(id, lock);
	
	LOG_RETURN(0);
}


#if 0
int marcel_ev_wait(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	int old_nb_grouped;
	int waken_up=0;
	LOG_IN();
	


	ma_spin_lock_softirq(&id->lock);
	ma_tasklet_disable(&id->poll_tasklet);

	old_nb_grouped=id->ev_poll_grouped_nb;

	if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		mdebug("Using Immediate POLL_ONE\n");
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
			 ev, id->ev_poll_grouped_nb, 0);
		waken_up=__wake_ready(id);
	}
	if (!(ev->state & MARCEL_EV_STATE_UNBLOCKED)) {
		/* On doit ajouter la requête à celles en attente */
		mdebug("Adding request\n");
		list_add(&ev->ev_list, &id->ev_poll_grouped);
		marcel_sem_init(&ev->sem, 0);
		id->ev_poll_grouped_nb++;
		ev->state |= (MARCEL_EV_STATE_GROUPED|
			      MARCEL_EV_STATE_NEED_SEM_V);
		__poll_group(id, ev);
	}
	if (!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		/* On a pas encore testé. On tente ici */
#ifdef MA__DEBUG
		MA_BUG_ON(!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL]);
#endif
		mdebug("Using Immediate POLL_ALL\n");
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL])
			(id, MARCEL_EV_FUNCTYPE_POLL_ALL,
			 ev, id->ev_poll_grouped_nb, 0);	
		waken_up=__wake_ready(id);
	}
	/* On a fait un poll d'une manière ou d'une autre */
	/* On teste si des requêtes groupées sont finies */
	if (waken_up) {
		/* Le nombre de tache en cours de polling a varié */
		if(!id->ev_poll_grouped_nb) {
			/* Rien a scruter */
			if (old_nb_grouped) {
				/* Il y avait du polling. On l'arrête */
				__poll_stop(id);
			}
		} else {
			/* Des choses à scruter */
			__poll_group(id, NULL);
			if (!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
				/* On a fait un poll global, on
				 * s'occupe du timer.
				 *
				 * Le timer était nécessairement actif car
				 * 1) il y a eu au moins une tâche réveillée
				 * 2) il reste encore une tâche en attente
				 * 3) donc il y avait une tâche en attente avant
				 */
				__update_timer(id);
			}
		}
	} else {
		/* Rien de réveillé. Que doit-on faire ? */
		if(id->ev_poll_grouped_nb) {
			/* Il y a du polling a faire */
			if(old_nb_grouped==0) {
				/* On vient de mettre la première tâche */
				__poll_start(id);
			} else {
				__update_timer(id);
			}
		}
	}
	ma_tasklet_enable(&id->poll_tasklet);
	ma_spin_unlock_softirq(&id->lock);
	//__ensure_timer(id);

	if (ev->state & MARCEL_EV_STATE_UNBLOCKED) {
		return 0;
	}
	marcel_sem_P(&ev->sem);
	return 0;
}

#endif

/****************************************************************
 * Fonctions d'initialisation/terminaison
 */

int marcel_ev_server_set_poll_settings(marcel_ev_serverid_t id, 
					      unsigned poll_points,
					      unsigned frequency)
{
#ifdef MA__DEBUG
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(id->state != MA_EV_SERVER_STATE_INIT);
#endif
	id->poll_points=poll_points;
	if (poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		frequency=frequency?:1;
		id->frequency = frequency * MA_JIFFIES_PER_TIMER_TICK;
	}
	return 0;	
}

int marcel_ev_server_start(marcel_ev_serverid_t id)
{
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(id->state != MA_EV_SERVER_STATE_INIT);
	if (!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]
	    && !id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL]) {
		mdebug("One poll function needed for %s\n", id->name);
		RAISE(PROGRAM_ERROR);
	}
	id->state=MA_EV_SERVER_STATE_LAUNCHED;
	return 0;
}

int marcel_ev_server_stop(marcel_ev_serverid_t id)
{
	int lock;
	marcel_ev_inst_t ev, tmp;
	LOG_IN();

	lock=ensure_lock_server(id);
	verify_server_state(id);

	id->state=MA_EV_SERVER_STATE_HALTED;

	FOREACH_EV_POLL_BASE_SAFE(ev, tmp, id) {
		__unregister(id, ev);
	}
	wake_id_waiters(id, -ECANCELED);

	restore_lock_server_unlocked(id, lock);

	LOG_RETURN(0);
	
	return 0;
}

