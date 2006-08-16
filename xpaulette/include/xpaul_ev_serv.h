
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

#ifndef XPAUL_EV_SERV
#define XPAUL_EV_SERV
#include "pm2_list.h"
// THESE: INTERFACE START
/****************************************************************
 * Certains commentaires sont étiquetés par U/S/C/I signifiant
 * User       : utile aux threads applicatifs pour exploiter ces mécanismes
 * Start      : utile pour l'initialisation
 * Call-backs : utile pour le concepteur de call-backs
 * Internal   : ne devrait pas être utilisé hors de XPaulette
 *
 * Ce ne sont que des indications, pas des contraintes
 ****************************************************************/

/*[SC]****************************************************************
 * Récupère l'adresse d'une structure englobante 
 * Il faut donner :
 * - l'adresse du champ interne
 * - le type de la structure englobante
 * - le nom du champ interne dans la structure englobante
 */
#define struct_up(ptr, type, member) \
	tbx_container_of(ptr, type, member)


/*[U]****************************************************************
 * Les types abstaits pour le serveur et les événements
 */
/* Le serveur définit les call-backs et les paramètres de scrutation à
 * utiliser. Il va éventuellement regrouper les ressources
 * enregistrées (en cas de scrutation active par exemple)
 */
typedef struct xpaul_server *xpaul_server_t;

/* Une requête définit une entité qui pourra recevoir un événement que
 * l'on pourra attendre.  Diverses requêtes d'un serveur pourront être
 * groupées (afin de déterminer plus rapidement l'ensemble des états
 * de chacune des requêtes à chaque scrutation)
 */
typedef struct xpaul_req *xpaul_req_t;

/* Un thread peut attendre l'arrivée effective d'un événement
 * correspondant à une requête. Le thread est déschedulé tant que la
 * requête n'est pas prête (que l'événement n'est pas reçu).
 */
typedef struct xpaul_wait *xpaul_wait_t;

/*[SC]****************************************************************
 * Les différents opérations call-backs possibles
 */
typedef enum {
	/* server, XX, req to poll, nb_req_grouped, flags */
	XPAUL_FUNCTYPE_POLL_POLLONE,
	/* server, XX, NA, nb_req_grouped, NA */
	XPAUL_FUNCTYPE_POLL_GROUP,
	/* server, XX, NA, nb_req_grouped, NA */
	XPAUL_FUNCTYPE_POLL_POLLANY,
	/* Les suivants ne sont pas encore utilisés... */
	/* server, XX, req to block, NA, NA */
	XPAUL_FUNCTYPE_BLOCK_WAITONE,
	XPAUL_FUNCTYPE_BLOCK_WAITONE_TIMEOUT,
	XPAUL_FUNCTYPE_BLOCK_GROUP,
	XPAUL_FUNCTYPE_BLOCK_WAITANY,
	XPAUL_FUNCTYPE_BLOCK_WAITANY_TIMEOUT,
	XPAUL_FUNCTYPE_UNBLOCK_WAITONE,
	XPAUL_FUNCTYPE_UNBLOCK_WAITANY,
	/* PRIVATE */
	XPAUL_FUNCTYPE_SIZE
} xpaul_op_t;

/*[C]****************************************************************
 * Rapidité d'un callback 
 */
typedef enum {
        XPAUL_CALLBACK_SLOWEST=1,
	XPAUL_CALLBACK_SLOW=2,
	XPAUL_CALLBACK_NORMAL_SPEED=3,
	XPAUL_CALLBACK_FAST=4,
	XPAUL_CALLBACK_IMMEDIATE=5,
} xpaul_callback_speed_t;

