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
//#define EXPORT_THREADS 1
#include "pioman.h"
#ifdef MARCEL
#include "marcel.h"
#include "linux_spinlock.h"
#include "linux_interrupt.h"
#endif				// MARCEL

#include <errno.h>

/****************************************************************
 * Voir piom_io.c pour un exemple d'implémentation
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
	volatile tbx_bool_t blocked;
	struct piom_per_lwp_polling_s *next;
} piom_per_lwp_polling_t;
#endif				// MARCEL
/****************************************************************
 * Implémentation courante
 ****************************************************************/

/* Variable contenant la liste chainée des serveurs de polling ayant
 * quelque chose à scruter.
 * Le lock permet de protéger le parcours/modification de la liste
 */
LIST_HEAD(piom_list_poll);
#ifdef MARCEL
static ma_spinlock_t piom_poll_lock = MA_SPIN_LOCK_UNLOCKED;
static int job_scheduled = 0;
#else
void* piom_poll_lock;
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
 *     (exemple marcel_piom_wait())
 */
#ifdef MARCEL
__tbx_inline__ static void __piom_lock_server(piom_server_t server,
				       marcel_task_t * owner)
{
	ma_spin_lock_softirq(&server->lock);
	ma_tasklet_disable(&server->poll_tasklet);
	server->lock_owner = owner;
}

__tbx_inline__ static void __piom_trylock_server(piom_server_t server,
					  marcel_task_t * owner)
{
	ma_spin_lock_softirq(&server->lock);
	server->lock_owner = owner;
}

__tbx_inline__ static void __piom_unlock_server(piom_server_t server)
{
	server->lock_owner = NULL;
	ma_tasklet_enable(&server->poll_tasklet);
	ma_spin_unlock_softirq(&server->lock);
}
__tbx_inline__ static void __piom_tryunlock_server(piom_server_t server)
{
	server->lock_owner = NULL;
	ma_spin_unlock_softirq(&server->lock);
}

/* Renvoie MARCEL_SELF si on a le lock
   NULL sinon
*/
__tbx_inline__ static marcel_task_t *piom_ensure_lock_server(piom_server_t
						      server)
{
	if (server->lock_owner == MARCEL_SELF) {
		LOG_RETURN(MARCEL_SELF);
	}
	__piom_lock_server(server, MARCEL_SELF);
	return NULL;
}

__tbx_inline__ static marcel_task_t *piom_ensure_trylock_from_tasklet(piom_server_t server) {
	if(!server->lock_owner) {
		server->lock_owner = MARCEL_SELF;
		__piom_trylock_server(server, MARCEL_SELF);
		LOG_RETURN(MARCEL_SELF);
	} else if (server->lock_owner == MARCEL_SELF )
		LOG_RETURN(MARCEL_SELF);
	ma_tasklet_disable(&server->poll_tasklet);
	ma_spin_lock_softirq(&server->lock);
	server->lock_owner = MARCEL_SELF;
	return NULL;
}

__tbx_inline__ static marcel_task_t *piom_ensure_trylock_server(piom_server_t server)
{
	if (server->lock_owner == MARCEL_SELF) {
		LOG_RETURN(MARCEL_SELF);
	}
	__piom_trylock_server(server, MARCEL_SELF);
	return NULL;
}

__tbx_inline__ static void piom_lock_server_owner(piom_server_t server,
					   marcel_task_t * owner)
{
	__piom_lock_server(server, owner);
}

__tbx_inline__ static void piom_restore_lock_server_locked(piom_server_t server,
						    marcel_task_t *
						    old_owner)
{
	if (!old_owner) {
		__piom_unlock_server(server);
	}
}

__tbx_inline__ static void piom_restore_trylocked_from_tasklet(piom_server_t server, marcel_task_t *old_owner) {
	if(!old_owner){
			__piom_unlock_server(server);	
	} else {
		__piom_tryunlock_server(server);	
	}
}

__tbx_inline__ static void piom_restore_lock_server_trylocked(piom_server_t server,
						    marcel_task_t *
						    old_owner)
{
	if (!old_owner) {
		__piom_tryunlock_server(server);
	}
}

__tbx_inline__ static void piom_restore_lock_server_unlocked(piom_server_t
						      server,
						      marcel_task_t *
						      old_owner)
{
	if (old_owner) {
		__piom_lock_server(server, old_owner);
	}
}

/* Utilisé par l'application */
int piom_lock(piom_server_t server)
{
	/* Ce lock n'est pas réentrant ! Vérifier l'appli */
	PIOM_BUG_ON(server->lock_owner == MARCEL_SELF);

	__piom_lock_server(server, MARCEL_SELF);
	return 0;
}

/* Utilisé par l'application */
int piom_unlock(piom_server_t server)
{
	/* On doit avoir le lock pour le relâcher */
	PIOM_BUG_ON(server->lock_owner != MARCEL_SELF);

	__piom_unlock_server(server);
	return 0;
}

#define _piom_spin_lock_softirq(lock) ma_spin_lock_softirq(lock)
#define _piom_spin_unlock_softirq(lock) ma_spin_unlock_softirq(lock)
#define _piom_spin_trylock_softirq(lock) ma_spin_trylock_softirq(lock)
#define _piom_write_lock_softirq(lock) ma_write_lock_softirq(lock)
#define _piom_write_unlock_softirq(lock) ma_write_unlock_softirq(lock)
#define _piom_read_lock_softirq(lock) ma_read_lock_softirq(lock)
#define _piom_read_unlock_softirq(lock) ma_read_unlock_softirq(lock)

#define _piom_spin_lock(lock) ma_spin_lock(lock)
#define _piom_spin_unlock(lock) ma_spin_unlock(lock)

#else

#define __piom_lock_server(server, owner) (void) 0
#define __piom_unlock_server(server) (void) 0
#define _piom_spin_trylock_softirq(lock) (void) 0
#define piom_ensure_lock_server(server) (void) 0
#define piom_lock_server_owner(server, owner) (void) 0
#define piom_restore_lock_server_locked(server, old_owner) (void) 0
#define piom_restore_lock_server_unlocked(server, old_owner) (void) 0
#define piom_lock(server) (void) 0
#define piom_unlock(server) (void) 0

#define _piom_spin_lock_softirq(lock) (void) 0
#define _piom_spin_unlock_softirq(lock) (void) 0
#define _piom_write_lock_softirq(lock) (void) 0
#define _piom_write_unlock_softirq(lock) (void) 0
#define _piom_read_lock_softirq(lock) (void) 0
#define _piom_read_unlock_softirq(lock) (void) 0

#define _piom_spin_lock(lock) (void) 0
#define _piom_spin_unlock(lock) (void) 0
#endif				// MARCEL

