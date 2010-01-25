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
#ifdef __MARCEL_KERNEL__
#include "tbx_fast_list.h"
#endif


/** Public macros **/
/*[S]****************************************************************
 * Initialisation d'un serveur
 */

/* Initialisation statique
 * var : la variable constante marcel_ev_server_t
 * name: une cha�ne de caract�res pour identifier ce serveur dans les
 *        messages de debug
 */
#define MARCEL_EV_SERVER_DEFINE(var, name) \
  struct marcel_ev_server ma_s_#var = MARCEL_EV_SERVER_INIT(ma_s_#var, name); \
  const MARCEL_EV_SERVER_DECLARE(var) = &ma_s_#var

/*[S]****************************************************************
 * Les constantes pour le polling
 * Elles peuvent �tre ORed
 */
#define MARCEL_EV_POLL_AT_TIMER_SIG  1
#define MARCEL_EV_POLL_AT_YIELD      2
#define MARCEL_EV_POLL_AT_LIB_ENTRY  4
#define MARCEL_EV_POLL_AT_IDLE       8

/*[C]****************************************************************
 * It�rateurs pour les call-backs
 */

/****************************************************************
 * It�rateur pour les requ�tes group�es de la scrutation (polling)
 */

/* It�rateur avec le type de base
   marcel_ev_req_t req : it�rateur
   marcel_ev_server_t server : serveur
*/
#define FOREACH_REQ_POLL_BASE(req, server) \
  tbx_fast_list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) \
  tbx_fast_list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (it�rateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure point�e par req
*/
#define FOREACH_REQ_POLL(req, server, member) \
  tbx_fast_list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)


/****************************************************************
 * It�rateur pour les attentes d'�v�nements rattach�s � une requ�te
 */

/* It�rateur avec le type de base
   marcel_ev_wait_t wait : it�rateur
   marcel_ev_req_t req : requ�te
*/
#define FOREACH_WAIT_BASE(wait, req) \
  tbx_fast_list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req) \
  tbx_fast_list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (it�rateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure point�e par req
*/
#define FOREACH_WAIT(wait, req, member) \
  tbx_fast_list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)



/* Macro utilisable dans les call-backs pour indiquer qu'une requ�te
   a re�u un �v�nement.

   * la requ�te sera fournie � marcel_ev_get_success_req() (tant
   qu'elle n'est pas d�senregistr�e)
   * les threads encore en attente d'un �v�nement li� � cette requ�te seront
   r�veill�s et renverront le code 0

   marcel_ev_req_t req : requ�te
*/
#define MARCEL_EV_REQ_SUCCESS(req) \
  do { \
        tbx_fast_list_move(&(req)->chain_req_ready, &(req)->server->list_req_ready); \
  } while(0)


/*  =============== PRIVATE =============== */
/****************************************************************
 * Ce qui suit n'a pas vocation � �tre utilis� hors de Marcel
 */

#define MARCEL_EV_SERVER_DECLARE(var) \
  const marcel_ev_server_t var
#define MARCEL_EV_SERVER_INIT(var, sname) \
  { \
    .lock=MA_SPIN_LOCK_UNLOCKED, \
    .lock_owner=NULL, \
    .list_req_registered=TBX_FAST_LIST_HEAD_INIT((var).list_req_registered), \
    .list_req_ready=TBX_FAST_LIST_HEAD_INIT((var).list_req_ready), \
    .list_req_success=TBX_FAST_LIST_HEAD_INIT((var).list_req_success), \
    .list_id_waiters=TBX_FAST_LIST_HEAD_INIT((var).list_id_waiters), \
    .req_success_lock=MA_SPIN_LOCK_UNLOCKED, \
    .registered_req_not_yet_polled=0, \
    .list_req_poll_grouped=TBX_FAST_LIST_HEAD_INIT((var).list_req_poll_grouped), \
    .req_poll_grouped_nb=0, \
    .funcs={NULL, }, \
    .poll_points=0, \
    .frequency=0, \
    .chain_poll=TBX_FAST_LIST_HEAD_INIT((var).chain_poll), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &marcel_poll_from_tasklet, \
                     (unsigned long)(marcel_ev_server_t)&(var), 1 ), \
    .poll_timer= MA_TIMER_INITIALIZER(marcel_poll_timer, 0, \
                   (unsigned long)(marcel_ev_server_t)&(var)), \
    .state=MA_EV_SERVER_STATE_INIT, \
    .name=sname, \
  }


