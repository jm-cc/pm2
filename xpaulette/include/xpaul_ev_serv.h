
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
 * Certains commentaires sont �tiquet�s par U/S/C/I signifiant
 * User       : utile aux threads applicatifs pour exploiter ces m�canismes
 * Start      : utile pour l'initialisation
 * Call-backs : utile pour le concepteur de call-backs
 * Internal   : ne devrait pas �tre utilis� hors de XPaulette
 *
 * Ce ne sont que des indications, pas des contraintes
 ****************************************************************/

/*[SC]****************************************************************
 * R�cup�re l'adresse d'une structure englobante 
 * Il faut donner :
 * - l'adresse du champ interne
 * - le type de la structure englobante
 * - le nom du champ interne dans la structure englobante
 */
#define struct_up(ptr, type, member) \
	tbx_container_of(ptr, type, member)


/*[U]****************************************************************
 * Les types abstaits pour le serveur et les �v�nements
 */
/* Le serveur d�finit les call-backs et les param�tres de scrutation �
 * utiliser. Il va �ventuellement regrouper les ressources
 * enregistr�es (en cas de scrutation active par exemple)
 */
typedef struct xpaul_server *xpaul_server_t;

/* Une requ�te d�finit une entit� qui pourra recevoir un �v�nement que
 * l'on pourra attendre.  Diverses requ�tes d'un serveur pourront �tre
 * group�es (afin de d�terminer plus rapidement l'ensemble des �tats
 * de chacune des requ�tes � chaque scrutation)
 */
typedef struct xpaul_req *xpaul_req_t;

/* Un thread peut attendre l'arriv�e effective d'un �v�nement
 * correspondant � une requ�te. Le thread est d�schedul� tant que la
 * requ�te n'est pas pr�te (que l'�v�nement n'est pas re�u).
 */
typedef struct xpaul_wait *xpaul_wait_t;

/*[SC]****************************************************************
 * Les diff�rents op�rations call-backs possibles
 */
