
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
	
static ma_atomic_t nb_poll_structs = MA_ATOMIC_INIT(-1);
static struct poll_struct poll_structs[MAX_POLL_IDS];

typedef struct marcel_per_lwp_polling_s{
	int *data;
	int value_to_match;
	void (*func) (void *);
	void *arg;
	marcel_t task;
	volatile tbx_bool_t blocked;
	struct marcel_per_lwp_polling_s* next;
}marcel_per_lwp_polling_t;

int marcel_per_lwp_polling_register(int *data,
                                    int value_to_match,
				    void (*func) (void *),
				    void *arg)
{
	marcel_per_lwp_polling_t cell;
	marcel_t task;
	marcel_lwp_t *lwp;
	
	if(func != NULL)
        	func(arg);
	
	if((*data) == value_to_match)
		return 0;

	ma_local_bh_disable(); 
        ma_preempt_disable();
	
	task = marcel_self();
	lwp = ma_get_task_lwp(task);

	cell.data = data;
	cell.value_to_match = value_to_match;
	cell.func = func;
	cell.arg = arg;
	cell.task = task;
	cell.blocked = tbx_true;
	
	/*Insertion de la donnée*/
	cell.next = lwp->polling_list;
	lwp->polling_list = &cell;

	/*Blocage*/
	INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
			                        cell.blocked,
			                        ma_preempt_enable_no_resched(); 
						ma_local_bh_enable(),
			                        ma_local_bh_disable();
						ma_preempt_disable());
	ma_preempt_enable_no_resched();
        ma_local_bh_enable();	
	return 0;
}
	
void marcel_per_lwp_polling_proceed(){
	marcel_per_lwp_polling_t* cur_cell;
	marcel_per_lwp_polling_t* new_cell_list;
	marcel_per_lwp_polling_t* next_cell;
        marcel_t task;
        marcel_lwp_t *lwp;
	
	ma_local_bh_disable(); 
        ma_preempt_disable();
	
        task = marcel_self();
        lwp = ma_get_task_lwp(task);
	
	new_cell_list = NULL;
	cur_cell = lwp->polling_list;
	while(cur_cell != NULL){
		next_cell = cur_cell->next;
		if((*(cur_cell->data)) != cur_cell->value_to_match) {
			if(cur_cell->func != NULL)
				cur_cell->func(cur_cell->arg);
			cur_cell->next = new_cell_list;
			new_cell_list = cur_cell;
		} else {
			ma_wake_up_thread(cur_cell->task);		
		}
		cur_cell = next_cell;
	}
	lwp->polling_list = new_cell_list;
	ma_preempt_enable_no_resched(); 
        ma_local_bh_enable();
}

static int compat_poll_one(marcel_ev_server_t server, 
		    marcel_ev_op_t op,
		    marcel_ev_req_t req, 
		    int nb_ev, int option)
{
	marcel_pollinst_t inst=struct_up(req, poll_cell_t, inst);
	marcel_pollid_t ps=struct_up(server, struct poll_struct, server);
	tbx_bool_t first_call=tbx_true;

	ps->cur_cell = inst;
	if (option & MARCEL_EV_OPT_REQ_IS_GROUPED) {
		first_call=tbx_false;
	}
	if ((*ps->fastfunc)(ps, inst->arg, first_call)) {
		MARCEL_EV_REQ_SUCCESS(req);
	}
	return 0;
}

static int compat_poll_all(marcel_ev_server_t server, 
		    marcel_ev_op_t op,
		    marcel_ev_req_t req, 
		    int nb_ev, int option)
{
	marcel_pollid_t ps=struct_up(server, struct poll_struct, server);
	(*ps->func)(ps,
		    ma_nr_ready(),
		    0,
		    0);	
	return 0;
}

