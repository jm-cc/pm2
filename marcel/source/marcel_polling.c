
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
 * Variable contenant la liste chainée des serveurs de polling ayant
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

// Checks to see if some polling jobs should be done.
static void check_polling_for(marcel_ev_server_t server)
{
	int nb=__need_poll(server);
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
		__lock_server(server, lock);
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