/* Détermine s'il est judicieux d'exporter un syscall */
__tbx_inline__ static
int __piom_need_export(piom_server_t server, piom_req_t req,
			piom_wait_t wait, piom_time_t timeout)
{
  /*********************************************
   *  TODO : A prendre en compte :
   *   - la vitesse de la fonction
   *   - l'historique des requêtes
   *   - priorité de la requête, du thread ?
   *   - changement de méthode
   *********************************************/
#ifndef PIOM_BLOCKING_CALLS
	return 0;
#else
	/* requête terminée */
	if (req->state & PIOM_STATE_OCCURED)
		return 0;
	/* pas de méthode bloquante */
	if (!server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)
		return 0;
	/* force le polling */
	if (req->func_to_use == PIOM_FUNC_POLLING)
		return 0;
	
	if(req->func_to_use==PIOM_FUNC_SYSCALL)
		return 1;
	int i;
	for(i=0;i<marcel_nblwps();i++) {
		if(! ma_topo_vpdata_l(GET_LWP_BY_NUM(i)->vp_level, nb_tasks))	
			return 0;
	}
#endif				// MARCEL
	return 1;
}

void piom_req_success(piom_req_t req)
{
#ifdef MARCEL
	_piom_spin_lock_softirq(&req->server->req_ready_lock); 
#endif
	PIOM_LOGF("Req %p succeded !\n",req);
	if(req->priority >  req->server->max_priority)
		req->server->max_priority = req->priority;
	list_move(&(req)->chain_req_ready, &(req)->server->list_req_ready); 
#ifdef MARCEL
	_piom_spin_unlock_softirq(&req->server->req_ready_lock); 
#endif
}

/****************************************************************
 * Gestion des événements signalés OK par les call-backs
 *
 */

piom_req_t piom_get_success(piom_server_t server)
{
	LOG_IN();
	piom_req_t req = NULL;
	_piom_spin_lock_softirq(&server->req_success_lock);

	if (!list_empty(&server->list_req_success)) {
		req = list_entry(server->list_req_success.next,
				 struct piom_req, chain_req_success);
		list_del_init(&req->chain_req_success);
		PIOM_LOGF("Getting success req %p pour [%s]\n", req, server->name);
	}

	_piom_spin_unlock_softirq(&server->req_success_lock);
	LOG_RETURN(req);
}

__tbx_inline__ static int __piom_del_success_req(piom_server_t server,
					  piom_req_t req)
{
	LOG_IN();
	if (list_empty(&req->chain_req_success)) {
		/* On n'est pas enregistré */
		LOG_RETURN(0);
	}
	_piom_spin_lock(&server->req_success_lock);
	list_del_init(&req->chain_req_success);
	PIOM_LOGF("Removing success ev %p pour [%s]\n", req, server->name);
	_piom_spin_unlock(&server->req_success_lock);

	LOG_RETURN(0);
}

__tbx_inline__ static int __piom_add_success_req(piom_server_t server,
					  piom_req_t req)
{
	LOG_IN();
	if (!list_empty(&req->chain_req_success)) {
		/* On est déjà enregistré */
		LOG_RETURN(0);
	}
	_piom_spin_lock(&server->req_success_lock);

	PIOM_LOGF("Adding success req %p pour [%s]\n", req, server->name);
	list_add_tail(&req->chain_req_success, &server->list_req_success);
	_piom_spin_unlock(&server->req_success_lock);

	LOG_RETURN(0);
}