static int compat_poll_group(marcel_ev_server_t server, 
		      marcel_ev_op_t op,
		      marcel_ev_req_t req, 
		      int nb_ev, int option)
{
	marcel_pollid_t ps=struct_up(server, struct poll_struct, server);
	(*ps->gfunc)(ps);
	return 0;
}


TBX_SYM_WARN(marcel_pollid_create_X, "marcel_pollid_create will be removed in favor of marcel_ev_server_init");
marcel_pollid_t marcel_pollid_create_X(marcel_pollgroup_func_t g,
				       marcel_poll_func_t f,
				       marcel_fastpoll_func_t h,
				       unsigned polling_points,
				       char* name)
{
	marcel_pollid_t id;
	int index;

	LOG_IN();

	if ((index = ma_atomic_inc_return(&nb_poll_structs)) == MAX_POLL_IDS) {
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}

	id = &poll_structs[index];

	marcel_ev_server_init(&id->server, name);
	marcel_ev_server_set_poll_settings(&id->server, 
					    polling_points
					    |MARCEL_EV_POLL_AT_IDLE
					    |MARCEL_EV_POLL_AT_TIMER_SIG, 1);
	if (g) {
		id->gfunc = g;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_GROUP,
					      compat_poll_group);
	}
	if (f) {
		id->func = f;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_POLLANY,
					      compat_poll_all);
	}
	if (h) {
		id->fastfunc = h;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_POLLONE,
					      compat_poll_one);
	}

	marcel_ev_server_start(&id->server);

	mdebug("registering pollid %p (gr=%p, func=%p, fast=%p, pts=%x)\n",
	       id, g, f, h, id->server.poll_points);

	LOG_RETURN(id);
}

void marcel_poll(marcel_pollid_t id, any_t arg)
{
	poll_cell_t cell;
	struct marcel_ev_wait wait;

	cell.arg=arg;

	marcel_ev_wait(&id->server, &cell.inst, &wait, 0);
}

