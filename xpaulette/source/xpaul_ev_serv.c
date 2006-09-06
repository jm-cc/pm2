
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

#define XPAUL_FILE_DEBUG xpaul_ev_serv
#include "xpaul.h"
#ifdef MARCEL
#include "marcel.h"
#include "linux_spinlock.h"
#include "linux_interrupt.h"
#endif				// MARCEL

#include <errno.h>

/****************************************************************
 * Voir xpaul_io.c pour un exemple d'implémentation
 */

/****************************************************************
 * Compatibility stuff
 * Cette partie disparaîtra bientôt...
 */
#define MAX_POLL_IDS    16


// TODO : dupliquer les fonctions de lock, unlock pour support sans Marcel
#ifdef MARCEL
typedef struct per_lwp_polling_s {
	int *data;
	int value_to_match;
	void (*func) (void *);
	void *arg;
	marcel_t task;
	tbx_bool_t blocked;
	struct xpaul_per_lwp_polling_s *next;
} xpaul_per_lwp_polling_t;
#endif				// MARCEL
/****************************************************************
 * Implémentation courante
 ****************************************************************/

/* Variable contenant la liste chainée des serveurs de polling ayant
 * quelque chose à scruter.
 * Le lock permet de protéger le parcours/modification de la liste
 */
LIST_HEAD(xpaul_list_poll);
#ifdef MARCEL
static ma_rwlock_t xpaul_poll_lock = MA_RW_LOCK_UNLOCKED;
#endif
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
 *     (exemple marcel_xpaul_wait())
 */
#ifdef MARCEL
inline static void __xpaul_lock_server(xpaul_server_t server,
				       marcel_task_t * owner)
{
	ma_spin_lock_softirq(&server->lock);
	ma_tasklet_disable(&server->poll_tasklet);
	server->lock_owner = owner;
}

inline static void __xpaul_unlock_server(xpaul_server_t server)
{
	server->lock_owner = NULL;
	ma_tasklet_enable(&server->poll_tasklet);
	ma_spin_unlock_softirq(&server->lock);
}

/* Renvoie MARCEL_SELF si on a le lock
   NULL sinon
*/
inline static marcel_task_t *xpaul_ensure_lock_server(xpaul_server_t
						      server)
{
	if (server->lock_owner == MARCEL_SELF) {
		LOG_RETURN(MARCEL_SELF);
	}
	__xpaul_lock_server(server, MARCEL_SELF);
	return NULL;
}

inline static void xpaul_lock_server_owner(xpaul_server_t server,
					   marcel_task_t * owner)
{
	__xpaul_lock_server(server, owner);
}

inline static void xpaul_restore_lock_server_locked(xpaul_server_t server,
						    marcel_task_t *
						    old_owner)
{
	if (!old_owner) {
		__xpaul_unlock_server(server);
	}
}

inline static void xpaul_restore_lock_server_unlocked(xpaul_server_t
						      server,
						      marcel_task_t *
						      old_owner)
{
	if (old_owner) {
		__xpaul_lock_server(server, old_owner);
	}
}

/* Utilisé par l'application */
int xpaul_lock(xpaul_server_t server)
{
#ifdef XPAUL__DEBUG
	/* Ce lock n'est pas réentrant ! Vérifier l'appli */
	XPAUL_BUG_ON(server->lock_owner == MARCEL_SELF);
#endif
	__xpaul_lock_server(server, MARCEL_SELF);
	return 0;
}

/* Utilisé par l'application */
int xpaul_unlock(xpaul_server_t server)
{
#ifdef MA__DEBUG
	/* On doit avoir le lock pour le relâcher */
	XPAUL_BUG_ON(server->lock_owner != MARCEL_SELF);
#endif
	__xpaul_unlock_server(server);
	return 0;
}

#define xpaul_spin_lock_softirq(lock) ma_spin_lock_softirq(lock)
#define xpaul_spin_unlock_softirq(lock) ma_spin_unlock_softirq(lock)
#define xpaul_write_lock_softirq(lock) ma_write_lock_softirq(lock)
#define xpaul_write_unlock_softirq(lock) ma_write_unlock_softirq(lock)
#define xpaul_read_lock_softirq(lock) ma_read_lock_softirq(&xpaul_poll_lock)
#define xpaul_read_unlock_softirq(lock) ma_read_unlock_softirq(&xpaul_poll_lock)

#define xpaul_spin_lock(lock) ma_spin_lock(lock)
#define xpaul_spin_unlock(lock) ma_spin_unlock(lock)

#else