/* Réveils pour l'événement */
__tbx_inline__ static int __piom_wake_req_waiters(piom_server_t server,
					   piom_req_t req, int code)
{
	piom_wait_t wait, tmp;
	LOG_IN();
	FOREACH_WAIT_BASE_SAFE(wait, tmp, req) {
#ifdef PIOM__DEBUG
		switch (code) {
		case 0:
			PIOM_LOGF("Poll succeed with task %p\n", wait->task);
			break;
		default:
			PIOM_LOGF("Poll awake with task %p on code %i\n",
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
__tbx_inline__ static int __piom_poll_group(piom_server_t server,
				     piom_req_t req)
{
	PIOM_BUG_ON(!server->req_poll_grouped_nb);

	if (server->funcs[PIOM_FUNCTYPE_POLL_GROUP].func &&
	    (server->req_poll_grouped_nb > 1
	     || !server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)) {
		PIOM_LOGF
		    ("Factorizing %i polling(s) with POLL_GROUP for [%s]\n",
		     server->req_poll_grouped_nb, server->name);
		(*server->funcs[PIOM_FUNCTYPE_POLL_GROUP].func)
		    (server, PIOM_FUNCTYPE_POLL_POLLONE,
		     req, server->req_poll_grouped_nb, 0);
		return 1;
	}
	PIOM_LOGF("No need to group %i polling(s) for [%s]\n",
	       server->req_poll_grouped_nb, server->name);
	return 0;
}


__tbx_inline__ static int __piom_block_group(piom_server_t server,
				     piom_req_t req)
{
	PIOM_BUG_ON(!server->req_block_grouped_nb);

	if (server->funcs[PIOM_FUNCTYPE_BLOCK_GROUP].func &&
	    (server->req_block_grouped_nb > 1
	     || !server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)) {
		PIOM_LOGF
		    ("Factorizing %i blocking(s) with BLOCK_GROUP for [%s]\n",
		     server->req_block_grouped_nb, server->name);
		(*server->funcs[PIOM_FUNCTYPE_BLOCK_GROUP].func)
		    (server, PIOM_FUNCTYPE_BLOCK_WAITONE,
		     req, server->req_block_grouped_nb, 0);

//		req->state |= PIOM_STATE_GROUPED;
		return 1;
	}
	PIOM_LOGF("No need to group %i polling(s) for [%s]\n",
	       server->req_block_grouped_nb, server->name);
	return 0;
}

// TODO: support sans Marcel
extern unsigned long volatile ma_jiffies;

/* On démarre un polling (et éventuellement le timer) en l'ajoutant
 * dans la liste
 */
__tbx_inline__ static void __piom_poll_start(piom_server_t server)
{
	_piom_spin_lock_softirq(&piom_poll_lock);
	PIOM_LOGF("Starting polling for %s\n", server->name);
	list_add(&server->chain_poll, &piom_list_poll);
	if (server->poll_points & PIOM_POLL_AT_TIMER_SIG) {
		PIOM_LOGF("Starting timer polling for [%s] at period %i\n",
		       server->name, server->period);
		/* ma_mod_timer des fois qu'un ancien soit toujours
		 * pending... */
#ifdef MARCEL
		// TODO: support du timer
		ma_mod_timer(&server->poll_timer,
			     ma_jiffies + server->period);
#endif				// MARCEL
	}
	_piom_spin_unlock_softirq(&piom_poll_lock);
}

/* Plus rien à scruter 
 * reste à
 * + arrêter le timer
 * + enlever le serveur de la liste des scrutations en cours
 */
__tbx_inline__ static void __piom_poll_stop(piom_server_t server)
{
	_piom_spin_lock_softirq(&piom_poll_lock);
	PIOM_LOGF("Stopping polling for [%s]\n", server->name);
	list_del_init(&server->chain_poll);
	if (server->poll_points & PIOM_POLL_AT_TIMER_SIG) {
		PIOM_LOGF("Stopping timer polling for [%s]\n", server->name);
#ifdef MARCEL
		// TODO: support du timer sans Marcel
		ma_del_timer(&server->poll_timer);
#endif				// MARCEL
	}
	_piom_spin_unlock_softirq(&piom_poll_lock);
}

/* Réveils du serveur */
__tbx_inline__ static int __piom_wake_id_waiters(piom_server_t server, int code)
{
	piom_wait_t wait, tmp;
	LOG_IN();
	list_for_each_entry_safe(wait, tmp, &server->list_id_waiters,
				 chain_wait) {
#ifdef PIOM__DEBUG
		switch (code) {
		case 0:
			PIOM_LOGF("Poll succeed with global task %p\n",
			       wait->task);
			break;
		default:
			PIOM_LOGF
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


__tbx_inline__ static void __piom_init_req(piom_req_t req)
{
	PIOM_LOGF("Clearing Grouping request %p\n", req);
	INIT_LIST_HEAD(&req->chain_req_registered);
	INIT_LIST_HEAD(&req->chain_req_grouped);
#ifdef PIOM_BLOCKING_CALLS
	INIT_LIST_HEAD(&req->chain_req_block_grouped);
	INIT_LIST_HEAD(&req->chain_req_to_export);
#endif /* PIOM_BLOCKING_CALLS */
	INIT_LIST_HEAD(&req->chain_req_ready);
	INIT_LIST_HEAD(&req->chain_req_success);
	
	req->state = 0;
	req->server = NULL;
	req->max_poll = 0;
	req->nb_polled = 0;

	req->func_to_use=PIOM_FUNC_AUTO;
	req->priority=PIOM_REQ_PRIORITY_NORMAL;

	INIT_LIST_HEAD(&req->list_wait);
}

#ifdef PIOM_BLOCKING_CALLS
__tbx_inline__ static void __piom_init_lwp(piom_comm_lwp_t lwp)
{
	INIT_LIST_HEAD(&lwp->chain_lwp_ready);
	INIT_LIST_HEAD(&lwp->chain_lwp_working);
	lwp->vp_nb = -1;
	lwp->fds[0] = lwp->fds[1] = -1;
	lwp->server = NULL;
	lwp->pid = NULL;
}
#endif /* PIOM_BLOCKING_CALLS */
__tbx_inline__ static int __piom_register(piom_server_t server, piom_req_t req)
{

	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	PIOM_LOGF("Register req %p for [%s]\n", req, server->name);

	list_add(&req->chain_req_registered, &server->list_req_registered);
	req->state |= PIOM_STATE_REGISTERED;
	req->server = server;
	LOG_RETURN(0);
}

__tbx_inline__ static int __piom_unregister(piom_server_t server,
				     piom_req_t req)
{
	LOG_IN();
	PIOM_LOGF("Unregister request %p for [%s]\n", req, server->name);
	__piom_del_success_req(server, req);
	list_del_init(&req->chain_req_registered);
	list_del_init(&req->chain_req_ready);
	req->state &= ~PIOM_STATE_REGISTERED;
	LOG_RETURN(0);
}

__tbx_inline__ static int __piom_register_poll(piom_server_t server,
					piom_req_t req)
{
	LOG_IN();
	/* On doit ajouter la requête à celles en attente */
	PIOM_LOGF("Grouping Poll request %p for [%s]\n", req, server->name);
	PIOM_BUG_ON(! (req->state & PIOM_STATE_REGISTERED));
	list_add(&req->chain_req_grouped, &server->list_req_poll_grouped);

	server->req_poll_grouped_nb++;
	req->state |= PIOM_STATE_GROUPED;
	__piom_poll_group(server, req);

	PIOM_BUG_ON(&req->chain_req_grouped == req->chain_req_grouped.next);
	PIOM_BUG_ON(&req->chain_req_grouped == req->chain_req_grouped.prev);

	if (server->req_poll_grouped_nb == 1) {
		__piom_poll_start(server);
	}
	LOG_RETURN(0);
}

__tbx_inline__ static int __piom_unregister_poll(piom_server_t server,
					  piom_req_t req)
{
	LOG_IN();
	if (req->state & PIOM_STATE_GROUPED) {
		PIOM_LOGF("Ungrouping Poll request %p for [%s]\n", req,
		       server->name);
#ifdef PIOM_BLOCKING_CALLS
		if(req->state & PIOM_STATE_EXPORTED){
			list_del_init(&req->chain_req_block_grouped);
			list_del_init(&req->chain_req_to_export);
			server->req_block_grouped_nb--;
		} else 
#endif /* PIOM_BLOCKING_CALLS */
		{
			list_del_init(&req->chain_req_grouped);
			server->req_poll_grouped_nb--;
		}
		req->state &= ~PIOM_STATE_GROUPED;
		PIOM_BUG_ON(server->list_req_poll_grouped.next == &req->chain_req_grouped);
		PIOM_BUG_ON(server->list_req_poll_grouped.prev == &req->chain_req_grouped);
		LOG_RETURN(1);
	}
#ifdef PIOM_BLOCKING_CALLS
	else if(req->state & PIOM_STATE_EXPORTED){
		list_del_init(&req->chain_req_to_export);
	}
#endif /* PIOM_BLOCKING_CALLS */
	LOG_RETURN(0);
}

#ifdef PIOM_BLOCKING_CALLS
static int __piom_register_block(piom_server_t server,
					piom_req_t req)
{
	LOG_IN();
	PIOM_LOGF("Grouping Block request %p for [%s]\n", req, server->name);

	__piom_unregister_poll(server, req);

	list_add(&req->chain_req_to_export, &server->list_req_to_export);			
	list_add(&req->chain_req_block_grouped, &server->list_req_block_grouped);			

	server->req_block_grouped_nb++;	
	req->state |= PIOM_STATE_GROUPED;// | PIOM_STATE_EXPORTED;

	__piom_block_group(server, req);
		
	LOG_RETURN(0);
}
#endif 

/* On gère les événements signalés OK par les call-backs
 * - retrait événtuel de la liste des ev groupés (que l'on compte)
 * - réveil des waiters sur les événements et le serveur
 */
__tbx_inline__ static int __piom_manage_ready(piom_server_t server)
{
	int nb_grouped_req_removed = 0;
	int nb_req_ask_wake_server = 0;
	piom_req_t req, tmp, bak;

	if (list_empty(&server->list_req_ready)) {
		return 0;
	}
	bak = NULL;
	_piom_spin_lock_softirq(&server->req_ready_lock); 
	list_for_each_entry_safe(req, tmp, &server->list_req_ready,
				 chain_req_ready) {
		if (req == bak) {
			break;
		}
		PIOM_LOGF("Poll succeed with req %p\n", req);
		req->state |= PIOM_STATE_OCCURED;

		__piom_wake_req_waiters(server, req, 0);
		if (!(req->state & PIOM_STATE_NO_WAKE_SERVER)) {
			__piom_add_success_req(server, req);
			nb_req_ask_wake_server++;
		}

		list_del_init(&req->chain_req_ready);
		if (req->state & PIOM_STATE_ONE_SHOT) {
			nb_grouped_req_removed +=__piom_unregister_poll(server, req);
			__piom_unregister(server, req);
		}
		bak = req;
	}

	PIOM_BUG_ON(!list_empty(&server->list_req_ready));
	_piom_spin_unlock_softirq(&server->req_ready_lock); 
	if (nb_grouped_req_removed) {
		PIOM_LOGF("Nb grouped task set to %i\n",
		       server->req_poll_grouped_nb);
	}

	if (nb_req_ask_wake_server) {
		__piom_wake_id_waiters(server, nb_req_ask_wake_server);
	}
	
	return 0;
}

__tbx_inline__ static int __piom_need_poll(piom_server_t server)
{
	return server->req_poll_grouped_nb;
}


/* On vient de faire une scrutation complète (pas un POLL_ONE sur le
 * premier), on réenclanche un timer si nécessaire.
 */
__tbx_inline__ static void __piom_update_timer(piom_server_t server)
{
	// TODO: Support sans Marcel
	if (server->poll_points & PIOM_POLL_AT_TIMER_SIG) {
		PIOM_LOGF("Update timer polling for [%s] at period %i\n",
			server->name, server->period);
#ifdef MARCEL
		ma_mod_timer(&server->poll_timer,
			     ma_jiffies + server->period);
#endif
	}
}


// Checks to see if some polling jobs should be done.
// TODO : choisir d'exporter un syscall (relancer __need_export)
static void piom_check_polling_for(piom_server_t server)
{
	int nb = __piom_need_poll(server);
	piom_req_t req  ;	
#ifdef PIOM__DEBUG
	static int count = 0;

	PIOM_LOGF("Check polling for [%s]\n", server->name);

	if (!count--) {
		PIOM_LOGF("Check polling 50000 for [%s]\n", server->name);
		count = 50000;
	}
#endif
	if (!nb)
		return;

	server->max_priority = PIOM_REQ_PRIORITY_LOWEST;
	server->registered_req_not_yet_polled = 0;
	if (nb == 1 && server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func){
		req=list_entry(server->list_req_poll_grouped.next,
					  struct piom_req, chain_req_grouped);
#ifdef PIOM_BLOCKING_CALLS
		if(list_entry(server->list_req_poll_grouped.next,
			      struct piom_req, chain_req_grouped)->func_to_use != PIOM_FUNC_SYSCALL ) 
#endif
		{			
			(*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
				(server, PIOM_FUNCTYPE_POLL_POLLONE,
				 req, nb, PIOM_OPT_REQ_IS_GROUPED);
		}
	} else if (server->funcs[PIOM_FUNCTYPE_POLL_POLLANY].func) {
		(*server->funcs[PIOM_FUNCTYPE_POLL_POLLANY].func)
		    (server, PIOM_FUNCTYPE_POLL_POLLANY, NULL, nb, 0);

	} else if (server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func) {
		piom_req_t req, tmp;
		FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) {
			if(! (req->state & PIOM_STATE_OCCURED))
			if(req->priority >= server->max_priority) {
				if(req->func_to_use != PIOM_FUNC_SYSCALL){
					(*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
						(server, PIOM_FUNCTYPE_POLL_POLLONE,
						 req, nb,
						 PIOM_OPT_REQ_IS_GROUPED
						 | PIOM_OPT_REQ_ITER);
				}
			}
		}
	} else {
		/* Pas de méthode disponible ! */
		PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
	}
	__piom_manage_ready(server);
	if (nb != __piom_need_poll(server)) {
		/* Le nombre de tache en cours de polling a varié */
		if (!__piom_need_poll(server)) {
			__piom_poll_stop(server);
			return;
		} else {
			__piom_poll_group(server, NULL);
		}
	}
	__piom_update_timer(server);
	return;
}

/* call-back pour la tasklet */
void piom_poll_from_tasklet(unsigned long hid)
{
	piom_server_t server = (piom_server_t) hid;
	PIOM_LOGF("Tasklet function for [%s]\n", server->name);
	piom_check_polling_for(server);
#ifdef MARCEL
	_piom_spin_lock(&piom_poll_lock);
	job_scheduled=0;
	_piom_spin_unlock(&piom_poll_lock);
#endif
	return;
}

void piom_manage_from_tasklet(unsigned long hid)
{
	piom_server_t server = (piom_server_t) hid;
	PIOM_LOGF("Tasklet function for [%s]\n", server->name);
	__piom_manage_ready(server);
	return;
}

/* call-back pour le timer */
void piom_poll_timer(unsigned long hid)
{
	piom_server_t server = (piom_server_t) hid;
#ifdef MARCEL
	PIOM_LOGF("Timer function for [%s]\n", server->name);
	ma_tasklet_schedule(&server->poll_tasklet);
#else
	// TODO: lancement du polling
	piom_check_polling_for(server);
#endif
	return;
}

/* Appelé par le code en divers points (idle et timer
 * principalement) */
void __piom_check_polling(unsigned polling_point)
{
#ifdef MARCEL
	if( job_scheduled || list_empty(&piom_list_poll))
		return;
	if( ! _piom_spin_trylock_softirq(&piom_poll_lock))
		return;
	int scheduled=0;
#endif

	PROF_IN();
	piom_server_t server, bak=NULL;

	list_for_each_entry(server, &piom_list_poll, chain_poll) {
		if(bak==server){
			break;	
		}
		if (server->poll_points & polling_point) {				
#ifdef MARCEL		
			job_scheduled=1;
			scheduled++;
			ma_tasklet_schedule(&server->poll_tasklet);
#else
			piom_check_polling_for(server);
#endif				// MARCEL
			bak=server;			
		}
	}
#ifdef MARCEL
	if(!scheduled)
	{
		list_for_each_entry(server, &piom_list_poll, chain_poll) {
			if(bak==server){
				break;	
			}
			job_scheduled=1;
			ma_tasklet_schedule(&server->poll_tasklet);
			bak=server;			
		}
	}
#endif
	_piom_spin_unlock_softirq(&piom_poll_lock);
	PROF_OUT();
}

/* Poll 'req' for 'usec' micro secondes*/
void piom_poll_req(piom_req_t req, unsigned usec){
	if(!req->server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func) 
		return; 	/* TODO : allow polling with other functions (pollany, etc.) */
#ifdef MARCEL
	marcel_task_t *lock;
	lock=piom_ensure_trylock_from_tasklet(req->server);
#endif				// MARCEL       
	tbx_tick_t t1, t2;
	TBX_GET_TICK(t1);
	int i=0;
	do {
		(*req->server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
			(req->server, PIOM_FUNCTYPE_POLL_POLLONE,
			 req, 1, PIOM_OPT_REQ_ITER);
	
		_piom_spin_lock_softirq(&req->server->req_ready_lock); 
		if(! list_empty(&req->chain_req_ready)) {
			/* Request succeeded */
			TBX_GET_TICK(t2);
			req->state |= PIOM_STATE_OCCURED;
			__piom_wake_req_waiters(req->server, req, 0);
			if (!(req->state & PIOM_STATE_NO_WAKE_SERVER))
				__piom_add_success_req(req->server, req);
			
			list_del_init(&req->chain_req_ready);
			if (req->state & PIOM_STATE_ONE_SHOT) {
				__piom_unregister_poll(req->server, req);
				__piom_unregister(req->server, req);
			}
			_piom_spin_unlock_softirq(&req->server->req_ready_lock); 
			break;
		}
		_piom_spin_unlock_softirq(&req->server->req_ready_lock); 
		TBX_GET_TICK(t2);
		i++;
	} while(TBX_TIMING_DELAY(t1, t2) < usec);
#ifdef MARCEL
	piom_restore_trylocked_from_tasklet(req->server, lock);
#endif				//MARCEL
}

/* Stop polling a specified query */
void piom_poll_stop(piom_req_t req){
	if(req->state & PIOM_STATE_GROUPED)
		__piom_unregister_poll(req->server,req);
}

/* Resume polling a specified query */
void piom_poll_resume(piom_req_t req){
	if(!(req->state & PIOM_STATE_GROUPED))
		__piom_register_poll(req->server,req);
}

/* Force une scrutation sur un serveur */
void piom_poll_force(piom_server_t server)
{
	LOG_IN();
	PIOM_LOGF("Poll forced for [%s]\n", server->name);
	/* Pas très important si on loupe quelque chose ici (genre
	 * liste modifiée au même instant)
	 */
	if (__piom_need_poll(server)) {
#ifdef MARCEL
		ma_tasklet_schedule(&server->poll_tasklet);
#else
		piom_check_polling_for(server);
		//TODO: Lancement polling
#endif
	}
	LOG_OUT();
}

/* Force une scrutation synchrone sur un serveur */
void piom_poll_force_sync(piom_server_t server)
{
	LOG_IN();
	PIOM_LOGF("Sync poll forced for [%s]\n", server->name);
#ifdef MARCEL
	marcel_task_t *lock;
	/* On se synchronise */
	lock = piom_ensure_lock_server(server);
#endif
	piom_check_polling_for(server);

#ifdef MARCEL
	piom_restore_lock_server_locked(server, lock);
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
__tbx_inline__ static void piom_verify_server_state(piom_server_t server)
{
	PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_LAUNCHED);
}


int piom_req_init(piom_req_t req)
{
	LOG_IN();
	__piom_init_req(req);
	LOG_RETURN(0);
}

/* Ajout d'un attribut spécifique à un événement */
int piom_req_attr_set(piom_req_t req, int attr)
{
	LOG_IN();
	if (attr & PIOM_ATTR_ONE_SHOT) {
		req->state |= PIOM_STATE_ONE_SHOT;
	}
	if (attr & PIOM_ATTR_NO_WAKE_SERVER) {
		req->state |= PIOM_STATE_NO_WAKE_SERVER;
	}
	if (attr & PIOM_ATTR_DONT_POLL_FIRST) {
		req->state |= PIOM_STATE_DONT_POLL_FIRST;
	}
	if (attr &
	    (~
	     (PIOM_ATTR_ONE_SHOT | PIOM_ATTR_NO_WAKE_SERVER))) {
		PIOM_EXCEPTION_RAISE(PIOM_CONSTRAINT_ERROR);
	}
	LOG_RETURN(0);
}

/* Enregistrement d'un événement */
int piom_req_submit(piom_server_t server, piom_req_t req)
{
	LOG_IN();
#ifdef MARCEL
	marcel_task_t *lock;
	lock=piom_ensure_trylock_from_tasklet(server);
#endif				// MARCEL

	piom_verify_server_state(server);
	PIOM_BUG_ON(req->state & PIOM_STATE_REGISTERED);

/*Entre en conflit avec le test de la fonction __register*/
//      req->server=server;
	__piom_register(server, req);

	PIOM_BUG_ON(!(req->state & PIOM_STATE_REGISTERED));
#ifdef PIOM_BLOCKING_CALLS
	req->state &= ~PIOM_STATE_OCCURED;

	if (__piom_need_export(server, req, NULL, NULL) )
	{
		piom_callback_will_block(server, req);
	}
	else
#endif /* PIOM_BLOCKING_CALLS */

	if(! (req->state & PIOM_STATE_DONT_POLL_FIRST)) {
		piom_poll_req(req, 0);
		req->state&= ~PIOM_STATE_DONT_POLL_FIRST;
		if(req->chain_req_ready.next!= &(req->chain_req_ready) && (req->state= PIOM_STATE_ONE_SHOT)) {
			// ie state occured 
			//piom_req_cancel(req, 0);
		} else
			__piom_register_poll(server, req);
	} else {
		server->registered_req_not_yet_polled++;
		__piom_register_poll(server, req);
	}
#ifdef MARCEL
	piom_restore_trylocked_from_tasklet(server, lock);
#endif				//MARCEL
	LOG_RETURN(0);
}

/* Abandon d'un événement et retour des threads en attente sur cet événement */
int piom_req_cancel(piom_req_t req, int ret_code)
{
#ifdef MARCEL
	marcel_task_t *lock;
#endif				//MARCEL
	piom_server_t server;
	LOG_IN();

	PIOM_BUG_ON(!(req->state & PIOM_STATE_REGISTERED));
	server = req->server;
#ifdef MARCEL
	lock = piom_ensure_lock_server(server);
#endif				//MARCEL
	piom_verify_server_state(server);

	__piom_wake_req_waiters(server, req, ret_code);

	if (__piom_unregister_poll(server, req)) {
		if (__piom_need_poll(server)) {
			__piom_poll_group(server, NULL);
		} else {
			__piom_poll_stop(server);
		}
	}
	__piom_unregister(server, req);

#ifdef MARCEL
	piom_restore_lock_server_locked(server, lock);
#endif				//MARCEL

	LOG_RETURN(0);
}

#ifdef PIOM_BLOCKING_CALLS
/* Previent un LWP de comm qu'il y a des requetes à exporter */
int piom_callback_will_block(piom_server_t server, piom_req_t req) 
{
	piom_comm_lwp_t lwp=NULL;	
	int foo=42;

	__piom_register_block(server, req);

	/* Choisit un LWP */
	do {
		if(!list_empty(server->list_lwp_ready.next))
			lwp = list_entry(server->list_lwp_ready.next, struct piom_comm_lwp, chain_lwp_ready);
		else if( server->stopable && !list_empty(server->list_lwp_working.next))
			lwp = list_entry(server->list_lwp_working.next, struct piom_comm_lwp, chain_lwp_working);
		else 		/* Cree un autre LWP (attention, c'est TRES cher) */
			piom_server_start_lwp(server, 1);
	}while(! lwp);

	/* wakeup LWP */
	PIOM_LOGF("Waiking up comm LWP #%d\n",lwp->vp_nb);
	PROF_EVENT(writing_tube);
	write(lwp->fds[1], &foo, 1);
	PROF_EVENT(writing_tube_done);
#ifdef EXPORT_THREADS
	marcel_task_t *lock;	/* recupere les requetes à poster */
	piom_req_t prev, tmp, req2=NULL;
	if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].func && server->req_block_grouped_nb>1)
	{
		__piom_tryunlock_server(server);
		
		(*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].
		 func) (server, PIOM_FUNCTYPE_BLOCK_WAITANY, req2,
			server->req_block_grouped_nb, lwp->fds[0]);
		
		lock = piom_ensure_trylock_server(server);
	}
	else{
		if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)
		{
			prev=NULL;
			FOREACH_REQ_TO_EXPORT_BASE_SAFE(req2, tmp, server) {
				if(req2==prev)
					break;
				PIOM_WARN_ON(req2->state &PIOM_STATE_OCCURED);
				/* Place la req dans la liste des req exportées */
				list_del_init(&req2->chain_req_to_export);
				list_add(&req2->chain_req_exported, &server->list_req_exported);
				req2->state|=PIOM_STATE_EXPORTED;
				
				__piom_tryunlock_server(server);
					
				/* appel a fonction bloquante */		
				(*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].
				 func) (server, PIOM_FUNCTYPE_BLOCK_WAITONE, req2,
					server->req_poll_grouped_nb, lwp->fds[0]);
				
				list_del_init(&req2->chain_req_exported);
				
				lock = piom_ensure_trylock_server(server);
				
				prev=req;
				break;
			}
		}
		else 
			PIOM_EXCEPTION_RAISE(PIOM_CALLBACK_ERROR);
		
	}
	/* traitement des requetes terminees */
	__piom_manage_ready(server);
	lock = piom_ensure_trylock_server(server);
	
#endif
	return 0;
}

/* TODO : lors de la création du serveur : spécifier le nombre de LWPs a lancer */
/* attend d'etre débloqué pour lancer un appel systeme */
void *__piom_syscall_loop(void * param)
{
	piom_comm_lwp_t lwp=(piom_comm_lwp_t) param;
	piom_server_t server=lwp->server;
	piom_req_t prev, tmp, req=NULL;
	int foo=0;
	marcel_task_t *lock;
#ifdef EXPORT_THREADS

	/* On met la priorite au minimum */
	struct sched_param pthread_param;
	if(sched_getparam(0, &pthread_param)) {
		fprintf(stderr, "couldn't get my priority !\n");
		switch(errno){
		case EINVAL: 
			fprintf(stderr, "invalid param\n"); 
			break;
		case EPERM:
			fprintf(stderr, " The calling process does not have appropriate privileges \n");
			break;
		case ESRCH:
			fprintf(stderr, "The process whose ID is 0 is unknown\n");
			break;
		}
	}
	if(nice(10) == -1){
		fprintf(stderr, "couldn't lower my priority\n");
	}
#endif
	lock = piom_ensure_trylock_server(server);
	do {
		/* etat working -> ready */
		list_del_init(&lwp->chain_lwp_working);
		list_add_tail(&lwp->chain_lwp_ready, &server->list_lwp_ready);
		__piom_tryunlock_server(server);

                /* Lit n'importe quoi */		
		PROF_EVENT2(syscall_waiting, lwp->vp_nb, lwp->fds[0]);
#ifdef EXPORT_THREADS
		/* TODO : faire vraiment qqchose ! */
		do {
			read(lwp->fds[0], &foo, 1); 
		} while(1); 
#else
		read(lwp->fds[0], &foo, 1); 
#endif
		PROF_EVENT2(syscall_read_done, lwp->vp_nb, lwp->fds[0]);
		PIOM_LOGF("Comm LWP #%d is working\n",lwp->vp_nb);

		if( lwp->server->state == PIOM_SERVER_STATE_HALTED ){
			list_del_init(&lwp->chain_lwp_ready);
			return NULL;
		}
		
		/* etat ready->working */
		lock = piom_ensure_trylock_server(server);

		list_del_init(&lwp->chain_lwp_ready);
		list_add(&lwp->chain_lwp_working, &server->list_lwp_working);
		
		/* recupere les requetes à poster */
		if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].func && server->req_block_grouped_nb>1)
		{
			__piom_tryunlock_server(server);

			(*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].
			 func) (server, PIOM_FUNCTYPE_BLOCK_WAITANY, req,
				server->req_block_grouped_nb, lwp->fds[0]);

			lock = piom_ensure_trylock_server(server);
		}
		else{
			if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)
			{
				prev=NULL;
				FOREACH_REQ_TO_EXPORT_BASE_SAFE(req, tmp, server) {
					if(req==prev)
						break;
					PIOM_WARN_ON(req->state &PIOM_STATE_OCCURED);
					/* Place la req dans la liste des req exportées */
					list_del_init(&req->chain_req_to_export);
					list_add(&req->chain_req_exported, &server->list_req_exported);
					req->state|=PIOM_STATE_EXPORTED;
					
					__piom_tryunlock_server(server);
					
					/* appel a fonction bloquante */		
					(*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].
					 func) (server, PIOM_FUNCTYPE_BLOCK_WAITONE, req,
						server->req_poll_grouped_nb, lwp->fds[0]);
					
					list_del_init(&req->chain_req_exported);
					
					lock = piom_ensure_trylock_server(server);

					prev=req;
					break;
				}
			}
			else 
				PIOM_EXCEPTION_RAISE(PIOM_CALLBACK_ERROR);

		}
		/* traitement des requetes terminees */
		__piom_manage_ready(server);
		lock = piom_ensure_trylock_server(server);

	} while(lwp->server->state!=PIOM_SERVER_STATE_HALTED);
	__piom_tryunlock_server(server);
	return NULL;}