void marcel_force_check_polling(marcel_pollid_t id)
{
	LOG_IN();
	
	marcel_ev_poll_force(&id->server);
	
	LOG_OUT();
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

inline static void __lock_server(marcel_ev_server_t server, marcel_task_t *owner)
{
	LOG_IN();
	ma_spin_lock_softirq(&server->lock);
	ma_tasklet_disable(&server->poll_tasklet);
	server->lock_owner = owner;
	LOG_OUT();
}

inline static void __unlock_server(marcel_ev_server_t server)
{
	LOG_IN();
	server->lock_owner=NULL;
	ma_tasklet_enable(&server->poll_tasklet);
	ma_spin_unlock_softirq(&server->lock);
	LOG_OUT();
}

/* Renvoie MARCEL_SELF si on a le lock
   NULL sinon
*/
inline static marcel_task_t* ensure_lock_server(marcel_ev_server_t server)
{
	LOG_IN();
	if (server->lock_owner == MARCEL_SELF) {
		LOG_RETURN(MARCEL_SELF);
	}
	__lock_server(server, MARCEL_SELF);
	LOG_RETURN(NULL);
}

inline static void lock_server_owner(marcel_ev_server_t server,
				     marcel_task_t *owner)
{
	LOG_IN();
	__lock_server(server, owner);
	LOG_OUT();
}

inline static void restore_lock_server_locked(marcel_ev_server_t server, 
					      marcel_task_t *old_owner)
{
	LOG_IN();
	if (!old_owner) {
		__unlock_server(server);
	}
	LOG_OUT();
}

inline static void restore_lock_server_unlocked(marcel_ev_server_t server,
					        marcel_task_t *old_owner)
{
	LOG_IN();
	if (old_owner) {
		__lock_server(server, old_owner);
	}
	LOG_OUT();

}

/* Utilisé par l'application */
int marcel_ev_lock(marcel_ev_server_t server)
{
	LOG_IN();
#ifdef MA__DEBUG	
	/* Ce lock n'est pas réentrant ! Vérifier l'appli */
	MA_BUG_ON (server->lock_owner == MARCEL_SELF);
#endif
	__lock_server(server, MARCEL_SELF);
	LOG_RETURN(0);
}

/* Utilisé par l'application */
int marcel_ev_unlock(marcel_ev_server_t server)
{
	LOG_IN();
#ifdef MA__DEBUG	
	/* On doit avoir le lock pour le relâcher */
	MA_BUG_ON (server->lock_owner != MARCEL_SELF);
#endif
	__unlock_server(server);
	LOG_RETURN(0);
}

/****************************************************************
 * Gestion des événements signalés OK par les call-backs
 *
 */

marcel_ev_req_t marcel_ev_get_success(marcel_ev_server_t server)
{
	marcel_ev_req_t req=NULL;
	LOG_IN();
	ma_spin_lock_softirq(&server->req_success_lock);
	if (!list_empty(&server->list_req_success)) {
		req=list_entry(server->list_req_success.next, 
			      struct marcel_ev_req, chain_req_success);
		list_del_init(&req->chain_req_success);
		mdebug("Getting success req %p pour [%s]\n", req, server->name);
	}
	ma_spin_unlock_softirq(&server->req_success_lock);
	LOG_RETURN(req);
}

inline static int __del_success_req(marcel_ev_server_t server, 
				    marcel_ev_req_t req)
{
	LOG_IN();
	if (list_empty(&req->chain_req_success)) {
		/* On n'est pas enregistré */
		LOG_RETURN(0);
	}
	ma_spin_lock(&server->req_success_lock);
	list_del_init(&req->chain_req_success);
	mdebug("Removing success ev %p pour [%s]\n", req, server->name);
	ma_spin_unlock(&server->req_success_lock);	
	
	LOG_RETURN(0);
}

inline static int __add_success_req(marcel_ev_server_t server,
				     marcel_ev_req_t req)
{
	LOG_IN();
	if (!list_empty(&req->chain_req_success)) {
		/* On est déjà enregistré */
		LOG_RETURN(0);
	}
	ma_spin_lock(&server->req_success_lock);
	mdebug("Adding success req %p pour [%s]\n", req, server->name);
	list_add_tail(&req->chain_req_success, &server->list_req_success);
	ma_spin_unlock(&server->req_success_lock);
	LOG_RETURN(0);
}

/* Réveils pour l'événement */
inline static int __wake_req_waiters(marcel_ev_server_t server,
				     marcel_ev_req_t req, int code)
{
	marcel_ev_wait_t wait, tmp;
	LOG_IN();
	FOREACH_WAIT_BASE_SAFE(wait, tmp, req) {
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
inline static int __wake_id_waiters(marcel_ev_server_t server, int code)
{
	marcel_ev_wait_t wait, tmp;
	LOG_IN();
	list_for_each_entry_safe(wait, tmp, &server->list_id_waiters, chain_wait) {
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

inline static int __unregister_poll(marcel_ev_server_t server,
				    marcel_ev_req_t req);
inline static int __unregister(marcel_ev_server_t server,
			       marcel_ev_req_t req);
/* On gère les événements signalés OK par les call-backs
 * - retrait événtuel de la liste des ev groupés (que l'on compte)
 * - réveil des waiters sur les événements et le serveur
 */
inline static int __manage_ready(marcel_ev_server_t server)
{
	int nb_grouped_req_removed = 0;
	int nb_req_ask_wake_server = 0;
	marcel_ev_req_t req, tmp;

	//LOG_IN();
	if (list_empty(&server->list_req_ready)) {
		//LOG_RETURN(0);
		return 0;
	}

	list_for_each_entry_safe(req, tmp, &server->list_req_ready,
				 chain_req_ready) {
		mdebug("Poll succeed with req %p\n", req);
		req->state |= MARCEL_EV_STATE_OCCURED;

		__wake_req_waiters(server, req, 0);
		if (! (req->state & MARCEL_EV_STATE_NO_WAKE_SERVER)) {
			__add_success_req(server, req);
			nb_req_ask_wake_server++;
		}

		list_del_init(&req->chain_req_ready);
		if (req->state & MARCEL_EV_STATE_ONE_SHOT) {
			nb_grouped_req_removed+=__unregister_poll(server, req);
			__unregister(server, req);
		}
	}
	if (nb_grouped_req_removed) {
		mdebug("Nb grouped task set to %i\n", 
		       server->req_poll_grouped_nb);
	}
	
	if (nb_req_ask_wake_server) {
		__wake_id_waiters(server, nb_req_ask_wake_server);
	}

#ifdef MA__DEBUG
	MA_BUG_ON(!list_empty(&server->list_req_ready));
#endif
	//LOG_RETURN(nb_grouped_req_removed);
	return 0;
}

/* On groupe si nécessaire, ie :
 * s'il y a  au moins 2 requetes
 * ou s'il n'y a pas de "fast poll", alors il faut factoriser.
 *
 * Il doit y avoir au moins une tâche à grouper.
 */
inline static int __poll_group(marcel_ev_server_t server, marcel_ev_req_t req)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!server->req_poll_grouped_nb);
#endif
	if (server->funcs[MARCEL_EV_FUNCTYPE_POLL_GROUP] &&
	    ( server->req_poll_grouped_nb > 1 
	      || ! server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE])) {
		mdebug("Factorizing %i polling(s) with POLL_GROUP for [%s]\n",
		       server->req_poll_grouped_nb, server->name);
		(*server->funcs[MARCEL_EV_FUNCTYPE_POLL_GROUP])
			(server, MARCEL_EV_FUNCTYPE_POLL_POLLONE,
			 req, server->req_poll_grouped_nb, 0);
		return 1;
	}
	mdebug("No need to group %i polling(s) for [%s]\n",
	       server->req_poll_grouped_nb, server->name);
	return 0;
}

inline static int __need_poll(marcel_ev_server_t server)
{
	return server->req_poll_grouped_nb;
}

/* On démarre un polling (et éventuellement le timer) en l'ajoutant
 * dans la liste
 */
inline static void __poll_start(marcel_ev_server_t server)
{
	ma_write_lock_softirq(&ev_poll_lock);
	mdebug("Starting polling for %s\n", server->name);
	MA_BUG_ON(!list_empty(&server->chain_poll));
	list_add(&server->chain_poll, &ma_ev_list_poll);
	if (server->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebug("Starting timer polling for [%s] at frequency %i\n",
		       server->name, server->frequency);
		/* ma_mod_timer des fois qu'un ancien soit toujours
		 * pending... */
		ma_mod_timer(&server->poll_timer, ma_jiffies+server->frequency);
	}
	ma_write_unlock_softirq(&ev_poll_lock);
}

/* Plus rien à scruter 
 * reste à
 * + arrêter le timer
 * + enlever le serveur de la liste des scrutations en cours
 */
inline static void __poll_stop(marcel_ev_server_t server)
{
	ma_write_lock_softirq(&ev_poll_lock);
	mdebug("Stopping polling for [%s]\n", server->name);
	list_del_init(&server->chain_poll);
	if (server->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebug("Stopping timer polling for [%s]\n", server->name);
		ma_del_timer(&server->poll_timer);
	}
	ma_write_unlock_softirq(&ev_poll_lock);
}

/* On vient de faire une scrutation complète (pas un POLL_ONE sur le
 * premier), on réenclanche un timer si nécessaire.
 */
inline static void __update_timer(marcel_ev_server_t server)
{
	if (server->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebugl(7, "Update timer polling for [%s] at frequency %i\n",
			server->name, server->frequency);
		ma_mod_timer(&server->poll_timer, ma_jiffies+server->frequency);
	}
}

//inline static void __ensure_timer(marcel_ev_server_t server)
//{
//	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
//		ma_debug(polling,
//			 "Ensuring timer polling for [%s] at frequency %i\n",
//			 id->name, id->frequency);
//		ma_mod_timer(&id->poll_timer, id->poll_timer.expires);
//	}
//}

// Checks to see if some polling jobs should be done.
static void check_polling_for(marcel_ev_server_t server)
{
	int nb=__need_poll(server);
#ifdef MA__DEBUG
	static int count=0;

	mdebugl(7, "Check polling for [%s]\n", server->name);
	
	if (! count--) {
		mdebugl(6, "Check polling 50000 for [%s]\n", server->name);
		count=50000;
	}
#endif

	if (!nb) 
		return;

	server->registered_req_not_yet_polled=0;
	if(nb == 1 && server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE]) {
		(*server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE])
			(server, MARCEL_EV_FUNCTYPE_POLL_POLLONE, 
			 list_entry(server->list_req_poll_grouped.next,
				    struct marcel_ev_req, chain_req_grouped),
			 nb, MARCEL_EV_OPT_REQ_IS_GROUPED);

	} else if (server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLANY]) {
		(*server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLANY])
			(server, MARCEL_EV_FUNCTYPE_POLL_POLLANY,
			 NULL, nb, 0);
		
	} else if (server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE]) {
		marcel_ev_req_t req;
		FOREACH_REQ_POLL_BASE(req, server){
			(*server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE])
				(server, MARCEL_EV_FUNCTYPE_POLL_POLLONE,
				 req, nb,
				 MARCEL_EV_OPT_REQ_IS_GROUPED
				 |MARCEL_EV_OPT_REQ_ITER);
		}
	} else {
		/* Pas de méthode disponible ! */
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}

	__manage_ready(server);
	if (nb != __need_poll(server)) {
		/* Le nombre de tache en cours de polling a varié */
		if(!__need_poll(server)) {
			__poll_stop(server);
			return;
		} else {
			__poll_group(server, NULL);
		}
	}
	__update_timer(server);
	return;
}