/** Public data types **/
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

/* Attribut pouvant �tre attach� aux �v�nements */
enum {
	/* D�sactive la requ�te lorsque survient l'occurrence
	 * suivante de l'�v�nement */
	MARCEL_EV_ATTR_ONE_SHOT=1,
	/* Ne r�veille pas les threads en attente d'�v�nements du
	 * serveur (ie marcel_ev_server_wait())*/
	MARCEL_EV_ATTR_NO_WAKE_SERVER=2,
};

/*[SC]****************************************************************
 * Les diff�rents op�rations call-backs possibles
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
	MARCEL_EV_OPT_REQ_IS_GROUPED=1,
	MARCEL_EV_OPT_REQ_ITER=2,
};


/*[C]****************************************************************
 * Le prototype des call-back
 * server : le serveur
 * op : l'op�ration (call-back) demand�e
 * req : pour *(POLL|WAIT)ONE* : la requ�te � tester en particulier
 * nb_req : pour POLL_* : le nombre de requ�tes group�es
 * option : flags d�pendant de l'op�ration
 *  - pour POLL_POLLONE :
 *     + EV_IS_GROUPED : si la requ�te est d�j� group�e
 *     + EV_ITER : si POLL_POLLONE est appel�e sur toutes les requ�tes
 *                 en attente (ie POLL_POLLANY n'est pas disponible)
 *
 * La valeur de retour est pour l'instant ignor�e.
 */
typedef struct marcel_ev_server *marcel_ev_server_t;
typedef int (marcel_ev_callback_t)(marcel_ev_server_t server,
				  marcel_ev_op_t op,
				  marcel_ev_req_t req,
				  int nb_ev, int option);
typedef marcel_ev_callback_t *marcel_ev_pcallback_t;

/*[U]****************************************************************
 * Les types abstaits pour le serveur et les �v�nements
 */
/* Le serveur d�finit les call-backs et les param�tres de scrutation �
 * utiliser. Il va �ventuellement regrouper les ressources
 * enregistr�es (en cas de scrutation active par exemple)
 */
struct marcel_ev_server {
	/* Spinlock r�gissant l'acc�s � cette structure */
	ma_spinlock_t lock;
	/* Thread propri�taire du lock (pour le locking applicatif) */
	marcel_task_t *lock_owner;
	/* Liste des requ�tes soumises */
	struct tbx_fast_list_head list_req_registered;
	/* Liste des requ�tes signal�es pr�tes par les call-backs */
	struct tbx_fast_list_head list_req_ready;
	/* Liste des requ�tes pour ceux en attente dans la liste pr�c�dente */
	struct tbx_fast_list_head list_req_success;
	/* Liste des attentes en cours sur les �v�nements */
	struct tbx_fast_list_head list_id_waiters;
	/* Spinlock r�gissant l'acc�s � la liste pr�c�dente */
	ma_spinlock_t req_success_lock;
	/* Polling events registered but not yet polled */
	int registered_req_not_yet_polled;

	/* Liste des requ�tes group�es pour le polling (ou en cours de groupage) */
	struct tbx_fast_list_head list_req_poll_grouped;
	/* Nombre de requ�tes dans la liste pr�c�dente */
	int req_poll_grouped_nb;

