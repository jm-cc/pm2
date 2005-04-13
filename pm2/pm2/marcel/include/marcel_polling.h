
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

// THESE: INTERFACE START
/****************************************************************
 * Certains commentaires sont �tiquet�s par U/S/C/I signifiant
 * User       : utile aux threads applicatifs pour exploiter ces m�canismes
 * Start      : utile pour l'initialisation
 * Call-backs : utile pour le concepteur de call-backs
 * Internal   : ne devrait pas �tre utilis� hors de Marcel
 *
 * Ce ne sont que des indications, pas des contraintes
 ****************************************************************/

#section macros
/*[SC]****************************************************************
 * R�cup�re l'adresse d'une structure englobante 
 * Il faut donner :
 * - l'adresse du champ interne
 * - le type de la structure englobante
 * - le nom du champ interne dans la structure englobante
 */
#define struct_up(ptr, type, member) \
	tbx_container_of(ptr, type, member)

#section types

/*[U]****************************************************************
 * Les types abstaits pour le serveur et les �v�nements
 */
/* Le serveur d�finit les call-backs et les param�tres de scrutation �
 * utiliser. Il va �ventuellement regrouper les ressources
 * enregistr�es (en cas de scrutation active par exemple)
 */
typedef struct marcel_ev_server *marcel_ev_server_t;

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

#section macros
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

#section functions
/* Idem, mais dynamique */
#ifndef __cplusplus
inline static void marcel_ev_server_init(marcel_ev_server_t server, char* name);
#endif

/* Enregistrement des call-backs utilisables */
inline static int marcel_ev_server_add_callback(marcel_ev_server_t server, 
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

// THESE: PROTO_EV_WAIT START
/*[U]****************************************************************
 * Fonctions � l'usage des threads applicatifs
 */

#section types
/* Attribut pouvant �tre attach� aux �v�nements */
enum {
	/* D�sactive la requ�te lorsque survient l'occurrence
	 * suivante de l'�v�nement */
	MARCEL_EV_ATTR_ONE_SHOT=1,
	/* Ne r�veille pas les threads en attente d'�v�nements du
	 * serveur (ie marcel_ev_server_wait())*/
	MARCEL_EV_ATTR_NO_WAKE_SERVER=2,
};

#section functions
/* Un raccourci pratique des fonctions suivantes, utile si l'on ne
 * soumet la requ�te qu'une seule fois. Les op�rations suivantes sont
 * effectu�es : initialisation, soumission et attente d'un
 * �v�nement avec ONE_SHOT positionn� */
int marcel_ev_wait(marcel_ev_server_t server, marcel_ev_req_t req,
		   marcel_ev_wait_t wait, marcel_time_t timeout);

/* Initialisation d'un �v�nement
 * (� appeler en premier si l'on utilise autre chose que marcel_ev_wait) */
int marcel_ev_req_init(marcel_ev_req_t req);

/* Ajout d'un attribut sp�cifique � une requ�te */
int marcel_ev_req_attr_set(marcel_ev_req_t req, int attr);

/* Soumission d'une requ�te (le serveur PEUT commencer � scruter si cela
 * lui convient) */
int marcel_ev_req_submit(marcel_ev_server_t server, marcel_ev_req_t req);

/* Abandon d'une requ�te et retour des threads en attente sur cette
 * requ�te (avec le code de retour fourni) */
int marcel_ev_req_cancel(marcel_ev_req_t req, int ret_code);

/* Attente bloquante d'un �v�nement sur une requ�te d�j� enregistr�e */
int marcel_ev_req_wait(marcel_ev_req_t req, marcel_ev_wait_t wait, 
		       marcel_time_t timeout);

/* Attente bloquante d'un �v�nement sur une quelconque requ�te du serveur */
int marcel_ev_server_wait(marcel_ev_server_t server, marcel_time_t timeout);

/* Renvoie une requ�te survenue n'ayant pas l'attribut NO_WAKE_SERVER
 * (utile au retour de server_wait())
 * � l'abandon d'une requ�te (wait, req_cancel ou ONE_SHOT) la requ�te
 * est �galement retir�e de cette file (donc n'est plus consultable) */
marcel_ev_req_t marcel_ev_get_success_req(marcel_ev_server_t server);

// THESE: PROTO_EV_WAIT STOP
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
/* Pour BLOCK_ONE et BLOCK_ALL avant et apr�s l'appel syst�me bloquant */
int marcel_ev_callback_will_block(marcel_ev_server_t server);
int marcel_ev_callback_has_blocked(marcel_ev_server_t server);

/* Appel forc� de la fonction de scrutation */
void marcel_ev_poll_force(marcel_ev_server_t server);
/* Idem, mais on est s�r que l'appel a �t� fait quand on retourne */
void marcel_ev_poll_force_sync(marcel_ev_server_t server);

#section macros
/*[S]****************************************************************
 * Les constantes pour le polling
 * Elles peuvent �tre ORed
 */
#define MARCEL_EV_POLL_AT_TIMER_SIG  1
#define MARCEL_EV_POLL_AT_YIELD      2
#define MARCEL_EV_POLL_AT_LIB_ENTRY  4
#define MARCEL_EV_POLL_AT_IDLE       8

#section types
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
	/* Les suivants ne sont pas encore utilis�s... */
	/* server, XX, req to block, NA, NA */
	MARCEL_EV_FUNCTYPE_BLOCK_WAITONE,
	MARCEL_EV_FUNCTYPE_BLOCK_WAITONE_TIMEOUT,
	MARCEL_EV_FUNCTYPE_BLOCK_GROUP,
	MARCEL_EV_FUNCTYPE_BLOCK_WAITANY,
	MARCEL_EV_FUNCTYPE_BLOCK_WAITANY_TIMEOUT,
	MARCEL_EV_FUNCTYPE_UNBLOCK_WAITONE,
	MARCEL_EV_FUNCTYPE_UNBLOCK_WAITANY,
	/* PRIVATE */
	MA_EV_FUNCTYPE_SIZE
} marcel_ev_op_t;

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
typedef int (marcel_ev_callback_t)(marcel_ev_server_t server, 
				   marcel_ev_op_t op,
				   marcel_ev_req_t req, 
				   int nb_ev, int option);