/*[C]****************************************************************
 * Le prototype des call-back
 * server : le serveur
 * op : l'opération (call-back) demandée
 * req : pour *(POLL|WAIT)ONE* : la requête à tester en particulier
 * nb_req : pour POLL_* : le nombre de requêtes groupées
 * option : flags dépendant de l'opération
 *  - pour POLL_POLLONE : 
 *     + XPAUL_IS_GROUPED : si la requête est déjà groupée
 *     + XPAUL_ITER : si POLL_POLLONE est appelée sur toutes les requêtes
 *                 en attente (ie POLL_POLLANY n'est pas disponible)
 *
 * La valeur de retour est pour l'instant ignorée.
 */
typedef int (xpaul_callback_t)(xpaul_server_t server, 
			    xpaul_op_t op,
			    xpaul_req_t req, 
			    int nb_ev, int option);

typedef struct{
  xpaul_callback_t *func;
  xpaul_callback_speed_t speed;
}xpaul_pcallback_t;

/****************************************************************
 * État d'un serveur d'événements
 */
enum {
	/* Non initialisé */
	XPAUL_SERVER_STATE_NONE,
	/* En cours d'initialisation */
	XPAUL_SERVER_STATE_INIT=1,
	/* En fonctionnement */
	XPAUL_SERVER_STATE_LAUNCHED=2,
	/* En arrêt */
	XPAUL_SERVER_STATE_HALTED=3,
};

#ifdef MARCEL
#include "marcel.h"
#endif

/****************************************************************
 * Structure serveur d'événements
 */
struct xpaul_server {
#ifdef MARCEL
	/* Spinlock régissant l'accès à cette structure */
        // TODO inutile sans marcel ? 
        ma_spinlock_t lock;
	/* Thread propriétaire du lock (pour le locking applicatif) */
        // TODO inutile sans marcel ? 
	marcel_task_t *lock_owner;
#else
        // foo
        void *lock;
        void *lock_owner;
#endif // MARCEL

	/* Liste des requêtes soumises */
	struct list_head list_req_registered;
	/* Liste des requêtes signalées prêtes par les call-backs */
	struct list_head list_req_ready;
	/* Liste des requêtes pour ceux en attente dans la liste précédente */
	struct list_head list_req_success;
	/* Liste des attentes en cours sur les événements */
	struct list_head list_id_waiters;
#ifdef MARCEL
	/* Spinlock régissant l'accès à la liste précédente */
	ma_spinlock_t req_success_lock;
#else
        void *req_success_lock; // foo
#endif // MARCEL
	/* Polling events registered but not yet polled */
	int registered_req_not_yet_polled;

	/* Liste des requêtes groupées pour le polling (ou en cours de groupage) */
	struct list_head list_req_poll_grouped;
	/* Nombre de requêtes dans la liste précédente */
	int req_poll_grouped_nb;

	/* Call-backs */
        xpaul_pcallback_t funcs[XPAUL_FUNCTYPE_SIZE];
        
        /* Points et fréquence de scrutation */
	unsigned poll_points;
	unsigned frequency;

        /* Nombre de scrutations à effectuer avant d'effectuer un appel bloquant 
	 * ( -1 : ne jamais utiliser d'appel bloquant) */
        int max_poll;

	/* Chaine des serveurs en cours de polling 
	   (state == 2 et tâches en attente de polling) */
	struct list_head chain_poll;
	
#ifdef MARCEL
        /* Tasklet et timer utilisée pour la scrutation */
        // TODO inutile sans marcel ? 
	struct ma_tasklet_struct poll_tasklet;
	struct ma_timer_list poll_timer;
#endif
	/* État du serveur */
	int state;
	/* Nom (utilisé pour le débug) */
	char* name;
};



/*[S]****************************************************************
 * Initialisation d'un serveur
 */

/* Initialisation statique
 * var : la variable constante xpaul_server_t
 * name: une chaîne de caractères pour identifier ce serveur dans les
 *        messages de debug
 */
#define XPAUL_SERVER_DEFINE(var, name) \
  struct xpaul_server ma_s_#var = XPAUL_SERVER_INIT(ma_s_#var, name); \
  const XPAUL_SERVER_DECLARE(var) = &ma_s_#var