/* call-back pour la tasklet */
void marcel_poll_from_tasklet(unsigned long hid)
{
	marcel_ev_server_t server=(marcel_ev_server_t)hid;
	mdebugl(7, "Tasklet function for [%s]\n", server->name);
	check_polling_for(server);
	return;
}

/* call-back pour le timer */
void marcel_poll_timer(unsigned long hid)
{
	marcel_ev_server_t server=(marcel_ev_server_t)hid;
	mdebugl(7, "Timer function for [%s]\n", server->name);
	ma_tasklet_schedule(&server->poll_tasklet);
	return;
}

/* Appelé par le code en divers points (yield et lib_entry
 * principalement) */
void __marcel_check_polling(unsigned polling_point)
{

	marcel_ev_server_t server;

//	if(polling_point == MARCEL_EV_POLL_AT_IDLE)
		marcel_per_lwp_polling_proceed();	
	
	//debug("Checking polling at %i\n", polling_point);
	ma_read_lock_softirq(&ev_poll_lock);
	list_for_each_entry(server, &ma_ev_list_poll, chain_poll) {
		if (server->poll_points & polling_point) {
			mdebugl(7, "Scheduling polling for [%s] at point %i\n",
				server->name, polling_point);
			ma_tasklet_schedule(&server->poll_tasklet);
		}
	}
	ma_read_unlock_softirq(&ev_poll_lock);
}