typedef marcel_ev_callback_t *marcel_ev_pcallback_t;

/*[C]****************************************************************
 * Les flags des call-backs (voir ci-dessus)
 */
enum {
	/* Pour POLL_POLLONE */
	MARCEL_EV_OPT_REQ_IS_GROUPED=1,
	MARCEL_EV_OPT_REQ_ITER=2,
};

#section macros
/*[C]****************************************************************
 * It�rateurs pour les call-backs
 */

/****************************************************************
 * It�rateur pour les requ�tes enregistr�es d'un serveur
 */

/* It�rateur avec le type de base
   marcel_ev_req_t req : it�rateur
   marcel_ev_server_t server : serveur
*/
#define FOREACH_REQ_REGISTERED_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_registered, chain_req_registered)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_REGISTERED_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_registered, chain_req_registered)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (it�rateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure point�e par req
*/
#define FOREACH_REQ_REGISTERED(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_registered, member.chain_req_registered)

/****************************************************************
 * It�rateur pour les requ�tes group�es de la scrutation (polling)
 */

/* It�rateur avec le type de base
   marcel_ev_req_t req : it�rateur
   marcel_ev_server_t server : serveur
*/
#define FOREACH_REQ_POLL_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (it�rateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure point�e par req
*/
#define FOREACH_REQ_POLL(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)


/****************************************************************
 * It�rateur pour les requ�tes group�es de l'attente bloquante
 */

/* It�rateur avec le type de base
   marcel_ev_req_t req : it�rateur
   marcel_ev_server_t server : serveur
*/
#define FOREACH_REQ_BLOCK_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_block, chain_req)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_REQ_BLOCK_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_block, chain_req)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (it�rateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure point�e par req
*/
#define FOREACH_REQ_BLOCK(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_block, member.chain_req)


/****************************************************************
 * It�rateur pour les attentes d'�v�nements rattach�s � une requ�te
 */

/* It�rateur avec le type de base
   marcel_ev_wait_t wait : it�rateur
   marcel_ev_req_t req : requ�te
*/
#define FOREACH_WAIT_BASE(wait, req) \
  list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem mais prot�g� (usage interne) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req) \
  list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* It�rateur avec un type utilisateur
   [User Type] req : pointeur sur structure contenant un struct marcel_req
                     (it�rateur)
   marcel_ev_server_t server : serveur
   member : nom de struct marcel_ev dans la structure point�e par req
*/
#define FOREACH_WAIT(wait, req, member) \
  list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)



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
        list_del(&(req)->chain_req_ready); \
        list_add(&(req)->chain_req_ready, &(req)->server->list_req_ready); \
  } while(0)
 


/* Macro utilisable dans les call-backs pour r�veiller un thread en attente
   en lui fournissant le code de retour
   marcel_ev_wait_t wait : l'�v�nement � reveiller
*/
#define MARCEL_EV_WAIT_SUCCESS(wait, code) \
  do { \
        list_del(&(wait)->chain_wait); \
        (wait)->ret_code=(code); \
  } while(0)
 