/* Idem, mais dynamique */
void xpaul_server_init(xpaul_server_t server, char* name);

/* Réglage des paramètres de scrutation */
int xpaul_server_set_poll_settings(xpaul_server_t server,
				unsigned poll_points,
				unsigned poll_frequency,
				int max_poll);

/* Enregistrement des call-backs utilisables */
__tbx_inline__ static int xpaul_server_add_callback(xpaul_server_t server, 
						xpaul_op_t op,
						 xpaul_pcallback_t func);

/* Démarrage du serveur
 * Il devient possible d'attendre des événements
 */
int xpaul_server_start(xpaul_server_t server);

/* Arrêt du serveur
 * Il est nécessaire de le réinitialiser pour le redémarrer
 */
int xpaul_server_stop(xpaul_server_t server);


// THESE: PROTO_XPAUL_WAIT START
/*[U]****************************************************************
 * Fonctions à l'usage des threads applicatifs
 */

/* Attribut pouvant être attaché aux événements */
enum {
  /* Désactive la requête lorsque survient l'occurence
   * suivante de l'événement */
  XPAUL_ATTR_ONE_SHOT=1,
  /* Ne réveille pas les threads en attente d'événements du
   * serveur (ie xpaul_server_wait() ) */
  XPAUL_ATTR_NO_WAKE_SERVER=2,
};

/* int xpaul_per_lwp_polling_register(int *data, */
/*                                     int value_to_match, */
/*                                     void (*func) (void *), */
/*                                     void *arg); */

/* void xpaul_per_lwp_polling_proceed(void); */

#ifndef MARCEL
#define marcel_time_t int
#endif // MARCEL
// TODO supprimer marcel_time_t (compatibilité sans Marcel)
int xpaul_wait(xpaul_server_t server, xpaul_req_t req,
		   xpaul_wait_t wait, marcel_time_t timeout);

/* Un raccourci pratique des fonctions suivantes, utile si l'on ne
 * soumet la requête qu'une seule fois. Les opérations suivantes sont
 * effectuées : initialisation, soumission et attente d'un
 * événement avec ONE_SHOT positionné */

/* Initialisation d'un événement
 * (à appeler en premier si l'on utilise autre chose que xpaul_wait) */
int xpaul_req_init(xpaul_req_t req);

/* Ajout d'un attribut spécifique à une requête */
int xpaul_req_attr_set(xpaul_req_t req, int attr);

/* Soumission d'une requête (le serveur PEUT commencer à scruter si cela
 * lui convient) */
int xpaul_req_submit(xpaul_server_t server, xpaul_req_t req);

/* Abandon d'une requête et retour des threads en attente sur cette
 * requête (avec le code de retour fourni) */
int xpaul_req_cancel(xpaul_req_t req, int ret_code);

/* Attente bloquante d'un événement sur une requête déjà enregistrée */
// TODO supprimer marcel_time_t (compatibilité sans Marcel)
int xpaul_req_wait(xpaul_req_t req, xpaul_wait_t wait, 
		       marcel_time_t timeout);

/* Attente bloquante d'un événement sur une quelconque requête du serveur */
int xpaul_server_wait(xpaul_server_t server, marcel_time_t timeout);

/* Renvoie une requête survenue n'ayant pas l'attribut NO_WAKE_SERVER
 * (utile au retour de server_wait())
 * À l'abandon d'une requête (wait, req_cancel ou ONE_SHOT) la requête
 * est également retirée de cette file (donc n'est plus consultable) */
xpaul_req_t xpaul_get_success_req(xpaul_server_t server);