	/* Call-backs */
	marcel_ev_pcallback_t funcs[MA_EV_FUNCTYPE_SIZE];
	/* Points et fr�quence de scrutation */
	unsigned poll_points;
	unsigned frequency;
	/* Chaine des serveurs en cours de polling 
	   (state == 2 et t�ches en attente de polling) */
	struct tbx_fast_list_head chain_poll;
	/* Tasklet et timer utilis�e pour la scrutation */
	struct ma_tasklet_struct poll_tasklet;
	struct ma_timer_list poll_timer;
	/* �tat du serveur */
	int state;
	/* Nom (utilis� pour le d�bug) */
	const char* name;
};


/** Public functions **/
/* Idem, mais dynamique */
/* #ifndef __cplusplus */
void marcel_ev_server_init(marcel_ev_server_t server, char* name);
/* #endif */

/* Enregistrement des call-backs utilisables */
__tbx_inline__ static int marcel_ev_server_add_callback(marcel_ev_server_t server, 
						marcel_ev_op_t op,
						marcel_ev_callback_t *func);
/* R�glage des param�tres de scrutation */
int marcel_ev_server_set_poll_settings(marcel_ev_server_t server, 
				       unsigned poll_points,
				       unsigned poll_frequency);

/* D�marrage du serveur
 * Il devient possible d'attendre des �v�nements
 */
int marcel_ev_server_start(marcel_ev_server_t server);

/* Arr�t du serveur
 * Il est n�cessaire de le r�initialiser pour le red�marrer
 */
int marcel_ev_server_stop(marcel_ev_server_t server);

/*[U]****************************************************************
 * Fonctions � l'usage des threads applicatifs
 */

/* Un raccourci pratique des fonctions suivantes, utile si l'on ne
 * soumet la requ�te qu'une seule fois. Les op�rations suivantes sont
 * effectu�es : initialisation, soumission et attente d'un
 * �v�nement avec ONE_SHOT positionn� */
int marcel_ev_wait(marcel_ev_server_t server, marcel_ev_req_t req,
		   marcel_ev_wait_t wait, marcel_time_t timeout);

/* Exclusion mutuelle pour un serveur d'�v�nements
 *
 * - les call-backs sont TOUJOURS appel�s � l'int�rieur de cette
 *   exclusion mutuelle.
 * - les call-backs BLOCK_ONE|ALL doivent rel�cher puis reprendre ce
 *   lock avant et apr�s l'appel syst�me bloquant avec les deux fonctions
 *   pr�vues pour (marcel_ev_callback_*).
 * - les fonctions pr�c�dentes peuvent �tre appel�es avec ou sans ce
 *   lock
 * - le lock est rel�ch� automatiquement par les fonctions d'attente
 *   (marcel_ev_*wait*())

 * - les call-backs et les r�veils des threads en attente sur les
 *   �v�nements signal�s par le call-back sont atomiques (vis-�-vis de 
 *   ce lock)
 * - si le lock est pris avant les fonctions d'attente (ev_wait_*),
 *   la mise en attente est atomique vis-�-vis des call-backs
 *   ie: un call-back signalant l'�v�nement attendu r�veillera cette
 *   attente
 * - si un �v�nement � la propri�t� ONE_SHOT, le d�senregistrement est
 *   atomic vis-�-vis du call-back qui a g�n�r� l'�v�nement.
 */
int marcel_ev_lock(marcel_ev_server_t server);
int marcel_ev_unlock(marcel_ev_server_t server);

/* Utilis� dans les initialiseurs */
void marcel_poll_from_tasklet(unsigned long id);
void marcel_poll_timer(unsigned long id);


#ifdef __MARCEL_KERNEL__


/** Internal data types **/
/****************************************************************
 * �tat d'un serveur d'�v�nements
 */
enum {
	/* Non initialis� */
	MA_EV_SERVER_STATE_NONE,
	/* En cours d'initialisation */
	MA_EV_SERVER_STATE_INIT=1,
	/* En fonctionnement */
	MA_EV_SERVER_STATE_LAUNCHED=2,
	/* En arr�t */
	MA_EV_SERVER_STATE_HALTED=3,
};

/****************************************************************
 * Structure serveur d'�v�nements
 */