#define __xpaul_lock_server(server, owner) (void) 0
#define __xpaul_unlock_server(server) (void) 0
#define xpaul_ensure_lock_server(server) (void) 0
#define xpaul_lock_server_owner(server, owner) (void) 0
#define xpaul_restore_lock_server_locked(server, old_owner) (void) 0
#define xpaul_restore_lock_server_unlocked(server, old_owner) (void) 0
#define xpaul_lock(server) (void) 0
#define xpaul_unlock(server) (void) 0

#define xpaul_spin_lock_softirq(lock) (void) 0
#define xpaul_spin_unlock_softirq(lock) (void) 0
#define xpaul_write_lock_softirq(lock) (void) 0
#define xpaul_write_unlock_softirq(lock) (void) 0
#define xpaul_read_lock_softirq(lock) (void) 0
#define xpaul_read_unlock_softirq(lock) (void) 0

#define xpaul_spin_lock(lock) (void) 0
#define xpaul_spin_unlock(lock) (void) 0
#endif				// MARCEL
/****************************************************************
 * Gestion des événements signalés OK par les call-backs
 *
 */

xpaul_req_t xpaul_get_success(xpaul_server_t server)
{
	LOG_IN();
	xpaul_req_t req = NULL;
	xpaul_spin_lock_softirq(&server->req_success_lock);

	if (!list_empty(&server->list_req_success)) {
		req = list_entry(server->list_req_success.next,
				 struct xpaul_req, chain_req_success);
		list_del_init(&req->chain_req_success);
		xdebug("Getting success req %p pour [%s]\n", req,
		       server->name);
	}

	xpaul_spin_unlock_softirq(&server->req_success_lock);
	LOG_RETURN(req);
}

inline static int __xpaul_del_success_req(xpaul_server_t server,
					  xpaul_req_t req)
{
	LOG_IN();
	if (list_empty(&req->chain_req_success)) {
		/* On n'est pas enregistré */
		LOG_RETURN(0);
	}
	xpaul_spin_lock(&server->req_success_lock);
	list_del_init(&req->chain_req_success);
	xdebug("Removing success ev %p pour [%s]\n", req, server->name);
	xpaul_spin_unlock(&server->req_success_lock);

	LOG_RETURN(0);
}

inline static int __xpaul_add_success_req(xpaul_server_t server,
					  xpaul_req_t req)
{
	LOG_IN();
	if (!list_empty(&req->chain_req_success)) {
		/* On est déjà enregistré */
		LOG_RETURN(0);
	}
	xpaul_spin_lock(&server->req_success_lock);

	xdebug("Adding success req %p pour [%s]\n", req, server->name);
	list_add_tail(&req->chain_req_success, &server->list_req_success);
	xpaul_spin_unlock(&server->req_success_lock);

	LOG_RETURN(0);
}

/* Réveils pour l'événement */
inline static int __xpaul_wake_req_waiters(xpaul_server_t server,
					   xpaul_req_t req, int code)
{
	xpaul_wait_t wait, tmp;
	LOG_IN();
	FOREACH_WAIT_BASE_SAFE(wait, tmp, req) {
#ifdef MA__DEBUG
		switch (code) {
		case 0:
			xdebug("Poll succeed with task %p\n", wait->task);
			break;
		default:
			xdebug("Poll awake with task %p on code %i\n",
			       wait->task, code);
		}
#endif
		wait->ret = code;
		list_del_init(&wait->chain_wait);
#ifdef MARCEL
		marcel_sem_V(&wait->sem);
#endif				// MARCEL
	}
	LOG_RETURN(0);
}


/* On groupe si nécessaire, ie :
 * s'il y a  au moins 2 requetes
 * ou s'il n'y a pas de "fast poll", alors il faut factoriser.
 *
 * Il doit y avoir au moins une tâche à grouper.
 */
inline static int __xpaul_poll_group(xpaul_server_t server,
				     xpaul_req_t req)
{
#ifdef MA__DEBUG
	XPAUL_BUG_ON(!server->req_poll_grouped_nb);
#endif
	if (server->funcs[XPAUL_FUNCTYPE_POLL_GROUP].func &&
	    (server->req_poll_grouped_nb > 1
	     || !server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func)) {
		xdebug
		    ("Factorizing %i polling(s) with POLL_GROUP for [%s]\n",
		     server->req_poll_grouped_nb, server->name);
		(*server->funcs[XPAUL_FUNCTYPE_POLL_GROUP].func)
		    (server, XPAUL_FUNCTYPE_POLL_POLLONE,
		     req, server->req_poll_grouped_nb, 0);
		return 1;
	}
	xdebug("No need to group %i polling(s) for [%s]\n",
	       server->req_poll_grouped_nb, server->name);
	return 0;
}