/* Force une scrutation sur un serveur */
void marcel_ev_poll_force(marcel_ev_server_t server)
{
	LOG_IN();
	mdebug("Poll forced for [%s]\n", server->name);
	/* Pas très important si on loupe quelque chose ici (genre
	 * liste modifiée au même instant)
	 */
	if (!__need_poll(server)) {
		ma_tasklet_schedule(&server->poll_tasklet);
	}
	LOG_OUT();
}

/* Force une scrutation synchrone sur un serveur */
void marcel_ev_poll_force_sync(marcel_ev_server_t server)
{
	marcel_task_t *lock;
	LOG_IN();
	mdebug("Sync poll forced for [%s]\n", server->name);

	/* On se synchronise */
	lock=ensure_lock_server(server);

	check_polling_for(server);
	
	restore_lock_server_locked(server, lock);
	LOG_OUT();
}

/****************************************************************
 * Attente d'événement
 *
 * Remarque: facile à scinder en deux si on veut de l'attente
 * asynchrone. Ne pas oublier alors les impératifs de locking.
 */


inline static void verify_server_state(marcel_ev_server_t server) {
#ifdef MA__DEBUG
	MA_BUG_ON(server->state!=MA_EV_SERVER_STATE_LAUNCHED);
#endif
}

inline static void __init_req(marcel_ev_req_t req)
{
	mdebug("Clearing Grouping request %p\n", req);
	INIT_LIST_HEAD(&req->chain_req_registered);
	INIT_LIST_HEAD(&req->chain_req_grouped);
	INIT_LIST_HEAD(&req->chain_req_ready);
	INIT_LIST_HEAD(&req->chain_req_success);
	req->state=0;
	req->server=NULL;
	INIT_LIST_HEAD(&req->list_wait);
}