// THESE: PROTO_XPAUL_WAIT STOP
/* Exclusion mutuelle pour un serveur d'événements
 *
 * - les call-backs sont TOUJOURS appelés à l'intérieur de cette
 *   exclusion mutuelle.
 * - les call-backs BLOCK_ONE|ALL doivent relâcher puis reprendre ce
 *   lock avant et après l'appel système bloquant avec les deux fonctions
 *   prévues pour (xpaul_callback_*).
 * - les fonctions précédentes peuvent être appelées avec ou sans ce
 *   lock
 * - le lock est relâché automatiquement par les fonctions d'attente
 *   (xpaul_*wait*())

 * - les call-backs et les réveils des threads en attente sur les
 *   événements signalés par le call-back sont atomiques (vis-à-vis de 
 *   ce lock)
 * - si le lock est pris avant les fonctions d'attente (xpaul_wait_*),
 *   la mise en attente est atomique vis-à-vis des call-backs
 *   ie: un call-back signalant l'événement attendu réveillera cette
 *   attente
 * - si un événement à la propriété ONE_SHOT, le désenregistrement est
 *   atomic vis-à-vis du call-back qui a généré l'événement.
 */
int xpaul_lock(xpaul_server_t server);
int xpaul_unlock(xpaul_server_t server);
/* Pour BLOCK_ONE et BLOCK_ALL avant et après l'appel système bloquant */
int xpaul_callback_will_block(xpaul_server_t server);
int xpaul_callback_has_blocked(xpaul_server_t server);

/* Appel forcé de la fonction de scrutation */
void xpaul_poll_force(xpaul_server_t server);
/* Idem, mais on est sûr que l'appel a été fait quand on retourne */
void xpaul_poll_force_sync(xpaul_server_t server);

/*[S]****************************************************************
 * Les constantes pour le polling
 * Elles peuvent être ORed
 */
#define XPAUL_POLL_AT_TIMER_SIG  1
#define XPAUL_POLL_AT_YIELD      2
#define XPAUL_POLL_AT_LIB_ENTRY  4
#define XPAUL_POLL_AT_IDLE       8





/*[C]****************************************************************
 * Les flags des call-backs (voir ci-dessus)
 */
enum {
	/* Pour POLL_POLLONE */
	XPAUL_OPT_REQ_IS_GROUPED=1,
	XPAUL_OPT_REQ_ITER=2,
};

/*[C]****************************************************************
 * Fonctions d'aide à la décision
 */
/* Nombre de threads de communication */
static int nb_comm_threads;

#ifdef MARCEL
/* Nombre de threads actifs */
__tbx_inline__ static int NB_RUNNING_THREADS(void){
  int res;
  marcel_threadslist(0,NULL, &res, NOT_BLOCKED_ONLY);  
  return res-nb_comm_threads;
}
#endif // MARCEL

/*[C]****************************************************************
 * Itérateurs pour les call-backs
 */

/****************************************************************
 * Itérateur pour les requêtes enregistrées d'un serveur
 */

/* Itérateur avec le type de base
   xpaul_req_t req : itérateur
   xpaul_server_t server : serveur
*/
#define FOREACH_REQ_REGISTERED_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_registered, chain_req_registered)

/* Idem mais protégé (usage interne) */
#define FOREACH_REQ_REGISTERED_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_registered, chain_req_registered)

/* Itérateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (itérateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul dans la structure pointée par req
*/
#define FOREACH_REQ_REGISTERED(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_registered, member.chain_req_registered)


/****************************************************************
 * Itérateur pour les requêtes groupées de la scrutation (polling)
 */

/* Itérateur avec le type de base
   xpaul_req_t req : itérateur
   xpaul_server_t server : serveur
*/
#define FOREACH_REQ_POLL_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem mais protégé (usage interne) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Itérateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (itérateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul_ev dans la structure pointée par req
*/
#define FOREACH_REQ_POLL(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)



/****************************************************************
 * Itérateur pour les requêtes groupées de l'attente bloquante
 */

/* Itérateur avec le type de base
   xpaul_req_t req : itérateur
   xpaul_server_t server : serveur
*/
#define FOREACH_REQ_BLOCK_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_block, chain_req)