#endif /* PIOM_BLOCKING_CALLS */

// TODO: support sans Marcel
__tbx_inline__ static int __piom_wait_req(piom_server_t server, piom_req_t req,
				   piom_wait_t wait, piom_time_t timeout)
{
	LOG_IN();
	if (timeout) {
		PIOM_EXCEPTION_RAISE(PIOM_NOT_IMPLEMENTED);
	}
	INIT_LIST_HEAD(&wait->chain_wait);
	INIT_LIST_HEAD(&req->list_wait);
	list_add(&wait->chain_wait, &req->list_wait);
#ifdef MARCEL
	marcel_sem_init(&wait->sem, 0);
	wait->task = MARCEL_SELF;
#endif /* MARCEL */
	wait->ret = 0;

#ifdef PIOM_BLOCKING_CALLS
	if (__piom_need_export(server, req, wait, timeout) )
	{
		piom_callback_will_block(server, req);
	}
#endif /* PIOM_BLOCKING_CALLS */

	__piom_unlock_server(server);

#ifdef MARCEL
	// TODO: utiliser pmarcel_sem_P qui peut renvoyer -1 avec errno EINT (support posix)
	marcel_sem_P(&wait->sem);	
#else
	int waken_up = 0;

	do {
		req->state |=
		    PIOM_STATE_ONE_SHOT | PIOM_STATE_NO_WAKE_SERVER;
		piom_check_polling_for(server);
/*
		if (server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func
		    && req->func_to_use != PIOM_FUNC_SYSCALL) {
			PIOM_LOGF("Using Immediate POLL_ONE\n");
			(*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
			    (server, PIOM_FUNCTYPE_POLL_POLLONE,
			     req, server->req_poll_grouped_nb, 0);
			waken_up = __piom_need_poll(server);
			__piom_manage_ready(server);
			waken_up -= __piom_need_poll(server);
			checked = 1;
		}
*/

		if (req->state & PIOM_STATE_OCCURED) {
			if (waken_up) {
				/* Le nombre de tache en cours de polling a varié */
				if (__piom_need_poll(server)) {
					__piom_poll_group(server, NULL);
				} else {
					__piom_poll_stop(server);
				}
			}
			__piom_unregister_poll(server, req);
			__piom_unregister(server, req);

			/* Pas update_timer(id) car on a fait un poll_one et
			 * pas poll_all */
			LOG_RETURN(0);
		}
	} while (!(req->state & PIOM_STATE_OCCURED));

#endif
	LOG_RETURN(wait->ret);
}

