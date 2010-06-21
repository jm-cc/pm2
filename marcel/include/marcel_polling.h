/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_POLLING_H__
#define __MARCEL_POLLING_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "marcel_timer.h"
#include "tbx_fast_list.h"


/** Public macros **/
/*[C]****************************************************************
 * Itérateurs pour les call-backs
 */

/****************************************************************
 * Itérateur pour les requêtes groupées de la scrutation (polling)
 */

/* Itérateur avec le type de base
   marcel_ev_req_t req : itérateur
   marcel_ev_server_t server : serveur
*/
#define FOREACH_REQ_POLL_BASE(req, server) \
  tbx_fast_list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem mais protégé (usage interne) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) \
  tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Itérateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (itérateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure pointée par req
*/
#define FOREACH_REQ_POLL(req, server, member) \
  tbx_fast_list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)


/****************************************************************
 * Itérateur pour les attentes d'événements rattachés à une requête
 */

/* Itérateur avec le type de base
   marcel_ev_wait_t wait : itérateur
   marcel_ev_req_t req : requête
*/
#define FOREACH_WAIT_BASE(wait, req) \
  tbx_fast_list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem mais protégé (usage interne) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req) \
  tbx_fast_list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* Itérateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (itérateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure pointée par req
*/
#define FOREACH_WAIT(wait, req, member) \
  tbx_fast_list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)



/* Macro utilisable dans les call-backs pour indiquer qu'une requête
   a reçu un événement.

   * la requête sera fournie à marcel_ev_get_success_req() (tant
   qu'elle n'est pas désenregistrée)
   * les threads encore en attente d'un événement lié à cette requête seront
   réveillés et renverront le code 0

   marcel_ev_req_t req : requête
*/
#define MARCEL_EV_REQ_SUCCESS(req) \
  do { \
        tbx_fast_list_move(&(req)->chain_req_ready, &(req)->server->list_req_ready); \
  } while(0)


/*[S]****************************************************************
 * Les constantes pour le polling
 * Elles peuvent être ORed
 */
#define MA_EV_POLL_AT_TIMER_SIG  (1 << MARCEL_SCHEDULING_POINT_TIMER     )
#define MA_EV_POLL_AT_YIELD      (1 << MARCEL_SCHEDULING_POINT_YIELD     )
#define MA_EV_POLL_AT_LIB_ENTRY  (1 << MARCEL_SCHEDULING_POINT_LIB_ENTRY )
#define MA_EV_POLL_AT_IDLE       (1 << MARCEL_SCHEDULING_POINT_IDLE      )
#define MA_EV_POLL_AT_CTX_SWITCH (1 << MARCEL_SCHEDULING_POINT_CTX_SWITCH)


/** Public data types **/
/* Attribut pouvant être attaché aux événements */
enum {
	/* Désactive la requête lorsque survient l'occurrence
	 * suivante de l'événement */
	MARCEL_EV_ATTR_ONE_SHOT = 1,
	/* Ne réveille pas les threads en attente d'événements du
	 * serveur (ie marcel_ev_server_wait())*/
	MARCEL_EV_ATTR_NO_WAKE_SERVER = 2
};

/*[SC]****************************************************************
 * Les différents opérations call-backs possibles
 */
typedef enum {
	/* server, XX, req to poll, nb_req_grouped, flags */
	MARCEL_EV_FUNCTYPE_POLL_POLLONE,
	/* server, XX, NA, nb_req_grouped, NA */
	MARCEL_EV_FUNCTYPE_POLL_GROUP,
	/* server, XX, NA, nb_req_grouped, NA */
	MARCEL_EV_FUNCTYPE_POLL_POLLANY,
	/* PRIVATE */
	MA_EV_FUNCTYPE_SIZE
} marcel_ev_op_t;

/*[C]****************************************************************
 * Les flags des call-backs (voir ci-dessus)
 */
enum {
	/* Pour POLL_POLLONE */
	MARCEL_EV_OPT_REQ_IS_GROUPED = 1,
	MARCEL_EV_OPT_REQ_ITER = 2
};

/****************************************************************
 * Structure serveur d'événements
 */