inline static int __register(marcel_ev_server_t server, marcel_ev_req_t req)
{

	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	mdebug("Register req %p for [%s]\n", req, server->name);
	MA_BUG_ON(req->server);
	list_add(&req->chain_req_registered, &server->list_req_registered);
	req->state |= MARCEL_EV_STATE_REGISTERED;
	req->server=server;
	LOG_RETURN(0);
}

inline static int __unregister(marcel_ev_server_t server, marcel_ev_req_t req)
{
	LOG_IN();


	mdebug("Unregister request %p for [%s]\n", req, server->name);
	__del_success_req(server, req);

	list_del_init(&req->chain_req_registered);
	req->state &= ~MARCEL_EV_STATE_REGISTERED;
	LOG_RETURN(0);
}

inline static int __register_poll(marcel_ev_server_t server,
				  marcel_ev_req_t req)
{

	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	mdebug("Grouping Poll request %p for [%s]\n", req, server->name);
	list_add(&req->chain_req_grouped, &server->list_req_poll_grouped);
	server->req_poll_grouped_nb++;
	req->state |= MARCEL_EV_STATE_GROUPED;

	__poll_group(server, req);
	
	if (server->req_poll_grouped_nb == 1) {
		__poll_start(server);
	}
	LOG_RETURN(0);
}

inline static int __unregister_poll(marcel_ev_server_t server,
				    marcel_ev_req_t req)
{
	LOG_IN();
	if (req->state & MARCEL_EV_STATE_GROUPED) {
		mdebug("Ungrouping Poll request %p for [%s]\n", req, server->name);
		list_del_init(&req->chain_req_grouped);
		server->req_poll_grouped_nb--;
		req->state &= ~MARCEL_EV_STATE_GROUPED;
		LOG_RETURN(1);
	}
	LOG_RETURN(0);
}


int marcel_req_init(marcel_ev_req_t req)
{
	LOG_IN();
	__init_req(req);
	LOG_RETURN(0);
}

/* Ajout d'un attribut spécifique à un événement */
int marcel_req_attr_set(marcel_ev_req_t req, int attr)
{
	LOG_IN();
	if (attr & MARCEL_EV_ATTR_ONE_SHOT) {
		req->state |= MARCEL_EV_STATE_ONE_SHOT;
	}
	if (attr & MARCEL_EV_ATTR_NO_WAKE_SERVER) {
		req->state |= MARCEL_EV_STATE_NO_WAKE_SERVER;
	}
	if (attr & (~(MARCEL_EV_ATTR_ONE_SHOT|MARCEL_EV_ATTR_NO_WAKE_SERVER))) {
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);
	}
	LOG_RETURN(0);
}