// THESE: INTERFACE STOP


// =============== PRIVATE ===============
/****************************************************************
 * Ce qui suit n'a pas vocation � �tre utilis� hors de Marcel
 */

#section structures
#depend "[marcel_structures]"

#section inline
#ifndef __cplusplus
inline static void marcel_ev_server_init(marcel_ev_server_t server, char* name)
{
	*server=(struct marcel_ev_server)MARCEL_EV_SERVER_INIT(*server, name);
}
#endif

inline static int marcel_ev_server_add_callback(marcel_ev_server_t server, 
						marcel_ev_op_t op,
						marcel_ev_pcallback_t func)
{
#ifdef MA__DEBUG
	/* Cette fonction doit �tre appel�e entre l'initialisation et
	 * le d�marrage de ce serveur d'�v�nements */
	MA_BUG_ON(server->state != MA_EV_SERVER_STATE_INIT);
	/* On v�rifie que l'index est correct */
	MA_BUG_ON(op>=MA_EV_FUNCTYPE_SIZE || op<0);
	/* On v�rifie que la fonction n'est pas d�j� l� */
	MA_BUG_ON(server->funcs[op]!=NULL);
#endif
	server->funcs[op]=func;
	return 0;
}

#section functions
/* Utilis� dans les initialiseurs */
void marcel_poll_from_tasklet(unsigned long id);
void marcel_poll_timer(unsigned long id);

#section marcel_structures
#depend "pm2_list.h"
#depend "marcel_sem.h[]"
#depend "marcel_descr.h[types]"
#depend "linux_interrupt.h[]"
#depend "linux_timer.h[]"

#section marcel_types
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
#section marcel_structures
struct marcel_ev_server {
	/* Spinlock r�gissant l'acc�s � cette structure */
	ma_spinlock_t lock;
	/* Thread propri�taire du lock (pour le locking applicatif) */
	marcel_task_t *lock_owner;
	/* Liste des requ�tes soumises */
	struct list_head list_req_registered;
	/* Liste des requ�tes signal�es pr�tes par les call-backs */
	struct list_head list_req_ready;
	/* Liste des requ�tes pour ceux en attente dans la liste pr�c�dente */
	struct list_head list_req_success;
	/* Liste des attentes en cours sur les �v�nements */
	struct list_head list_id_waiters;
	/* Spinlock r�gissant l'acc�s � la liste pr�c�dente */
	ma_spinlock_t req_success_lock;
	/* Polling events registered but not yet polled */
	int registered_req_not_yet_polled;

	/* Liste des requ�tes group�es pour le polling (ou en cours de groupage) */
	struct list_head list_req_poll_grouped;
	/* Nombre de requ�tes dans la liste pr�c�dente */
	int req_poll_grouped_nb;

	/* Call-backs */
	marcel_ev_pcallback_t funcs[MA_EV_FUNCTYPE_SIZE];
	/* Points et fr�quence de scrutation */
	unsigned poll_points;
	unsigned frequency;
	/* Chaine des serveurs en cours de polling 
	   (state == 2 et t�ches en attente de polling) */
	struct list_head chain_poll;
	/* Tasklet et timer utilis�e pour la scrutation */
	struct ma_tasklet_struct poll_tasklet;
	struct ma_timer_list poll_timer;
	/* �tat du serveur */
	int state;
	/* Nom (utilis� pour le d�bug) */
	char* name;
};

#section macros
#define MARCEL_EV_SERVER_DECLARE(var) \
  const marcel_ev_server_t var
#define MARCEL_EV_SERVER_INIT(var, sname) \
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
    .funcs={NULL, }, \
    .poll_points=0, \
    .frequency=0, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &marcel_poll_from_tasklet, \
                     (unsigned long)(marcel_ev_server_t)&(var) ), \
    .poll_timer= MA_TIMER_INITIALIZER(marcel_poll_timer, 0, \
                   (unsigned long)(marcel_ev_server_t)&(var)), \
    .state=MA_EV_SERVER_STATE_INIT, \
    .name=sname, \
  }


#section marcel_types
enum {
	MARCEL_EV_STATE_OCCURED=1,
	MARCEL_EV_STATE_GROUPED=2,
	MARCEL_EV_STATE_ONE_SHOT=4,
	MARCEL_EV_STATE_NO_WAKE_SERVER=8,
	MARCEL_EV_STATE_REGISTERED=16,
};

#section marcel_structures
struct marcel_ev_req {
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
	marcel_ev_server_t server;
	/* Liste des attentes en cours sur cette requ�te */
	struct list_head list_wait;
};