/* Idem mais protégé (usage interne) */
#define FOREACH_REQ_BLOCK_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_block, chain_req)

/* Itérateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (itérateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul_ev dans la structure pointée par req
*/
#define FOREACH_REQ_BLOCK(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_block, member.chain_req)



/****************************************************************
 * Itérateur pour les attentes d'événements rattachés à une requête
 */

/* Itérateur avec le type de base
   xpaul_wait_t wait : itérateur
   xpaul_req_t req : requête
*/
#define FOREACH_WAIT_BASE(wait, req) \
  list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem mais protégé (usage interne) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req) \
  list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* Itérateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (itérateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul_ev dans la structure pointée par req
*/
#define FOREACH_WAIT(wait, req, member) \
  list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)



/* Macro utilisable dans les call-backs pour indiquer qu'une requête
   a reçu un événement.

   * la requête sera fournie à _xpaul_get_success_req() (tant
   qu'elle n'est pas désenregistrée)
   * les threads encore en attente d'un événement lié à cette requête seront
   réveillés et renverront le code 0

   xpaul_req_t req : requête
*/
#define XPAUL_REQ_SUCCESS(req) \
  do { \
        list_del(&(req)->chain_req_ready); \
        list_add(&(req)->chain_req_ready, &(req)->server->list_req_ready); \
  } while(0)
 

/* Macro utilisable dans les call-backs pour réveiller un thread en attente
   en lui fournissant le code de retour
   xpaul_wait_t wait : l'événement à reveiller
*/
#define XPAUL_WAIT_SUCCESS(wait, code) \
  do { \
        list_del(&(wait)->chain_wait); \
        (wait)->ret_code=(code); \
  } while(0)
 
// THESE: INTERFACE STOP


// =============== PRIVATE ===============
/****************************************************************
 * Ce qui suit n'a pas vocation à être utilisé hors de XPaulette
 */

// #ifndef __cplusplus
// __tbx_inline__ static void marcel_xpaul_server_init(marcel_xpaul_server_t server, char* name)
// {
// 	*server=(struct marcel_xpaul_server)MARCEL_XPAUL_SERVER_INIT(*server, name);
// }
// #endif

__tbx_inline__ static int xpaul_server_add_callback(xpaul_server_t server, 
						 xpaul_op_t op,
						 xpaul_pcallback_t func)
{
#ifdef XPAUL_FILE_DEBUG
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	XPAUL_BUG_ON(server->state != XPAUL_SERVER_STATE_INIT);
	/* On vérifie que l'index est correct */
	XPAUL_BUG_ON(op>=XPAUL_FUNCTYPE_SIZE || op<0);
	/* On vérifie que la fonction n'est pas déjà là */
	XPAUL_BUG_ON(server->funcs[op].func!=NULL);
#endif
	server->funcs[op]=func;
	return 0;
}


/* Utilisé dans les initialiseurs */
void xpaul_poll_from_tasklet(unsigned long id);
void xpaul_poll_timer(unsigned long id);


#define XPAUL_SERVER_DECLARE(var) \
  const xpaul_server_t var

#ifdef MARCEL
#define XPAUL_SERVER_INIT(var, sname) \
  { \
    .lock=MA_SPIN_LOCK_UNLOCKED, \
    .lock_owner=NULL, \
    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered), \
    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready), \
    .list_req_success=LIST_HEAD_INIT((var).list_req_success), \
    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters), \
    .req_success_lock=MA_SPIN_LOCK_UNLOCKED, \
    .registered_req_not_yet_polled=0, \
    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped), \
    .req_poll_grouped_nb=0, \
    .poll_points=0, \
    .frequency=0, \
    .max_poll=-1, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &xpaul_poll_from_tasklet, \
                     (unsigned long)(xpaul_server_t)&(var) ), \
    .poll_timer= MA_TIMER_INITIALIZER(xpaul_poll_timer, 0, \
                   (unsigned long)(xpaul_server_t)&(var)), \
    .state=XPAUL_SERVER_STATE_INIT, \
    .name=sname, \
  }