enum {
	MARCEL_EV_STATE_OCCURED=1,
	MARCEL_EV_STATE_GROUPED=2,
	MARCEL_EV_STATE_ONE_SHOT=4,
	MARCEL_EV_STATE_NO_WAKE_SERVER=8,
	MARCEL_EV_STATE_REGISTERED=16,
};


/** Internal data structures **/
/* struct marcel_ev_server { */
/* 	/\* Spinlock r�gissant l'acc�s � cette structure *\/ */
/* 	ma_spinlock_t lock; */
/* 	/\* Thread propri�taire du lock (pour le locking applicatif) *\/ */
/* 	marcel_task_t *lock_owner; */
/* 	/\* Liste des requ�tes soumises *\/ */
/* 	struct tbx_fast_list_head list_req_registered; */
/* 	/\* Liste des requ�tes signal�es pr�tes par les call-backs *\/ */
/* 	struct tbx_fast_list_head list_req_ready; */
/* 	/\* Liste des requ�tes pour ceux en attente dans la liste pr�c�dente *\/ */
/* 	struct tbx_fast_list_head list_req_success; */
/* 	/\* Liste des attentes en cours sur les �v�nements *\/ */
/* 	struct tbx_fast_list_head list_id_waiters; */
/* 	/\* Spinlock r�gissant l'acc�s � la liste pr�c�dente *\/ */
/* 	ma_spinlock_t req_success_lock; */
/* 	/\* Polling events registered but not yet polled *\/ */
/* 	int registered_req_not_yet_polled; */

/* 	/\* Liste des requ�tes group�es pour le polling (ou en cours de groupage) *\/ */
/* 	struct tbx_fast_list_head list_req_poll_grouped; */
/* 	/\* Nombre de requ�tes dans la liste pr�c�dente *\/ */
/* 	int req_poll_grouped_nb; */

/* 	/\* Call-backs *\/ */
/* 	marcel_ev_pcallback_t funcs[MA_EV_FUNCTYPE_SIZE]; */
/* 	/\* Points et fr�quence de scrutation *\/ */
/* 	unsigned poll_points; */
/* 	unsigned frequency; */
/* 	/\* Chaine des serveurs en cours de polling  */
/* 	   (state == 2 et t�ches en attente de polling) *\/ */
/* 	struct tbx_fast_list_head chain_poll; */
/* 	/\* Tasklet et timer utilis�e pour la scrutation *\/ */
/* 	struct ma_tasklet_struct poll_tasklet; */
/* 	struct ma_timer_list poll_timer; */
/* 	/\* �tat du serveur *\/ */
/* 	int state; */
/* 	/\* Nom (utilis� pour le d�bug) *\/ */
/* 	const char* name; */
/* }; */

struct marcel_ev_req {
	/* Chaine des requ�tes soumises */
	struct tbx_fast_list_head chain_req_registered;
	/* Chaine des requ�tes group�es en attente */
	struct tbx_fast_list_head chain_req_grouped;
	/* Chaine des requ�tes signal�es OK par un call-back */
	struct tbx_fast_list_head chain_req_ready;
	/* Chaine des requ�tes � signaler au serveur */
	struct tbx_fast_list_head chain_req_success;
	/* �tat */
	int state;
	/* Serveur attach� */
	marcel_ev_server_t server;
	/* Liste des attentes en cours sur cette requ�te */
	struct tbx_fast_list_head list_wait;
};

struct marcel_ev_wait {
	/* Chaine des �v�nements group� en attente */
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
 * (state == 2 et t�ches en attente de polling) 
 */
extern TBX_EXTERN struct tbx_fast_list_head ma_ev_list_poll;


/** Internal functions **/
static __tbx_inline__ int marcel_polling_is_required(unsigned polling_point)
TBX_UNUSED;
static __tbx_inline__ void marcel_check_polling(unsigned polling_point)
TBX_UNUSED;
void TBX_EXTERN __marcel_check_polling(unsigned polling_point);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_POLLING_H__ **/