enum {
	MARCEL_EV_STATE_OCCURED = 1,
	MARCEL_EV_STATE_GROUPED = 2,
	MARCEL_EV_STATE_ONE_SHOT = 4,
	MARCEL_EV_STATE_NO_WAKE_SERVER = 8,
	MARCEL_EV_STATE_REGISTERED = 16
};

/* Une requ�te d�finit une entit� qui pourra recevoir un �v�nement que
 * l'on pourra attendre.  Diverses requ�tes d'un serveur pourront �tre
 * group�es (afin de d�terminer plus rapidement l'ensemble des �tats
 * de chacune des requ�tes � chaque scrutation)
 */
typedef struct marcel_ev_req *marcel_ev_req_t;

/* Un thread peut attendre l'arriv�e effective d'un �v�nement
 * correspondant � une requ�te. Le thread est d�schedul� tant que la
 * requ�te n'est pas pr�te (que l'�v�nement n'est pas re�u).
 */
typedef struct marcel_ev_wait *marcel_ev_wait_t;

/*[C]****************************************************************
 * Le prototype des call-back
 * server : le serveur
 * op : l'opération (call-back) demandée
 * req : pour *(POLL|WAIT)ONE* : la requête à tester en particulier
 * nb_req : pour POLL_* : le nombre de requêtes groupées
 * option : flags dépendant de l'opération
 *  - pour POLL_POLLONE :
 *     + EV_IS_GROUPED : si la requête est déjà groupée
 *     + EV_ITER : si POLL_POLLONE est appelée sur toutes les requêtes
 *                 en attente (ie POLL_POLLANY n'est pas disponible)
 *
 * La valeur de retour est pour l'instant ignorée.
 */
typedef struct marcel_ev_server *marcel_ev_server_t;
typedef int (marcel_ev_callback_t) (marcel_ev_server_t server,
				    marcel_ev_op_t op,
				    marcel_ev_req_t req, int nb_ev, int option);
typedef marcel_ev_callback_t *marcel_ev_pcallback_t;


/** Public functions **/
void marcel_ev_server_init(marcel_ev_server_t server, char *name);

/* Enregistrement des call-backs utilisables */
int marcel_ev_server_add_callback(marcel_ev_server_t server, marcel_ev_op_t op,
				  marcel_ev_callback_t * func);

/* Réglage des paramètres de scrutation */
int marcel_ev_server_set_poll_settings(marcel_ev_server_t server,
				       unsigned poll_points, unsigned poll_frequency);

/* Démarrage du serveur
 * Il devient possible d'attendre des événements
 */
int marcel_ev_server_start(marcel_ev_server_t server);

/* Arrêt du serveur
 * Il est nécessaire de le réinitialiser pour le redémarrer
 */
int marcel_ev_server_stop(marcel_ev_server_t server);

/*[U]****************************************************************
 * Fonctions à l'usage des threads applicatifs
 */

/* Un raccourci pratique des fonctions suivantes, utile si l'on ne
 * soumet la requête qu'une seule fois. Les opérations suivantes sont
 * effectuées : initialisation, soumission et attente d'un
 * événement avec ONE_SHOT positionné */
int marcel_ev_wait(marcel_ev_server_t server, marcel_ev_req_t req,
		   marcel_ev_wait_t wait, marcel_time_t timeout);

/* Exclusion mutuelle pour un serveur d'événements
 *
 * - les call-backs sont TOUJOURS appelés à l'intérieur de cette
 *   exclusion mutuelle.
 * - les call-backs BLOCK_ONE|ALL doivent relâcher puis reprendre ce
 *   lock avant et après l'appel système bloquant avec les deux fonctions
 *   prévues pour (marcel_ev_callback_*).
 * - les fonctions précédentes peuvent être appelées avec ou sans ce
 *   lock
 * - le lock est relâché automatiquement par les fonctions d'attente
 *   (marcel_ev_*wait*())

 * - les call-backs et les réveils des threads en attente sur les
 *   événements signalés par le call-back sont atomiques (vis-à-vis de 
 *   ce lock)
 * - si le lock est pris avant les fonctions d'attente (ev_wait_*),
 *   la mise en attente est atomique vis-à-vis des call-backs
 *   ie: un call-back signalant l'événement attendu réveillera cette
 *   attente
 * - si un événement à la propriété ONE_SHOT, le désenregistrement est
 *   atomic vis-à-vis du call-back qui a généré l'événement.
 */