typedef enum {
	/* server, XX, req to poll, nb_req_grouped, flags */
	XPAUL_FUNCTYPE_POLL_POLLONE,
	/* server, XX, NA, nb_req_grouped, NA */
	XPAUL_FUNCTYPE_POLL_GROUP,
	/* server, XX, NA, nb_req_grouped, NA */
	XPAUL_FUNCTYPE_POLL_POLLANY,
	/* Les suivants ne sont pas encore utilis�s... */
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
 * Rapidit� d'un callback 
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
 * op : l'op�ration (call-back) demand�e
 * req : pour *(POLL|WAIT)ONE* : la requ�te � tester en particulier
 * nb_req : pour POLL_* : le nombre de requ�tes group�es
 * option : flags d�pendant de l'op�ration
 *  - pour POLL_POLLONE : 
 *     + XPAUL_IS_GROUPED : si la requ�te est d�j� group�e
 *     + XPAUL_ITER : si POLL_POLLONE est appel�e sur toutes les requ�tes
 *                 en attente (ie POLL_POLLANY n'est pas disponible)
 *
 * La valeur de retour est pour l'instant ignor�e.
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
 * �tat d'un serveur d'�v�nements
 */
enum {
	/* Non initialis� */
	XPAUL_SERVER_STATE_NONE,
	/* En cours d'initialisation */
	XPAUL_SERVER_STATE_INIT=1,
	/* En fonctionnement */
	XPAUL_SERVER_STATE_LAUNCHED=2,
	/* En arr�t */
	XPAUL_SERVER_STATE_HALTED=3,
};

#ifdef MARCEL
#include "marcel.h"
#endif

/****************************************************************
 * Structure serveur d'�v�nements
 */
struct xpaul_server {
#ifdef MARCEL
	/* Spinlock r�gissant l'acc�s � cette structure */
        // TODO inutile sans marcel ? 
        ma_spinlock_t lock;
	/* Thread propri�taire du lock (pour le locking applicatif) */
        // TODO inutile sans marcel ? 
	marcel_task_t *lock_owner;
#else
        // foo
        void *lock;
        void *lock_owner;
#endif // MARCEL

	/* Liste des requ�tes soumises */
	struct list_head list_req_registered;
	/* Liste des requ�tes signal�es pr�tes par les call-backs */
	struct list_head list_req_ready;
	/* Liste des requ�tes pour ceux en attente dans la liste pr�c�dente */
	struct list_head list_req_success;
	/* Liste des attentes en cours sur les �v�nements */
	struct list_head list_id_waiters;
#ifdef MARCEL
	/* Spinlock r�gissant l'acc�s � la liste pr�c�dente */
	ma_spinlock_t req_success_lock;
#else
        void *req_success_lock; // foo
#endif // MARCEL
	/* Polling events registered but not yet polled */
	int registered_req_not_yet_polled;

	/* Liste des requ�tes group�es pour le polling (ou en cours de groupage) */
	struct list_head list_req_poll_grouped;
	/* Nombre de requ�tes dans la liste pr�c�dente */
	int req_poll_grouped_nb;

	/* Call-backs */
        xpaul_pcallback_t funcs[XPAUL_FUNCTYPE_SIZE];
        
        /* Points et fr�quence de scrutation */
	unsigned poll_points;
	unsigned frequency;

        /* Nombre de scrutations � effectuer avant d'effectuer un appel bloquant 
	 * ( -1 : ne jamais utiliser d'appel bloquant) */
        int max_poll;

	/* Chaine des serveurs en cours de polling 
	   (state == 2 et t�ches en attente de polling) */
	struct list_head chain_poll;
	
#ifdef MARCEL
        /* Tasklet et timer utilis�e pour la scrutation */
        // TODO inutile sans marcel ? 
	struct ma_tasklet_struct poll_tasklet;
	struct ma_timer_list poll_timer;
#endif
	/* �tat du serveur */
	int state;
	/* Nom (utilis� pour le d�bug) */
	char* name;
};



/*[S]****************************************************************
 * Initialisation d'un serveur
 */

/* Initialisation statique
 * var : la variable constante xpaul_server_t
 * name: une cha�ne de caract�res pour identifier ce serveur dans les
 *        messages de debug
 */
#define XPAUL_SERVER_DEFINE(var, name) \
  struct xpaul_server ma_s_#var = XPAUL_SERVER_INIT(ma_s_#var, name); \
  const XPAUL_SERVER_DECLARE(var) = &ma_s_#var


/* Idem, mais dynamique */
void xpaul_server_init(xpaul_server_t server, char* name);

/* R�glage des param�tres de scrutation */
int xpaul_server_set_poll_settings(xpaul_server_t server,
				unsigned poll_points,
				unsigned poll_frequency,
				int max_poll);

/* Enregistrement des call-backs utilisables */
__tbx_inline__ static int xpaul_server_add_callback(xpaul_server_t server, 
						xpaul_op_t op,
						 xpaul_pcallback_t func);

/* D�marrage du serveur
 * Il devient possible d'attendre des �v�nements
 */
int xpaul_server_start(xpaul_server_t server);

/* Arr�t du serveur
 * Il est n�cessaire de le r�initialiser pour le red�marrer
 */
int xpaul_server_stop(xpaul_server_t server);


// THESE: PROTO_XPAUL_WAIT START
/*[U]****************************************************************
 * Fonctions � l'usage des threads applicatifs
 */

/* Attribut pouvant �tre attach� aux �v�nements */
enum {
  /* D�sactive la requ�te lorsque survient l'occurence
   * suivante de l'�v�nement */
  XPAUL_ATTR_ONE_SHOT=1,
  /* Ne r�veille pas les threads en attente d'�v�nements du
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
// TODO supprimer marcel_time_t (compatibilit� sans Marcel)
int xpaul_wait(xpaul_server_t server, xpaul_req_t req,
		   xpaul_wait_t wait, marcel_time_t timeout);

/* Un raccourci pratique des fonctions suivantes, utile si l'on ne
 * soumet la requ�te qu'une seule fois. Les op�rations suivantes sont
 * effectu�es : initialisation, soumission et attente d'un
 * �v�nement avec ONE_SHOT positionn� */

/* Initialisation d'un �v�nement
 * (� appeler en premier si l'on utilise autre chose que xpaul_wait) */
int xpaul_req_init(xpaul_req_t req);

/* Ajout d'un attribut sp�cifique � une requ�te */
int xpaul_req_attr_set(xpaul_req_t req, int attr);

/* Soumission d'une requ�te (le serveur PEUT commencer � scruter si cela
 * lui convient) */
int xpaul_req_submit(xpaul_server_t server, xpaul_req_t req);

/* Abandon d'une requ�te et retour des threads en attente sur cette
 * requ�te (avec le code de retour fourni) */
int xpaul_req_cancel(xpaul_req_t req, int ret_code);

/* Attente bloquante d'un �v�nement sur une requ�te d�j� enregistr�e */
// TODO supprimer marcel_time_t (compatibilit� sans Marcel)
int xpaul_req_wait(xpaul_req_t req, xpaul_wait_t wait, 
		       marcel_time_t timeout);

/* Attente bloquante d'un �v�nement sur une quelconque requ�te du serveur */
int xpaul_server_wait(xpaul_server_t server, marcel_time_t timeout);

/* Renvoie une requ�te survenue n'ayant pas l'attribut NO_WAKE_SERVER
 * (utile au retour de server_wait())
 * � l'abandon d'une requ�te (wait, req_cancel ou ONE_SHOT) la requ�te
 * est �galement retir�e de cette file (donc n'est plus consultable) */
xpaul_req_t xpaul_get_success_req(xpaul_server_t server);

// THESE: PROTO_XPAUL_WAIT STOP
/* Exclusion mutuelle pour un serveur d'�v�nements
 *
 * - les call-backs sont TOUJOURS appel�s � l'int�rieur de cette
 *   exclusion mutuelle.
 * - les call-backs BLOCK_ONE|ALL doivent rel�cher puis reprendre ce
 *   lock avant et apr�s l'appel syst�me bloquant avec les deux fonctions
 *   pr�vues pour (xpaul_callback_*).
 * - les fonctions pr�c�dentes peuvent �tre appel�es avec ou sans ce
 *   lock
 * - le lock est rel�ch� automatiquement par les fonctions d'attente
 *   (xpaul_*wait*())

 * - les call-backs et les r�veils des threads en attente sur les
 *   �v�nements signal�s par le call-back sont atomiques (vis-�-vis de 
 *   ce lock)
 * - si le lock est pris avant les fonctions d'attente (xpaul_wait_*),
 *   la mise en attente est atomique vis-�-vis des call-backs
 *   ie: un call-back signalant l'�v�nement attendu r�veillera cette
 *   attente
 * - si un �v�nement � la propri�t� ONE_SHOT, le d�senregistrement est
 *   atomic vis-�-vis du call-back qui a g�n�r� l'�v�nement.
 */
int xpaul_lock(xpaul_server_t server);
int xpaul_unlock(xpaul_server_t server);
/* Pour BLOCK_ONE et BLOCK_ALL avant et apr�s l'appel syst�me bloquant */
int xpaul_callback_will_block(xpaul_server_t server);
int xpaul_callback_has_blocked(xpaul_server_t server);

/* Appel forc� de la fonction de scrutation */
void xpaul_poll_force(xpaul_server_t server);
/* Idem, mais on est s�r que l'appel a �t� fait quand on retourne */
void xpaul_poll_force_sync(xpaul_server_t server);

/*[S]****************************************************************
 * Les constantes pour le polling
 * Elles peuvent �tre ORed
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
 * Fonctions d'aide � la d�cision
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
 * It�rateurs pour les call-backs
 */

/****************************************************************
 * It�rateur pour les requ�tes enregistr�es d'un serveur
 */

/* It�rateur avec le type de base
   xpaul_req_t req : it�rateur
   xpaul_server_t server : serveur
*/
#define FOREACH_REQ_REGISTERED_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_registered, chain_req_registered)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_REGISTERED_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_registered, chain_req_registered)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (it�rateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul dans la structure point�e par req
*/
#define FOREACH_REQ_REGISTERED(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_registered, member.chain_req_registered)


/****************************************************************
 * It�rateur pour les requ�tes group�es de la scrutation (polling)
 */

/* It�rateur avec le type de base
   xpaul_req_t req : it�rateur
   xpaul_server_t server : serveur
*/
#define FOREACH_REQ_POLL_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (it�rateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul_ev dans la structure point�e par req
*/
#define FOREACH_REQ_POLL(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)



/****************************************************************
 * It�rateur pour les requ�tes group�es de l'attente bloquante
 */

/* It�rateur avec le type de base
   xpaul_req_t req : it�rateur
   xpaul_server_t server : serveur
*/
#define FOREACH_REQ_BLOCK_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_block, chain_req)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_BLOCK_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_block, chain_req)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (it�rateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul_ev dans la structure point�e par req
*/
#define FOREACH_REQ_BLOCK(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_block, member.chain_req)



/****************************************************************
 * It�rateur pour les attentes d'�v�nements rattach�s � une requ�te
 */

/* It�rateur avec le type de base
   xpaul_wait_t wait : it�rateur
   xpaul_req_t req : requ�te
*/
#define FOREACH_WAIT_BASE(wait, req) \
  list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req) \
  list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct xpaul_req
                     (it�rateur)
   xpaul_server_t server : serveur
   member : nom de struct xpaul_ev dans la structure point�e par req
*/
#define FOREACH_WAIT(wait, req, member) \
  list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)



/* Macro utilisable dans les call-backs pour indiquer qu'une requ�te
   a re�u un �v�nement.

   * la requ�te sera fournie � _xpaul_get_success_req() (tant
   qu'elle n'est pas d�senregistr�e)
   * les threads encore en attente d'un �v�nement li� � cette requ�te seront
   r�veill�s et renverront le code 0

   xpaul_req_t req : requ�te
*/
#define XPAUL_REQ_SUCCESS(req) \
  do { \
        list_del(&(req)->chain_req_ready); \
        list_add(&(req)->chain_req_ready, &(req)->server->list_req_ready); \
  } while(0)
 

/* Macro utilisable dans les call-backs pour r�veiller un thread en attente
   en lui fournissant le code de retour
   xpaul_wait_t wait : l'�v�nement � reveiller
*/
#define XPAUL_WAIT_SUCCESS(wait, code) \
  do { \
        list_del(&(wait)->chain_wait); \
        (wait)->ret_code=(code); \
  } while(0)
 
// THESE: INTERFACE STOP


// =============== PRIVATE ===============
/****************************************************************
 * Ce qui suit n'a pas vocation � �tre utilis� hors de XPaulette
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
	/* Cette fonction doit �tre appel�e entre l'initialisation et
	 * le d�marrage de ce serveur d'�v�nements */
	XPAUL_BUG_ON(server->state != XPAUL_SERVER_STATE_INIT);
	/* On v�rifie que l'index est correct */
	XPAUL_BUG_ON(op>=XPAUL_FUNCTYPE_SIZE || op<0);
	/* On v�rifie que la fonction n'est pas d�j� l� */
	XPAUL_BUG_ON(server->funcs[op].func!=NULL);
#endif
	server->funcs[op]=func;
	return 0;
}