#else
#define XPAUL_SERVER_INIT(var, sname) \
  { \
    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered), \
    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready), \
    .list_req_success=LIST_HEAD_INIT((var).list_req_success), \
    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters), \
    .registered_req_not_yet_polled=0, \
    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped), \
    .req_poll_grouped_nb=0, \
    .poll_points=0, \
    .frequency=0, \
    .max_poll=-1, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .state=XPAUL_SERVER_STATE_INIT, \
    .name=sname, \
  }
#endif // MARCEL


enum {
	XPAUL_STATE_OCCURED=1,
	XPAUL_STATE_GROUPED=2,
	XPAUL_STATE_ONE_SHOT=4,
	XPAUL_STATE_NO_WAKE_SERVER=8,
	XPAUL_STATE_REGISTERED=16,
};

/* Méthode à utiliser pour une requête */
typedef enum {
        /* Détermine la meilleur méthode à utiliser automatiquement */
        XPAUL_FUNC_AUTO=1,
	/* Force l'utilisation de la fonction de polling (ie la
	 * requête va se terminer très prochainement)*/
	XPAUL_FUNC_POLLING=2,
	/* Force l'utilisation du syscall (ie la requête va prendre
	 * beaucoup de temps) */
	XPAUL_FUNC_SYSCALL=3,
} xpaul_func_to_use_t;

/* Priorité de la requête (à étudier) */
typedef enum {
        XPAUL_REQ_PRIORITY_LOWEST=0,
	XPAUL_REQ_PRIORITY_LOW=1,
	XPAUL_REQ_PRIORITY_NORMAL=2,
	XPAUL_REQ_PRIORITY_HIGH=3,
	XPAUL_REQ_PRIORITY_HIGHEST=4,
} xpaul_req_priority_t;

struct xpaul_req {
        /* Méthode à utiliser pour cette requête */
        xpaul_func_to_use_t func_to_use;
        /* Priorité de la requête */
        xpaul_req_priority_t priority;

	/* Chaine des requêtes soumises */
	struct list_head chain_req_registered;
	/* Chaine des requêtes groupées en attente */
	struct list_head chain_req_grouped;
	/* Chaine des requêtes signalées OK par un call-back */
	struct list_head chain_req_ready;
	/* Chaine des requêtes à signaler au serveur */
	struct list_head chain_req_success;
	/* État */
	int state;
	/* Serveur attaché */
	xpaul_server_t server;
	/* Liste des attentes en cours sur cette requête */
	struct list_head list_wait;
};

struct xpaul_wait {
	/* Chaine des événements groupé en attente */
	struct list_head chain_wait;

#ifdef MARCEL
        // TODO inutile sans marcel ? 
        marcel_sem_t sem;
	marcel_task_t *task;
#endif
	/* 0: event
	   -ECANCELED: marcel_unregister called
	*/
	int ret;
};


#include "tbx_compiler.h"
/* Liste des serveurs en cours de polling 
 * (state == 2 et tâches en attente de polling) 
 */
extern struct list_head xpaul_list_poll;

static __tbx_inline__ int xpaul_polling_is_required(unsigned polling_point)
TBX_UNUSED;
static __tbx_inline__ void xpaul_check_polling(unsigned polling_point)
TBX_UNUSED;
void __xpaul_check_polling(unsigned polling_point);

static __tbx_inline__ int xpaul_polling_is_required(unsigned polling_point)
{
	return !list_empty(&xpaul_list_poll);
}

static __tbx_inline__ void xpaul_check_polling(unsigned polling_point)
{
	if(xpaul_polling_is_required(polling_point))
		__xpaul_check_polling(polling_point);
}


#define XPAUL_EXCEPTION_RAISE(e) fprintf(stderr, "%s\n", e), exit(1)

#endif