// TODO: support sans Marcel
extern unsigned long volatile ma_jiffies;

/* On démarre un polling (et éventuellement le timer) en l'ajoutant
 * dans la liste
 */
inline static void __xpaul_poll_start(xpaul_server_t server)
{
	xpaul_write_lock_softirq(&xpaul_poll_lock);
	xdebug("Starting polling for %s\n", server->name);
	XPAUL_BUG_ON(!list_empty(&server->chain_poll));
	list_add(&server->chain_poll, &xpaul_list_poll);
	if (server->poll_points & XPAUL_POLL_AT_TIMER_SIG) {
		xdebug("Starting timer polling for [%s] at period %i\n",
		       server->name, server->period);
		/* ma_mod_timer des fois qu'un ancien soit toujours
		 * pending... */
#ifdef MARCEL
		// TODO: support du timer
		ma_mod_timer(&server->poll_timer,
			     ma_jiffies + server->period);
#endif				// MARCEL
	}
	xpaul_write_unlock_softirq(&xpaul_poll_lock);
}

/* Plus rien à scruter 
 * reste à
 * + arrêter le timer
 * + enlever le serveur de la liste des scrutations en cours
 */
inline static void __xpaul_poll_stop(xpaul_server_t server)
{
	xpaul_write_lock_softirq(&xpaul_poll_lock);
	xdebug("Stopping polling for [%s]\n", server->name);
	list_del_init(&server->chain_poll);
	if (server->poll_points & XPAUL_POLL_AT_TIMER_SIG) {
		xdebug("Stopping timer polling for [%s]\n", server->name);
#ifdef MARCEL
		// TODO: support du timer sans Marcel
		ma_del_timer(&server->poll_timer);
#endif				// MARCEL
	}
	xpaul_write_unlock_softirq(&xpaul_poll_lock);
}

/* Réveils du serveur */
inline static int __xpaul_wake_id_waiters(xpaul_server_t server, int code)
{
	xpaul_wait_t wait, tmp;
	LOG_IN();
	list_for_each_entry_safe(wait, tmp, &server->list_id_waiters,
				 chain_wait) {
#ifdef MA__DEBUG
		switch (code) {
		case 0:
			xdebug("Poll succeed with global task %p\n",
			       wait->task);
			break;
		default:
			xdebug
			    ("Poll awake with global task %p on code %i\n",
			     wait->task, code);
		}
#endif
		wait->ret = code;
		list_del_init(&wait->chain_wait);
#ifdef MARCEL
		marcel_sem_V(&wait->sem);
#endif				// MARCEL
	}
	LOG_RETURN(0);
}


inline static void __xpaul_init_req(xpaul_req_t req)
{
	xdebug("Clearing Grouping request %p\n", req);
	INIT_LIST_HEAD(&req->chain_req_registered);
	INIT_LIST_HEAD(&req->chain_req_grouped);
	INIT_LIST_HEAD(&req->chain_req_ready);
	INIT_LIST_HEAD(&req->chain_req_success);
	req->state = 0;
	req->server = NULL;
	req->max_poll = 0;
	req->nb_polled = 0;
	INIT_LIST_HEAD(&req->list_wait);
}

inline static int __xpaul_register(xpaul_server_t server, xpaul_req_t req)
{

	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	xdebug("Register req %p for [%s]\n", req, server->name);
	XPAUL_BUG_ON(req->server);
	list_add(&req->chain_req_registered, &server->list_req_registered);
	req->state |= XPAUL_STATE_REGISTERED;
	req->server = server;
	LOG_RETURN(0);
}

inline static int __xpaul_unregister(xpaul_server_t server,
				     xpaul_req_t req)
{
	LOG_IN();
	xdebug("Unregister request %p for [%s]\n", req, server->name);
	__xpaul_del_success_req(server, req);

	list_del_init(&req->chain_req_registered);
	req->state &= ~XPAUL_STATE_REGISTERED;
	LOG_RETURN(0);
}

inline static int __xpaul_register_poll(xpaul_server_t server,
					xpaul_req_t req)
{
	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	xdebug("Grouping Poll request %p for [%s]\n", req, server->name);
	list_add(&req->chain_req_grouped, &server->list_req_poll_grouped);
	server->req_poll_grouped_nb++;
	req->state |= XPAUL_STATE_GROUPED;

	__xpaul_poll_group(server, req);

	if (server->req_poll_grouped_nb == 1) {
		__xpaul_poll_start(server);
	}
	LOG_RETURN(0);
}