struct marcel_ev_wait {
	/* Chaine des �v�nements group� en attente */
	struct list_head chain_wait;

	marcel_sem_t sem;
	/* 0: event
	   -ECANCELED: marcel_unregister called
	*/
	int ret;
	marcel_task_t *task;
};


#section marcel_variables
#include "tbx_compiler.h"
/* Liste des serveurs en cours de polling 
 * (state == 2 et t�ches en attente de polling) 
 */
extern struct list_head ma_ev_list_poll;

#section marcel_functions
static __inline__ int marcel_polling_is_required(unsigned polling_point)
TBX_UNUSED;
static __inline__ void marcel_check_polling(unsigned polling_point)
TBX_UNUSED;
#section marcel_inline
void __marcel_check_polling(unsigned polling_point);

static __inline__ int marcel_polling_is_required(unsigned polling_point)
{
	return !list_empty(&ma_ev_list_poll);
}

static __inline__ void marcel_check_polling(unsigned polling_point)
{
	if(marcel_polling_is_required(polling_point))
		__marcel_check_polling(polling_point);
}

/****************************************************************
 * Compatibility stuff
 *
 * Software using this should switch to the new interface
 * This will be remove later
 */

#section types

struct poll_struct;
struct poll_cell;

typedef struct poll_struct *marcel_pollid_t;
typedef struct poll_cell poll_cell_t, *marcel_pollinst_t;

#section marcel_structures
struct poll_struct {
	struct marcel_ev_server server;
	marcel_poll_func_t func;
	marcel_fastpoll_func_t fastfunc;
	marcel_pollgroup_func_t gfunc;
	void *specific;
	marcel_pollinst_t cur_cell;
};

struct poll_cell {
	struct marcel_ev_req inst;
	any_t arg;
};

#section types
typedef void (*marcel_pollgroup_func_t)(marcel_pollid_t id);

typedef void *(*marcel_poll_func_t)(marcel_pollid_t id,
				    unsigned active,
				    unsigned sleeping,
				    unsigned blocked);

typedef void *(*marcel_fastpoll_func_t)(marcel_pollid_t id,
					any_t arg,
					boolean first_call);

#section macros
#define MARCEL_POLL_AT_TIMER_SIG  MARCEL_EV_POLL_AT_TIMER_SIG
#define MARCEL_POLL_AT_YIELD      MARCEL_EV_POLL_AT_YIELD
#define MARCEL_POLL_AT_LIB_ENTRY  MARCEL_EV_POLL_AT_LIB_ENTRY
#define MARCEL_POLL_AT_IDLE       MARCEL_EV_POLL_AT_IDLE

#section functions
marcel_pollid_t __tbx_deprecated__
marcel_pollid_create_X(marcel_pollgroup_func_t g,
		       marcel_poll_func_t f,
		       marcel_fastpoll_func_t h,
		       unsigned polling_points,
		       char* name);
#define marcel_pollid_create(g, f, h, polling_points) \
  marcel_pollid_create_X(g, f, h, polling_points, \
                         "Auto created ("__BASE_FILE__")")

#section macros
#define MARCEL_POLL_FAILED                NULL
#define MARCEL_POLL_OK                    (void*)1
#define MARCEL_POLL_SUCCESS(id)           ({MARCEL_EV_REQ_SUCCESS(&(id)->cur_cell->inst); (void*)1;})
#define MARCEL_POLL_SUCCESS_FOR(pollinst) ({MARCEL_EV_REQ_SUCCESS(&(pollinst)->inst); (void*)1;})


// ATTENTION : changement d interface
// Remplacer : "FOREACH_POLL(id, _arg) { ..."
// par : "FOREACH_POLL(id) { GET_ARG(id, _arg); ..."
// Ou mieux: utiliser FOREACH_EV_POLL[_BASE]
#define FOREACH_POLL(id) \
  FOREACH_REQ_POLL((id)->cur_cell, &(id)->server, inst)
#define GET_ARG(id, _arg) \
	_arg = (typeof(_arg))((id)->cur_cell->arg)

#define GET_CURRENT_POLLINST(id) ((id)->cur_cell)

#section functions
void __tbx_deprecated__ marcel_poll(marcel_pollid_t id, any_t arg);

void __tbx_deprecated__ marcel_force_check_polling(marcel_pollid_t id);

static __inline__ void __tbx_deprecated__ 
marcel_pollid_setspecific(marcel_pollid_t id, void *specific)
{
	id->specific = specific;
}

void marcel_poll_lock(void);
void marcel_poll_unlock(void);

static __inline__ void * __tbx_deprecated__ 
marcel_pollid_getspecific(marcel_pollid_t id)
{
	return id->specific;
}