/* Attente d'un thread sur un événement déjà enregistré */
int piom_req_wait(piom_req_t req, piom_wait_t wait,
		   piom_time_t timeout)
{
	if(req->state&PIOM_STATE_OCCURED)
		return 0;
#ifdef MARCEL
	marcel_task_t *lock;
#endif				// MARCEL
	int ret = 0;
	piom_server_t server;
	LOG_IN();

	PIOM_BUG_ON(!(req->state & PIOM_STATE_REGISTERED));
	server = req->server;
#ifdef MARCEL
	lock = piom_ensure_lock_server(server);
#endif				// MARCEL
	piom_verify_server_state(server);

	req->state &= ~PIOM_STATE_OCCURED;

	/* TODO: On pourrait faire un check juste sur cet événement 
	 * (au moins quand on itère avec un poll_one sur tous les req) 
	 */
	if (!(req->state & PIOM_STATE_DONT_POLL_FIRST))
		piom_check_polling_for(server);
	else
		req->state &= ~PIOM_STATE_DONT_POLL_FIRST;

	if (!(req->state & PIOM_STATE_OCCURED)) {
		ret = __piom_wait_req(server, req, wait, timeout);
#ifdef MARCEL
		piom_restore_lock_server_unlocked(server, lock);
	} else {
		piom_restore_lock_server_locked(server, lock);
#endif				// MARCEL
	}

	LOG_RETURN(ret);
}