/* Utilis� dans les initialiseurs */
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

/* M�thode � utiliser pour une requ�te */
typedef enum {
        /* D�termine la meilleur m�thode � utiliser automatiquement */
        XPAUL_FUNC_AUTO=1,
	/* Force l'utilisation de la fonction de polling (ie la
	 * requ�te va se terminer tr�s prochainement)*/
	XPAUL_FUNC_POLLING=2,
	/* Force l'utilisation du syscall (ie la requ�te va prendre
	 * beaucoup de temps) */
	XPAUL_FUNC_SYSCALL=3,
} xpaul_func_to_use_t;

/* Priorit� de la requ�te (� �tudier) */
typedef enum {
        XPAUL_REQ_PRIORITY_LOWEST=0,
	XPAUL_REQ_PRIORITY_LOW=1,
	XPAUL_REQ_PRIORITY_NORMAL=2,
	XPAUL_REQ_PRIORITY_HIGH=3,
	XPAUL_REQ_PRIORITY_HIGHEST=4,
} xpaul_req_priority_t;

struct xpaul_req {
        /* M�thode � utiliser pour cette requ�te */
        xpaul_func_to_use_t func_to_use;
        /* Priorit� de la requ�te */
        xpaul_req_priority_t priority;

	/* Chaine des requ�tes soumises */
	struct list_head chain_req_registered;
	/* Chaine des requ�tes group�es en attente */
	struct list_head chain_req_grouped;
	/* Chaine des requ�tes signal�es OK par un call-back */
	struct list_head chain_req_ready;
	/* Chaine des requ�tes � signaler au serveur */
	struct list_head chain_req_success;
	/* �tat */
	int state;
	/* Serveur attach� */
	xpaul_server_t server;
	/* Liste des attentes en cours sur cette requ�te */
	struct list_head list_wait;
};

struct xpaul_wait {
	/* Chaine des �v�nements group� en attente */
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
 * (state == 2 et t�ches en attente de polling) 
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