int marcel_ev_lock(marcel_ev_server_t server);
int marcel_ev_unlock(marcel_ev_server_t server);

/* Utilisé dans les initialiseurs */
void marcel_poll_from_tasklet(unsigned long id);
void marcel_poll_timer(unsigned long id);
int marcel_check_polling(unsigned polling_point);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MARCEL_EV_SERVER_DECLARE(var) \
	const marcel_ev_server_t var


/** Internal data types **/
/* Etat serveur evenements */
enum {
	MA_EV_SERVER_STATE_NONE,
	MA_EV_SERVER_STATE_INIT = 1,
	MA_EV_SERVER_STATE_LAUNCHED = 2,
	MA_EV_SERVER_STATE_HALTED = 3
};


/*[U]****************************************************************
 * Les types abstaits pour le serveur et les événements
 */
/* Le serveur définit les call-backs et les paramètres de scrutation à
 * utiliser. Il va éventuellement regrouper les ressources
 * enregistrées (en cas de scrutation active par exemple)
 */
struct marcel_ev_server {
	/* Spinlock régissant l'accès à cette structure */
	ma_spinlock_t lock;
	/* Thread propriétaire du lock (pour le locking applicatif) */
	marcel_task_t *lock_owner;
	/* Liste des requêtes soumises */
	struct tbx_fast_list_head list_req_registered;
	/* Liste des requêtes signalées prêtes par les call-backs */
	struct tbx_fast_list_head list_req_ready;
	/* Liste des requêtes pour ceux en attente dans la liste précédente */
	struct tbx_fast_list_head list_req_success;
	/* Liste des attentes en cours sur les événements */
	struct tbx_fast_list_head list_id_waiters;
	/* Spinlock régissant l'accès à la liste précédente */
	ma_spinlock_t req_success_lock;
	/* Polling events registered but not yet polled */
	int registered_req_not_yet_polled;

	/* Liste des requêtes groupées pour le polling (ou en cours de groupage) */
	struct tbx_fast_list_head list_req_poll_grouped;
	/* Nombre de requêtes dans la liste précédente */
	int req_poll_grouped_nb;

	/* Call-backs */
	marcel_ev_pcallback_t funcs[MA_EV_FUNCTYPE_SIZE];
	/* Points et fréquence de scrutation */
	unsigned poll_points;
	unsigned frequency;
	/* Chaine des serveurs en cours de polling 
	   (state == 2 et tâches en attente de polling) */
	struct tbx_fast_list_head chain_poll;
	/* Tasklet et timer utilisée pour la scrutation */
	struct marcel_tasklet_struct poll_tasklet;
	struct marcel_timer_list poll_timer;
	/* État du serveur */
	int state;
	/* Nom (utilisé pour le débug) */
	const char *name;
};

/** Internal data structures **/
struct marcel_ev_req {
	/* Chaine des requêtes soumises */
	struct tbx_fast_list_head chain_req_registered;
	/* Chaine des requêtes groupées en attente */
	struct tbx_fast_list_head chain_req_grouped;
	/* Chaine des requêtes signalées OK par un call-back */
	struct tbx_fast_list_head chain_req_ready;
	/* Chaine des requêtes à signaler au serveur */
	struct tbx_fast_list_head chain_req_success;
	/* État */
	int state;
	/* Serveur attaché */
	marcel_ev_server_t server;
	/* Liste des attentes en cours sur cette requête */
	struct tbx_fast_list_head list_wait;
};

struct marcel_ev_wait {
	/* Chaine des événements groupé en attente */
	struct tbx_fast_list_head chain_wait;

	marcel_sem_t sem;
	/* 0: event
	   -ECANCELED: marcel_unregister called
	 */
	int ret;
	marcel_task_t *task;
};


/** Internal global variables **/
/* Liste des serveurs en cours de polling 
 * (state == 2 et tâches en attente de polling) 
 */
extern struct tbx_fast_list_head ma_ev_list_poll;


/** Internal functions **/
int ma_polling_is_required(unsigned polling_point);
void __ma_check_polling(unsigned polling_point);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_POLLING_H__ **/