// TODO: Support sans Marcel
/* Attente d'un thread sur un quelconque événement du serveur */
int piom_server_wait(piom_server_t server, piom_time_t timeout)
{
	LOG_IN();
	struct piom_wait wait;
#ifdef MARCEL
	marcel_task_t *lock;
	lock = piom_ensure_lock_server(server);
#endif				// MARCEL

	piom_verify_server_state(server);

	if (timeout) {
		PIOM_EXCEPTION_RAISE(PIOM_NOT_IMPLEMENTED);
	}

	list_add(&wait.chain_wait, &server->list_id_waiters);
#ifdef MARCEL
	marcel_sem_init(&wait.sem, 0);
#endif
	wait.ret = 0;
#ifdef PIOM__DEBUG
	wait.task = MARCEL_SELF;
#endif
	/* TODO: on pourrait ne s'enregistrer que si le poll ne fait rien */
	piom_check_polling_for(server);
#ifdef MARCEL
	__piom_unlock_server(server);
	// TODO: utiliser pmarcel_sem_P qui peut renvoyer -1 avec errno EINT (support posix) ?

	marcel_sem_P(&wait.sem);
	piom_restore_lock_server_unlocked(server, lock);
#endif				// MARCEL

	LOG_RETURN(wait.ret);
}

/* Enregistrement, attente et désenregistrement d'un événement */
int piom_wait(piom_server_t server, piom_req_t req,
	       piom_wait_t wait, piom_time_t timeout)
{
	LOG_IN();
	int checked = 0;
	int waken_up = 0;

#ifdef MARCEL
	marcel_task_t *lock;
	lock = piom_ensure_lock_server(server);
#endif				// MARCEL

	piom_verify_server_state(server);

	__piom_init_req(req);
	__piom_register(server, req);
#ifdef MARCEL
	PIOM_LOGF("Marcel_poll (thread %p)...\n", marcel_self());
#endif				// MARCEL
	PIOM_LOGF("using pollid [%s]\n", server->name);
	req->state |= PIOM_STATE_ONE_SHOT | PIOM_STATE_NO_WAKE_SERVER;

	if (server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func
	    && req->func_to_use != PIOM_FUNC_SYSCALL && !checked) {
		PIOM_LOGF("Using Immediate POLL_ONE\n");
		(*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
		    (server, PIOM_FUNCTYPE_POLL_POLLONE,
		     req, server->req_poll_grouped_nb, 0);
		waken_up = __piom_need_poll(server);
		__piom_manage_ready(server);
		waken_up -= __piom_need_poll(server);
		checked = 1;
	}

	if (req->state & PIOM_STATE_OCCURED) {
		if (waken_up) {
			/* Le nombre de tache en cours de polling a varié */
			if (__piom_need_poll(server)) {
				__piom_poll_group(server, NULL);
			} else {
				__piom_poll_stop(server);
			}
		}
		/* Pas update_timer(id) car on a fait un poll_one et
		 * pas poll_all */
#ifdef MARCEL
		piom_restore_lock_server_locked(server, lock);
#endif				// MARCEL
		LOG_RETURN(0);
	}

	__piom_register_poll(server, req);
	if (!checked && req->func_to_use != PIOM_FUNC_SYSCALL) {
		piom_check_polling_for(server);
	}

	if (!(req->state & PIOM_STATE_OCCURED)) {
		__piom_wait_req(server, req, wait, timeout);
#ifdef MARCEL
		piom_lock_server_owner(server, lock);
#endif				// MARCEL
	}
	__piom_unregister(server, req);
#ifdef MARCEL
	piom_restore_lock_server_locked(server, lock);
#endif				// MARCEL
	LOG_RETURN(wait->ret);
}

/****************************************************************
 * Fonctions d'initialisation/terminaison
 */

static tbx_flag_t piom_activity = tbx_flag_clear;

int
piom_test_activity(void)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  result = tbx_flag_test(&piom_activity);
  LOG_OUT();

  return result;
}