inline static int __xpaul_unregister_poll(xpaul_server_t server,
					  xpaul_req_t req)
{
	LOG_IN();
	if (req->state & XPAUL_STATE_GROUPED) {
		xdebug("Ungrouping Poll request %p for [%s]\n", req,
		       server->name);
		list_del_init(&req->chain_req_grouped);
		server->req_poll_grouped_nb--;
		req->state &= ~XPAUL_STATE_GROUPED;
		LOG_RETURN(1);
	}
	LOG_RETURN(0);
}

/* On gère les événements signalés OK par les call-backs
 * - retrait événtuel de la liste des ev groupés (que l'on compte)
 * - réveil des waiters sur les événements et le serveur
 */
inline static int __xpaul_manage_ready(xpaul_server_t server)
{
	int nb_grouped_req_removed = 0;
	int nb_req_ask_wake_server = 0;
	xpaul_req_t req, tmp, bak;

	if (list_empty(&server->list_req_ready)) {
		return 0;
	}
	bak = NULL;

	list_for_each_entry_safe(req, tmp, &server->list_req_ready,
				 chain_req_ready) {
		if (req == bak) {
			break;
		}
		xdebug("Poll succeed with req %p\n", req);
		req->state |= XPAUL_STATE_OCCURED;

		__xpaul_wake_req_waiters(server, req, 0);
		if (!(req->state & XPAUL_STATE_NO_WAKE_SERVER)) {
			__xpaul_add_success_req(server, req);
			nb_req_ask_wake_server++;
		}

		list_del_init(&req->chain_req_ready);
		if (req->state & XPAUL_STATE_ONE_SHOT) {
			nb_grouped_req_removed +=
			    __xpaul_unregister_poll(server, req);
			__xpaul_unregister(server, req);
		}
		bak = req;
	}
	if (nb_grouped_req_removed) {
		xdebug("Nb grouped task set to %i\n",
		       server->req_poll_grouped_nb);
	}

	if (nb_req_ask_wake_server) {
		__xpaul_wake_id_waiters(server, nb_req_ask_wake_server);
	}
#ifdef MA__DEBUG
	XPAUL_BUG_ON(!list_empty(&server->list_req_ready));
#endif
	return 0;
}

inline static int __xpaul_need_poll(xpaul_server_t server)
{
	return server->req_poll_grouped_nb;
}


/* On vient de faire une scrutation complète (pas un POLL_ONE sur le
 * premier), on réenclanche un timer si nécessaire.
 */
inline static void __xpaul_update_timer(xpaul_server_t server)
{
	// TODO: Support sans Marcel
	if (server->poll_points & XPAUL_POLL_AT_TIMER_SIG) {
		xdebugl(7,
			"Update timer polling for [%s] at period %i\n",
			server->name, server->period);
#ifdef MARCEL
		ma_mod_timer(&server->poll_timer,
			     ma_jiffies + server->period);
#endif
	}
}


// Checks to see if some polling jobs should be done.
// TODO : choisir d'exporter un syscall (relancer __need_export)
static void xpaul_check_polling_for(xpaul_server_t server)
{
	int nb = __xpaul_need_poll(server);
#ifdef MA__DEBUG
	static int count = 0;

	xdebugl(7, "Check polling for [%s]\n", server->name);

	if (!count--) {
		xdebugl(6, "Check polling 50000 for [%s]\n", server->name);
		count = 50000;
	}
#endif

	if (!nb)
		return;

	server->registered_req_not_yet_polled = 0;
	if (nb == 1 && server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func) {
		(*server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func)
		    (server, XPAUL_FUNCTYPE_POLL_POLLONE,
		     list_entry(server->list_req_poll_grouped.next,
				struct xpaul_req, chain_req_grouped),
		     nb, XPAUL_OPT_REQ_IS_GROUPED);

	} else if (server->funcs[XPAUL_FUNCTYPE_POLL_POLLANY].func) {
		(*server->funcs[XPAUL_FUNCTYPE_POLL_POLLANY].func)
		    (server, XPAUL_FUNCTYPE_POLL_POLLANY, NULL, nb, 0);

	} else if (server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func) {
		xpaul_req_t req;
		FOREACH_REQ_POLL_BASE(req, server) {
			(*server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func)
			    (server, XPAUL_FUNCTYPE_POLL_POLLONE,
			     req, nb,
			     XPAUL_OPT_REQ_IS_GROUPED
			     | XPAUL_OPT_REQ_ITER);
		}
	} else {
		/* Pas de méthode disponible ! */
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}

	__xpaul_manage_ready(server);
	if (nb != __xpaul_need_poll(server)) {
		/* Le nombre de tache en cours de polling a varié */
		if (!__xpaul_need_poll(server)) {
			__xpaul_poll_stop(server);
			return;
		} else {
			__xpaul_poll_group(server, NULL);
		}
	}
	__xpaul_update_timer(server);
	return;
}