/* Enregistrement d'un événement */
int marcel_ev_req_submit(marcel_ev_server_t server, marcel_ev_req_t req)
{
	marcel_task_t *lock;
	LOG_IN();

	lock=ensure_lock_server(server);

	verify_server_state(server);
	MA_BUG_ON(req->state & MARCEL_EV_STATE_REGISTERED);

/*Entre en conflit avec le test de la fonction __register*/
//	req->server=server;

	server->registered_req_not_yet_polled++;

	__register(server, req);
	__register_poll(server, req);
	
	MA_BUG_ON(!(req->state & MARCEL_EV_STATE_REGISTERED));
	restore_lock_server_locked(server, lock);

	LOG_RETURN(0);
}

/* Abandon d'un événement et retour des threads en attente sur cet événement */
int marcel_ev_req_cancel(marcel_ev_req_t req, int ret_code)
{
	marcel_task_t *lock;
	marcel_ev_server_t server;
	LOG_IN();

	MA_BUG_ON(!(req->state & MARCEL_EV_STATE_REGISTERED));
	server=req->server;

	lock=ensure_lock_server(server);

	verify_server_state(server);

	__wake_req_waiters(server, req, ret_code);

	if (__unregister_poll(server, req)) {
		if (__need_poll(server)) {
			__poll_group(server, NULL);
		} else {
			__poll_stop(server);
		}
	}
	__unregister(server, req);

	restore_lock_server_locked(server, lock);

	LOG_RETURN(0);
}

inline static int __wait_req(marcel_ev_server_t server, marcel_ev_req_t req, 
			     marcel_ev_wait_t wait, marcel_time_t timeout)
{
	LOG_IN();

	if (timeout) {
		MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
	}

	list_add(&wait->chain_wait, &req->list_wait);
	marcel_sem_init(&wait->sem, 0);
	wait->ret=0;
	wait->task=MARCEL_SELF;

	__unlock_server(server);

	marcel_sem_P(&wait->sem);

	LOG_RETURN(wait->ret);
}

/* Attente d'un thread sur un événement déjà enregistré */
int marcel_ev_req_wait(marcel_ev_req_t req, marcel_ev_wait_t wait,
		       marcel_time_t timeout)
{
	marcel_task_t *lock;
	int ret=0;
	marcel_ev_server_t server;
	LOG_IN();

	MA_BUG_ON(!(req->state & MARCEL_EV_STATE_REGISTERED));
	server=req->server;

	lock=ensure_lock_server(server);

	verify_server_state(server);

	req->state &= ~MARCEL_EV_STATE_OCCURED;

	/* TODO: On pourrait faire un check juste sur cet événement 
	 * (au moins quand on itère avec un poll_one sur tous les req) 
	 */
	check_polling_for(server);
	
	if (!(req->state & MARCEL_EV_STATE_OCCURED)) {
		ret=__wait_req(server, req, wait, timeout);
		restore_lock_server_unlocked(server, lock);
	} else {
		restore_lock_server_locked(server, lock);
	}

	LOG_RETURN(ret);
}

/* Attente d'un thread sur un quelconque événement du serveur */
int marcel_ev_server_wait(marcel_ev_server_t server, marcel_time_t timeout)
{
	marcel_task_t *lock;
	struct marcel_ev_wait wait;
	LOG_IN();

	lock=ensure_lock_server(server);
	verify_server_state(server);

	if (timeout) {
		MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
	}
	
	list_add(&wait.chain_wait, &server->list_id_waiters);
	marcel_sem_init(&wait.sem, 0);
	wait.ret=0;
#ifdef MA__DEBUG
	wait.task=MARCEL_SELF;
#endif
	/* TODO: on pourrait ne s'enregistrer que si le poll ne fait rien */
	check_polling_for(server);

	__unlock_server(server);

	marcel_sem_P(&wait.sem);

	restore_lock_server_unlocked(server, lock);

	LOG_RETURN(wait.ret);
}