void 
piom_server_start_lwp(piom_server_t server, unsigned nb_lwps)
{
#ifdef PIOM_BLOCKING_CALLS
	/* Lancement des LWP de comm */
	int i;
	marcel_attr_t attr;
	
	marcel_attr_init(&attr);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setname(&attr, "piom_receiver");

	INIT_LIST_HEAD(&server->list_lwp_working);
	INIT_LIST_HEAD(&server->list_lwp_ready);
	for(i=0;i<nb_lwps;i++)
	{
		PIOM_LOGF("Creating a lwp for server %p\n", server);
		piom_comm_lwp_t lwp=(piom_comm_lwp_t) malloc(sizeof(struct piom_comm_lwp));
		__piom_init_lwp(lwp);
		lwp->server=server;
		/* le LWP est considéré comme Working pendant l'initialisation */
		list_add(&lwp->chain_lwp_working, &server->list_lwp_working);

		lwp->vp_nb = marcel_add_lwp();
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(lwp->vp_nb));

		if (pipe(lwp->fds) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		marcel_create(&lwp->pid, &attr, (void*) &__piom_syscall_loop, lwp);

	}
#endif /* PIOM_BLOCKING_CALLS */
}

void piom_server_init(piom_server_t server, char *name)
{
	*server = (struct piom_server) PIOM_SERVER_INIT(*server, name);
	PIOM_LOGF( "New server %s : %p\n",name, server);
#ifdef MARCEL
//	ma_open_softirq(MA_PIOM_SOFTIRQ, __piom_manage_ready, NULL);
#endif /* MARCEL */
}