/* call-back pour la tasklet */
void xpaul_poll_from_tasklet(unsigned long hid)
{
	xpaul_server_t server = (xpaul_server_t) hid;
	xdebugl(7, "Tasklet function for [%s]\n", server->name);
	xpaul_check_polling_for(server);
	return;
}

/* call-back pour le timer */
void xpaul_poll_timer(unsigned long hid)
{
	xpaul_server_t server = (xpaul_server_t) hid;
	xdebugl(7, "Timer function for [%s]\n", server->name);
#ifdef MARCEL
	ma_tasklet_schedule(&server->poll_tasklet);
#else
	// TODO: lancement du polling
#endif
	return;
}

/* Appelé par le code en divers points (yield et lib_entry
 * principalement) */
void __xpaul_check_polling(unsigned polling_point)
{
	PROF_IN();
	xpaul_server_t server;

	//debug("Checking polling at %i\n", polling_point);
	xpaul_read_lock_softirq(&xpaul_poll_lock);
	list_for_each_entry(server, &xpaul_list_poll, chain_poll) {
		if (server->poll_points & polling_point) {
			xdebugl(7,
				"Scheduling polling for [%s] at point %i\n",
				server->name, polling_point);
#ifdef MARCEL
			ma_tasklet_schedule(&server->poll_tasklet);
#else
			//TODO: Lancement polling
#endif				// MARCEL
		}
	}
	xpaul_read_unlock_softirq(&xpaul_poll_lock);
	PROF_OUT();
}

/* Force une scrutation sur un serveur */
void xpaul_poll_force(xpaul_server_t server)
{
	LOG_IN();
	xdebug("Poll forced for [%s]\n", server->name);
	/* Pas très important si on loupe quelque chose ici (genre
	 * liste modifiée au même instant)
	 */
	if (!__xpaul_need_poll(server)) {
#ifdef MARCEL
		ma_tasklet_schedule(&server->poll_tasklet);
#else
		//TODO: Lancement polling
#endif
	}
	LOG_OUT();
}