/* Enregistrement, attente et désenregistrement d'un événement */
int marcel_ev_wait(marcel_ev_server_t server, marcel_ev_req_t req, 
		   marcel_ev_wait_t wait, marcel_time_t timeout)
{
	marcel_task_t *lock;
	int checked=0;
	int waken_up=0;
	LOG_IN();

	lock=ensure_lock_server(server);

	verify_server_state(server);

	__init_req(req);
	__register(server, req);

	mdebug("Marcel_poll (thread %p)...\n", marcel_self());
	mdebug("using pollid [%s]\n", server->name);

	req->state |= MARCEL_EV_STATE_ONE_SHOT|MARCEL_EV_STATE_NO_WAKE_SERVER;

	if (server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE]) {
		mdebug("Using Immediate POLL_ONE\n");
		(*server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE])
			(server, MARCEL_EV_FUNCTYPE_POLL_POLLONE,
			 req, server->req_poll_grouped_nb, 0);
		waken_up=__need_poll(server);
		__manage_ready(server);
		waken_up -= __need_poll(server);
		checked=1;
	}
	
	if (req->state & MARCEL_EV_STATE_OCCURED) {
		if (waken_up) {
			/* Le nombre de tache en cours de polling a varié */
			if (__need_poll(server)) {
				__poll_group(server, NULL);
			} else {
				__poll_stop(server);
			}
		}
		/* Pas update_timer(id) car on a fait un poll_one et
		 * pas poll_all */
		restore_lock_server_locked(server, lock);
		LOG_RETURN(0);
	}

	__register_poll(server, req);

	if (!checked) {
		check_polling_for(server);
	}

	if (!(req->state & MARCEL_EV_STATE_OCCURED)) {
		__wait_req(server, req, wait, timeout);
		lock_server_owner(server, lock);
	}

	__unregister(server, req);

	restore_lock_server_locked(server, lock);
	
	LOG_RETURN(wait->ret);
}

/****************************************************************
 * Fonctions d'initialisation/terminaison
 */
void marcel_ev_server_init(marcel_ev_server_t server, char* name)
{
        *server=(struct marcel_ev_server)MARCEL_EV_SERVER_INIT(*server, name);
}

int marcel_ev_server_set_poll_settings(marcel_ev_server_t server, 
				       unsigned poll_points,
				       unsigned frequency)
{
#ifdef MA__DEBUG
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(server->state != MA_EV_SERVER_STATE_INIT);
#endif
	server->poll_points=poll_points;
	if (poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		frequency=frequency?:1;
		server->frequency = frequency * MA_JIFFIES_PER_TIMER_TICK;
	}
	return 0;	
}

int marcel_ev_server_start(marcel_ev_server_t server)
{
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(server->state != MA_EV_SERVER_STATE_INIT);
	if (!server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLONE]
	    && !server->funcs[MARCEL_EV_FUNCTYPE_POLL_POLLANY]) {
		mdebug("One poll function needed for [%s]\n", server->name);
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
	server->state=MA_EV_SERVER_STATE_LAUNCHED;
	return 0;
}

int marcel_ev_server_stop(marcel_ev_server_t server)
{
	marcel_task_t *lock;
	marcel_ev_req_t req, tmp;
	LOG_IN();

	lock=ensure_lock_server(server);
	verify_server_state(server);

	server->state=MA_EV_SERVER_STATE_HALTED;

#ifndef ECANCELED
#define ECANCELED EIO
#endif
	list_for_each_entry_safe(req, tmp, 
				 &server->list_req_registered, 
				 chain_req_registered) {
		__wake_req_waiters(server, req, -ECANCELED);
		__unregister_poll(server, req);
		__unregister(server, req);
	}
	__wake_id_waiters(server, -ECANCELED);

	restore_lock_server_unlocked(server, lock);

	LOG_RETURN(0);
	
	return 0;
}