#ifndef MA_JIFFIES_PER_TIMER_TICK
#define MA_JIFFIES_PER_TIMER_TICK 1
#endif

int piom_server_set_poll_settings(piom_server_t server,
				   unsigned poll_points,
				   unsigned period, int max_poll)
{
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_INIT);

	server->max_poll = max_poll;
	server->poll_points = poll_points;
	if (poll_points & PIOM_POLL_AT_TIMER_SIG) {
		period = period ? : 1;
		//TODO : virer l'horloge de Marcel
		server->period = period * MA_JIFFIES_PER_TIMER_TICK;
	}
	return 0;
}

int piom_server_start(piom_server_t server)
{
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_INIT);

	if (!server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func
	    && !server->funcs[PIOM_FUNCTYPE_POLL_POLLANY].func) {
		PIOM_LOGF("One poll function needed for [%s]\n",
		       server->name);
		PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
	}
	piom_activity = tbx_flag_set;
	server->state = PIOM_SERVER_STATE_LAUNCHED;
	return 0;
}

int piom_req_free(piom_req_t req)
{
	piom_server_t server=req->server;
	if(server) {
#ifdef MARCEL
		marcel_task_t *lock;
		lock=piom_ensure_trylock_from_tasklet(server);
#endif
		int nb_req_ask_wake_server=0;
		_piom_spin_lock_softirq(&server->req_ready_lock);

		if(! list_empty(&req->chain_req_ready)) {
			/* Successful request. */
			__piom_wake_req_waiters(server, req, 0);
			if (!(req->state & PIOM_STATE_NO_WAKE_SERVER)) {
				__piom_add_success_req(server, req);
				nb_req_ask_wake_server++;
			}
			list_del_init(&req->chain_req_ready);
		}
		PIOM_BUG_ON(!list_empty(&req->chain_req_ready));
		__piom_unregister_poll(server, req);
		__piom_unregister(server, req);
		_piom_spin_unlock_softirq(&server->req_ready_lock);
		if (nb_req_ask_wake_server) {
			__piom_wake_id_waiters(server, nb_req_ask_wake_server);
		}
#ifdef MARCEL
		piom_restore_trylocked_from_tasklet(server, lock);
#endif

		if (!__piom_need_poll(server)) {
			__piom_poll_stop(server);
			return 1;
		} else {
			__piom_poll_group(server, NULL);
		}
	}

	return 0;
}

int piom_server_stop(piom_server_t server)
{
	LOG_IN();
	piom_req_t req, tmp;
	if(server->state != PIOM_SERVER_STATE_LAUNCHED)
		return 0;

#ifdef MARCEL
	marcel_task_t *lock;
	lock = piom_ensure_lock_server(server);
#endif				//MARCEL

	piom_verify_server_state(server);
	server->state = PIOM_SERVER_STATE_HALTED;
#ifdef PIOM_BLOCKING_CALLS
/* arret des LWPs de comm */
	piom_comm_lwp_t lwp;

	while(!list_empty(server->list_lwp_ready.next)){
		char foo=42;
		lwp = list_entry(server->list_lwp_ready.next, struct piom_comm_lwp, chain_lwp_ready);
		write(lwp->fds[1], &foo, 1);
	}
	while(!list_empty(server->list_lwp_working.next)){
		char foo=42;
		lwp = list_entry(server->list_lwp_working.next, struct piom_comm_lwp, chain_lwp_working);
		write(lwp->fds[1], &foo, 1);
	}
#endif
#ifndef ECANCELED
#define ECANCELED EIO
#endif
	list_for_each_entry_safe(req, tmp,
				 &server->list_req_registered,
				 chain_req_registered) {
		__piom_wake_req_waiters(server, req, -ECANCELED);
		__piom_unregister_poll(server, req);
		__piom_unregister(server, req);
	}
	__piom_wake_id_waiters(server, -ECANCELED);

#ifdef MARCEL
	piom_restore_lock_server_locked(server, lock);
#endif				//MARCEL

	LOG_RETURN(0);

	return 0;
}