/* Force une scrutation synchrone sur un serveur */
void xpaul_poll_force_sync(xpaul_server_t server)
{

	LOG_IN();
	xdebug("Sync poll forced for [%s]\n", server->name);
#ifdef MARCEL
	marcel_task_t *lock;
	/* On se synchronise */
	lock = xpaul_ensure_lock_server(server);
#endif
	xpaul_check_polling_for(server);

#ifdef MARCEL
	xpaul_restore_lock_server_locked(server, lock);
#endif
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
inline static void xpaul_verify_server_state(xpaul_server_t server)
{
#ifdef MA__DEBUG
	XPAUL_BUG_ON(server->state != XPAUL_SERVER_STATE_LAUNCHED);
#endif
}


int xpaul_req_init(xpaul_req_t req)
{
	LOG_IN();
	__xpaul_init_req(req);
	LOG_RETURN(0);
}

/* Ajout d'un attribut spécifique à un événement */
int xpaul_req_attr_set(xpaul_req_t req, int attr)
{
	LOG_IN();
	if (attr & XPAUL_ATTR_ONE_SHOT) {
		req->state |= XPAUL_STATE_ONE_SHOT;
	}
	if (attr & XPAUL_ATTR_NO_WAKE_SERVER) {
		req->state |= XPAUL_STATE_NO_WAKE_SERVER;
	}
	if (attr & XPAUL_ATTR_DONT_POLL_FIRST) {
		req->state |= XPAUL_STATE_DONT_POLL_FIRST;
	}
	if (attr &
	    (~
	     (XPAUL_ATTR_ONE_SHOT | XPAUL_ATTR_NO_WAKE_SERVER |
	      XPAUL_STATE_DONT_POLL_FIRST))) {
		XPAUL_EXCEPTION_RAISE(XPAUL_CONSTRAINT_ERROR);
	}
	LOG_RETURN(0);
}

/* Enregistrement d'un événement */
int xpaul_req_submit(xpaul_server_t server, xpaul_req_t req)
{
	LOG_IN();
#ifdef MARCEL
	marcel_task_t *lock;
	lock = xpaul_ensure_lock_server(server);
#endif				// MARCEL

	xpaul_verify_server_state(server);
	XPAUL_BUG_ON(req->state & XPAUL_STATE_REGISTERED);

/*Entre en conflit avec le test de la fonction __register*/
//      req->server=server;

	server->registered_req_not_yet_polled++;

	__xpaul_register(server, req);
	__xpaul_register_poll(server, req);

	XPAUL_BUG_ON(!(req->state & XPAUL_STATE_REGISTERED));
#ifdef MARCEL
	xpaul_restore_lock_server_locked(server, lock);
#endif				//MARCEL

	LOG_RETURN(0);
}

/* Abandon d'un événement et retour des threads en attente sur cet événement */
int xpaul_req_cancel(xpaul_req_t req, int ret_code)
{
#ifdef MARCEL
	marcel_task_t *lock;
#endif				//MARCEL
	xpaul_server_t server;
	LOG_IN();

	XPAUL_BUG_ON(!(req->state & XPAUL_STATE_REGISTERED));
	server = req->server;
#ifdef MARCEL
	lock = xpaul_ensure_lock_server(server);
#endif				//MARCEL
	xpaul_verify_server_state(server);

	__xpaul_wake_req_waiters(server, req, ret_code);

	if (__xpaul_unregister_poll(server, req)) {
		if (__xpaul_need_poll(server)) {
			__xpaul_poll_group(server, NULL);
		} else {
			__xpaul_poll_stop(server);
		}
	}
	__xpaul_unregister(server, req);

#ifdef MARCEL
	xpaul_restore_lock_server_locked(server, lock);
#endif				//MARCEL

	LOG_RETURN(0);
}

// TODO: support sans Marcel
inline static int __xpaul_wait_req(xpaul_server_t server, xpaul_req_t req,
				   xpaul_wait_t wait, xpaul_time_t timeout)
{
	LOG_IN();
	if (timeout) {
		XPAUL_EXCEPTION_RAISE(XPAUL_NOT_IMPLEMENTED);
	}

	list_add(&wait->chain_wait, &req->list_wait);
#ifdef MARCEL
	marcel_sem_init(&wait->sem, 0);
	wait->task = MARCEL_SELF;
#endif
	wait->ret = 0;

	__xpaul_unlock_server(server);
#ifdef MARCEL
	// TODO: utiliser pmarcel_sem_P qui peut renvoyer -1 avec errno EINT (support posix)
	marcel_sem_P(&wait->sem);
#else
	// while (!fini) fast_poll;
	int waken_up = 0;
	int checked = 0;
	do {
		req->state |=
		    XPAUL_STATE_ONE_SHOT | XPAUL_STATE_NO_WAKE_SERVER;

		if (server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func
		    && req->func_to_use != XPAUL_FUNC_SYSCALL) {
			xdebug("Using Immediate POLL_ONE\n");
			(*server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func)
			    (server, XPAUL_FUNCTYPE_POLL_POLLONE,
			     req, server->req_poll_grouped_nb, 0);
			waken_up = __xpaul_need_poll(server);
			__xpaul_manage_ready(server);
			waken_up -= __xpaul_need_poll(server);
			checked = 1;
		}

		if (req->state & XPAUL_STATE_OCCURED) {
			if (waken_up) {
				/* Le nombre de tache en cours de polling a varié */
				if (__xpaul_need_poll(server)) {
					__xpaul_poll_group(server, NULL);
				} else {
					__xpaul_poll_stop(server);
				}
			}
			/* Pas update_timer(id) car on a fait un poll_one et
			 * pas poll_all */
			LOG_RETURN(0);
		}
	} while (!(req->state & XPAUL_STATE_OCCURED));

#endif
	LOG_RETURN(wait->ret);
}

/* Attente d'un thread sur un événement déjà enregistré */
int xpaul_req_wait(xpaul_req_t req, xpaul_wait_t wait,
		   xpaul_time_t timeout)
{
#ifdef MARCEL
	marcel_task_t *lock;
#endif				// MARCEL
	int ret = 0;
	xpaul_server_t server;
	LOG_IN();

	XPAUL_BUG_ON(!(req->state & XPAUL_STATE_REGISTERED));
	server = req->server;
#ifdef MARCEL
	lock = xpaul_ensure_lock_server(server);
#endif				// MARCEL
	xpaul_verify_server_state(server);

	req->state &= ~XPAUL_STATE_OCCURED;

	/* TODO: On pourrait faire un check juste sur cet événement 
	 * (au moins quand on itère avec un poll_one sur tous les req) 
	 */
	if (!(req->state & XPAUL_STATE_DONT_POLL_FIRST))
		xpaul_check_polling_for(server);
	else
		req->state &= ~XPAUL_STATE_DONT_POLL_FIRST;

	if (!(req->state & XPAUL_STATE_OCCURED)) {
		ret = __xpaul_wait_req(server, req, wait, timeout);
#ifdef MARCEL
		xpaul_restore_lock_server_unlocked(server, lock);
	} else {
		xpaul_restore_lock_server_locked(server, lock);
#endif				// MARCEL
	}

	LOG_RETURN(ret);
}

// TODO: Support sans Marcel
/* Attente d'un thread sur un quelconque événement du serveur */
int xpaul_server_wait(xpaul_server_t server, xpaul_time_t timeout)
{
	LOG_IN();
	struct xpaul_wait wait;
#ifdef MARCEL
	marcel_task_t *lock;
	lock = xpaul_ensure_lock_server(server);
#endif				// MARCEL

	xpaul_verify_server_state(server);

	if (timeout) {
		XPAUL_EXCEPTION_RAISE(XPAUL_NOT_IMPLEMENTED);
	}

	list_add(&wait.chain_wait, &server->list_id_waiters);
#ifdef MARCEL
	marcel_sem_init(&wait.sem, 0);
#endif
	wait.ret = 0;
#ifdef MA__DEBUG
	wait.task = MARCEL_SELF;
#endif
	/* TODO: on pourrait ne s'enregistrer que si le poll ne fait rien */
	xpaul_check_polling_for(server);
#ifdef MARCEL
	__xpaul_unlock_server(server);
	// TODO: utiliser pmarcel_sem_P qui peut renvoyer -1 avec errno EINT (support posix) ?

	marcel_sem_P(&wait.sem);
	xpaul_restore_lock_server_unlocked(server, lock);
#endif				// MARCEL

	LOG_RETURN(wait.ret);
}

/* Détermine s'il est judicieux d'exporter un syscall */
int __xpaul_need_export(xpaul_server_t server, xpaul_req_t req,
			xpaul_wait_t wait, xpaul_time_t timeout)
{
  /*********************************************
   *  TODO : A prendre en compte :
   *   - la vitesse de la fonction
   *   - l'historique des requêtes
   *   - priorité de la requête, du thread ?
   *   - changement de méthode
   *********************************************/
#ifndef MARCEL
	return 0;
#else
	/* requête terminée */
	if (req->state & XPAUL_STATE_OCCURED)
		return 0;
	/* pas de méthode bloquante */
	if (!server->funcs[XPAUL_FUNCTYPE_BLOCK_WAITONE].func)
		return 0;
	/* force le polling */
	if (req->func_to_use == XPAUL_FUNC_POLLING)
		return 0;

	/* pas d'autres threads */
	PROF_EVENT1(running_threads, marcel_per_lwp_nbthreads());
	if (marcel_per_lwp_nbthreads() < 2)
		return 0;
#endif				// MARCEL
	return 1;
}

/* Enregistrement, attente et désenregistrement d'un événement */
int xpaul_wait(xpaul_server_t server, xpaul_req_t req,
	       xpaul_wait_t wait, xpaul_time_t timeout)
{
	LOG_IN();

	int checked = 0;
	int waken_up = 0;

#ifdef MARCEL
	marcel_task_t *lock;
	lock = xpaul_ensure_lock_server(server);
#endif				// MARCEL

	xpaul_verify_server_state(server);

	if (req->state & XPAUL_STATE_DONT_POLL_FIRST)
		checked = 1;
	__xpaul_init_req(req);
	__xpaul_register(server, req);

#ifdef MARCEL
	xdebug("Marcel_poll (thread %p)...\n", marcel_self());
#endif				// MARCEL
	xdebug("using pollid [%s]\n", server->name);

	req->state |= XPAUL_STATE_ONE_SHOT | XPAUL_STATE_NO_WAKE_SERVER;

	if (server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func
	    && req->func_to_use != XPAUL_FUNC_SYSCALL && !checked) {
		xdebug("Using Immediate POLL_ONE\n");
		(*server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func)
		    (server, XPAUL_FUNCTYPE_POLL_POLLONE,
		     req, server->req_poll_grouped_nb, 0);
		waken_up = __xpaul_need_poll(server);
		__xpaul_manage_ready(server);
		waken_up -= __xpaul_need_poll(server);
		checked = 1;
	}

	if (req->state & XPAUL_STATE_OCCURED) {
		if (waken_up) {
			/* Le nombre de tache en cours de polling a varié */
			if (__xpaul_need_poll(server)) {
				__xpaul_poll_group(server, NULL);
			} else {
				__xpaul_poll_stop(server);
			}
		}
		/* Pas update_timer(id) car on a fait un poll_one et
		 * pas poll_all */
#ifdef MARCEL
		xpaul_restore_lock_server_locked(server, lock);
#endif				// MARCEL
		LOG_RETURN(0);
	}

	if (__xpaul_need_export(server, req, wait, timeout)) {
		// TODO: mettre dans la liste des appels bloqués
		__xpaul_unregister(server, req);
		__xpaul_unlock_server(server);
		// TODO: gerer le regroupement de reqs
		(*server->funcs[XPAUL_FUNCTYPE_BLOCK_WAITONE].
		 func) (server, XPAUL_FUNCTYPE_POLL_POLLONE, req,
			server->req_poll_grouped_nb, 1);
#ifdef MARCEL
		xpaul_lock_server_owner(server, lock);
		__xpaul_manage_ready(server);
		xpaul_restore_lock_server_locked(server, lock);
#else
		__xpaul_manage_ready(server);
#endif				// MARCEL
		LOG_RETURN(wait->ret);
	}

	__xpaul_register_poll(server, req);

	if (!checked) {
		xpaul_check_polling_for(server);
	}

	if (!(req->state & XPAUL_STATE_OCCURED)) {
		__xpaul_wait_req(server, req, wait, timeout);
#ifdef MARCEL
		xpaul_lock_server_owner(server, lock);
#endif				// MARCEL
	}
	__xpaul_unregister(server, req);
#ifdef MARCEL
	xpaul_restore_lock_server_locked(server, lock);
#endif				// MARCEL
	LOG_RETURN(wait->ret);
}

/****************************************************************
 * Fonctions d'initialisation/terminaison
 */

static tbx_flag_t xpaul_activity = tbx_flag_clear;

int
xpaul_test_activity(void)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  result = tbx_flag_test(&xpaul_activity);
  LOG_OUT();

  return result;
}

void xpaul_server_init(xpaul_server_t server, char *name)
{
	*server = (struct xpaul_server) XPAUL_SERVER_INIT(*server, name);
}

int xpaul_server_set_poll_settings(xpaul_server_t server,
				   unsigned poll_points,
				   unsigned period, int max_poll)
{
#ifdef MA__DEBUG
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	XPAUL_BUG_ON(server->state != XPAUL_SERVER_STATE_INIT);
#endif
	server->max_poll = max_poll;
	server->poll_points = poll_points;
	if (poll_points & XPAUL_POLL_AT_TIMER_SIG) {
		period = period ? : 1;
		//TODO : virer l'horloge de Marcel
		//              server->period = period * MA_JIFFIES_PER_TIMER_TICK;
	}
	return 0;
}

int xpaul_server_start(xpaul_server_t server)
{
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	XPAUL_BUG_ON(server->state != XPAUL_SERVER_STATE_INIT);
	if (!server->funcs[XPAUL_FUNCTYPE_POLL_POLLONE].func
	    && !server->funcs[XPAUL_FUNCTYPE_POLL_POLLANY].func) {
		xdebug("One poll function needed for [%s]\n",
		       server->name);
		XPAUL_EXCEPTION_RAISE(XPAUL_PROGRAM_ERROR);
	}
	xpaul_activity = tbx_flag_set;
	server->state = XPAUL_SERVER_STATE_LAUNCHED;
	return 0;
}

int xpaul_server_stop(xpaul_server_t server)
{
	LOG_IN();
	xpaul_req_t req, tmp;

#ifdef MARCEL
	marcel_task_t *lock;
	lock = xpaul_ensure_lock_server(server);
#endif				//MARCEL

	xpaul_verify_server_state(server);
	server->state = XPAUL_SERVER_STATE_HALTED;

#ifndef ECANCELED
#define ECANCELED EIO
#endif
	list_for_each_entry_safe(req, tmp,
				 &server->list_req_registered,
				 chain_req_registered) {
		__xpaul_wake_req_waiters(server, req, -ECANCELED);
		__xpaul_unregister_poll(server, req);
		__xpaul_unregister(server, req);
	}
	__xpaul_wake_id_waiters(server, -ECANCELED);

#ifdef MARCEL
	xpaul_restore_lock_server_unlocked(server, lock);
#endif				//MARCEL

	LOG_RETURN(0);

	return 0;
}


