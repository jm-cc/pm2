
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

/****************************************************************
 * Certain commentaires sont �tiquet�s par U/S/C/I signifiant
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
        ((type *)((char *)(typeof (&((type *)0)->member))(ptr)- \
		  (unsigned long)(&((type *)0)->member)))



#section types

/*[U]****************************************************************
 * Les types abstaits pour le serveur et les �v�nements
 */
typedef struct marcel_ev_server *marcel_ev_serverid_t;
typedef struct marcel_ev *marcel_ev_inst_t;

#section macros
/*[S]****************************************************************
 * Initialisation d'un serveur
 */

/* Initialisation statique
 * var : la variable constante marcel_ev_serverid_t
 * name: une chaine de caract�re pour identifier ce serveur dans les
 *        messages de debug
 */
#define MARCEL_EV_SERVER_DEFINE(var, name) \
  struct marcel_ev_server ma_s_#var = MARCEL_EV_SERVER_INIT(ma_s_#var, name); \
  const MARCEL_EV_SERVER_DECLARE(var) = &ma_s_#var

#section functions
/* Idem, mais dynamique */
inline static void marcel_ev_server_init(marcel_ev_serverid_t id, char* name);

/* Enregistrement des call-backs utilisables */
inline static int marcel_ev_server_add_callback(marcel_ev_serverid_t id, 
						   marcel_ev_op_t op,
						   marcel_ev_func_t func);
/* R�glage des param�tres de scrutation */
int marcel_ev_server_set_poll_settings(marcel_ev_serverid_t id, 
				       unsigned poll_points,
				       unsigned frequency);

/* D�marrage du serveur
 * Il devient possible d'attendre des �v�nements
 */
int marcel_ev_server_start(marcel_ev_serverid_t id);

/*[U]****************************************************************
 * Fonctions � l'usage des threads applicatifs
 */

#section types
/* Attribut pouvant �tre attach� aux �v�nements */
enum {
	/* D�senregistre l'�v�nement lors de sa prochaine occurrence */
	MARCEL_EV_ATTR_ONE_SHOT=1,
	/* Ne r�veille pas les waiter du serveur */
	MARCEL_EV_ATTR_NO_WAKE_SERVER=2,
};

#section functions
/* Initialisation, enregistrement, attente et d�senregistrement d'un �v�nement */
int marcel_ev_wait_one(marcel_ev_serverid_t id, marcel_ev_inst_t ev, marcel_time_t timeout);

/* Initialisation d'un �v�nement
 * (� appeler en premier si on utilise autre chose que wait_one) 
 */
int marcel_ev_init(marcel_ev_serverid_t id, marcel_ev_inst_t ev);

/* Ajout d'un attribut sp�cifique � un �v�nement */
int marcel_ev_attr_set(marcel_ev_inst_t ev, int attr);

/* Enregistrement d'un �v�nement */
int marcel_ev_register(marcel_ev_serverid_t id, marcel_ev_inst_t ev);

/* Abandon d'un �v�nement et retour des threads en attente sur cet �v�nement */
int marcel_ev_unregister(marcel_ev_serverid_t id, marcel_ev_inst_t ev);

/* Attente d'un thread sur un �v�nement d�j� enregistr� */
int marcel_ev_wait_ev(marcel_ev_serverid_t id, marcel_ev_inst_t ev, marcel_time_t timeout);

/* Attente d'un thread sur un quelconque �v�nement du serveur */
int marcel_ev_wait_server(marcel_ev_serverid_t id, marcel_time_t timeout);

/* Renvoie un �v�nement survenu (utile au retour de wait_server)
   Au d�senregistrement d'un �v�nement (wait_one, unregister ou ONE_SHOT)
   l'�v�nement est �galement retir� de cette file (donc n'est plus consultable)
*/
marcel_ev_inst_t marcel_ev_get_success(marcel_ev_serverid_t id);

/* Exclusion mutuelle pour un serveur d'�v�nements
 *
 * - les call-backs sont TOUJOURS appel�s � l'int�rieur de cette
 *   exclusion mutuelle.
 * - les call-backs BLOCK_ONE|ALL doivent rel�cher puis reprendre ce
 *   lock avant et apr�s l'appel syst�me bloquant avec les deux fonctions
 *   suivantes.
 * - les fonctions pr�c�dentes peuvent �tre appel�es avec ou sans ce
 *   lock
 * - le lock est rel�ch� automatiquement par les fonctions d'attente
 *   (ev_wait_*)

 * - les call-backs et les r�veils des threads en attente sur les
 *   �v�nements signal�s par le call-back sont atomiques (vis � vis de 
 *   ce lock)
 * - si le lock est pris avant les fonctions d'attente (ev_wait_*),
 *   la mise en attente est atomique vis � vis des call-backs
 *   ie: un call-back signalant l'�v�nement attendu r�veillera cette
 *   attente
 * - si un �v�nement � la propri�t� ONE_SHOT, le d�senregistrement est
 *   atomic vis � vis du call-back qui a g�n�r� l'�v�nement.
 */
int marcel_ev_lock(marcel_ev_serverid_t id);
int marcel_ev_unlock(marcel_ev_serverid_t id);
/* Pour BLOCK_ONE et BLOCK_ALL avant et apr�s l'appel syst�me bloquant */
int marcel_ev_callback_will_block(marcel_ev_serverid_t id);
int marcel_ev_callback_has_blocked(marcel_ev_serverid_t id);

/* Appel forc� de la fonction de scrutation */
void marcel_ev_poll_force(marcel_ev_serverid_t id);
/* Idem, mais on est s�r que l'appel a �t� fait quand on retourne */
void marcel_ev_poll_force_sync(marcel_ev_serverid_t id);

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
	/* id, XX, ev to poll, nb_ev_grouped, flags */
	MARCEL_EV_FUNCTYPE_POLL_ONE,
	/* id, XX, NA, nb_ev_grouped, NA */
	MARCEL_EV_FUNCTYPE_POLL_GROUP,
	/* id, XX, NA, nb_ev_grouped, NA */
	MARCEL_EV_FUNCTYPE_POLL_ALL,
	/* Les suivants ne sont pas encore utilis�s... */
	/* id, XX, ev to block, NA, NA */
	MARCEL_EV_FUNCTYPE_BLOCK_ONE,
	MARCEL_EV_FUNCTYPE_BLOCK_ONE_TIMEOUT,
	MARCEL_EV_FUNCTYPE_BLOCK_GROUP,
	MARCEL_EV_FUNCTYPE_BLOCK_ALL,
	MARCEL_EV_FUNCTYPE_BLOCK_ALL_TIMEOUT,
	MARCEL_EV_FUNCTYPE_UNBLOCK_ONE,
	MARCEL_EV_FUNCTYPE_UNBLOCK_ALL,
	/* PRIVATE */
	MA_EV_FUNCTYPE_SIZE
} marcel_ev_op_t;

/*[C]****************************************************************
 * Le prototype des call-back
 * id : le serveur
 * op : l'op�ration (call-back) demand�
 * ev : pour *_ONE : l'�v�nement � tester en particulier
 * nb_ev : pour POLL_* : le nombre d'�v�nements actuellement group�s
 * option : flags d�pendant de l'op�ration
 *  - pour POLL_ONE : 
 *     + EV_IS_GROUPED : si ev est d�j� group�
 *     + EV_ITER : si POLL_ONE est appel� sur tous les ev en attente
 *                 (ie POLL_ALL n'est pas disponible)
 *
 * La valeur de retour est pour l'instant ignor�e.
 */
typedef int (*marcel_ev_func_t)(marcel_ev_serverid_t id, 
				marcel_ev_op_t op,
				marcel_ev_inst_t ev, 
				int nb_ev, int option);

/*[C]****************************************************************
 * Les flags des call-backs (voir ci-dessus)
 */
enum {
	/* Pour POLL_ONE */
	MARCEL_EV_OPT_EV_IS_GROUPED=1,
	MARCEL_EV_OPT_EV_ITER=2,
};

#section macros
/*[C]****************************************************************
 * It�rateurs pour les call-backs
 */

/* It�rateur avec le type de base
   marcel_ev_inst_t ev : iterateur
   marcel_ev_serverid_t ps : serveur id
*/
#define FOREACH_EV_POLL_BASE(ev, ps) \
  list_for_each_entry((ev), &(ps)->list_ev_poll_grouped, chain_ev)

/* It�rateur avec un type utilisateur
   [User Type] ev : pointeur sur structure contenant un
                    struct marcel_ev (it�rateur)
   marcel_ev_serverid_t ps : serveur id
   member : nom de struct marcel_ev dans la structure point�e par ev (ou tmp)
*/
#define FOREACH_EV_POLL(ev, ps, member) \
  list_for_each_entry((ev), &(ps)->list_ev_poll_grouped, member.chain_ev)

/* Macros utilisable dans les call-backs pour indiquer qu'un �v�nement 
   est survenu
   marcel_ev_serverid_t id : serveur id
   marcel_ev_inst_t ev : �v�nement
*/
#define MARCEL_EV_POLL_SUCCESS(id, ev) \
  do { \
        list_del(&(ev)->chain_ev_ready); \
        list_add(&(ev)->chain_ev_ready, &(id)->list_ev_ready); \
  } while(0)
 

// =============== PRIVATE ===============
/****************************************************************
 * Ce qui suit n'a pas vocation � �tre utilis� hors de Marcel
 */

#section structures
#depend "[marcel_structures]"

#section inline
inline static void marcel_ev_server_init(marcel_ev_serverid_t id, char* name)
{
	*id=(struct marcel_ev_server)MARCEL_EV_SERVER_INIT(*id, name);
}

inline static int marcel_ev_server_add_callback(marcel_ev_serverid_t id, 
						   marcel_ev_op_t op,
						   marcel_ev_func_t func)
{
#ifdef MA__DEBUG
	/* Cette fonction doit �tre appel�e entre l'initialisation et
	 * le d�marrage de ce serveur d'�v�nements */
	MA_BUG_ON(id->state != MA_EV_SERVER_STATE_INIT);
	/* On v�rifie que l'index est correct */
	MA_BUG_ON(op>=MA_EV_FUNCTYPE_SIZE || op<0);
	/* On v�rifie que la fonction n'est pas d�j� l� */
	MA_BUG_ON(id->funcs[op]!=NULL);
#endif
	id->funcs[op]=func;
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
	/* Liste des �v�nements de polling group�s (ou en cours de groupage) */
	struct list_head list_ev_poll_grouped;
	/* Nombre d'�v�nements dans la liste pr�c�dente */
	int ev_poll_grouped_nb;
	/* Liste des �v�nements signal�s pr�ts par les call-backs */
	struct list_head list_ev_ready;
	/* Liste des attentes en cours sur les �v�nements */
	struct list_head list_id_waiters;
	/* Liste des �v�nements pour ceux en attente dans la liste pr�c�dente */
	struct list_head list_ev_success;
	/* Spinlock r�gissant l'acc�s � la liste pr�c�dente */
	ma_spinlock_t ev_success_lock;
	/* Polling events registered but not yet polled */
	int registered_ev_not_yet_polled;

	/* Call-backs */
	marcel_ev_func_t funcs[MA_EV_FUNCTYPE_SIZE];
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
  const marcel_ev_serverid_t var
#define MARCEL_EV_SERVER_INIT(var, sname) \
  { \
    .lock=MA_SPIN_LOCK_UNLOCKED, \
    .lock_owner=NULL, \
    .list_ev_poll_grouped=LIST_HEAD_INIT((var).list_ev_poll_grouped), \
    .ev_poll_grouped_nb=0, \
    .list_ev_ready=LIST_HEAD_INIT((var).list_ev_ready), \
    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters), \
    .list_ev_success=LIST_HEAD_INIT((var).list_ev_success), \
    .ev_success_lock=MA_SPIN_LOCK_UNLOCKED, \
    .registered_ev_not_yet_polled=0, \
    .funcs={NULL, }, \
    .poll_points=0, \
    .frequency=0, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &marcel_poll_from_tasklet, \
                     (unsigned long)(marcel_ev_serverid_t)&(var) ), \
    .poll_timer= MA_TIMER_INITIALIZER(marcel_poll_timer, 0, \
                   (unsigned long)(marcel_ev_serverid_t)&(var)), \
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
struct marcel_ev {
	/* Chaine des �v�nements group� en attente */
	struct list_head chain_ev;
	/* Chaine des �v�nements signal�s OK par un call-back */
	struct list_head chain_ev_ready;
	/* Chaine des �v�nements � signaler au serveur */
	struct list_head chain_success;
	/* �tat */
	int state;
	/* Liste des attentes en cours sur cet �v�nement */
	struct list_head list_ev_waiters;
};

#section marcel_variables
/* Liste des serveurs en cours de polling 
 * (state == 2 et t�ches en attente de polling) 
 */
extern struct list_head ma_ev_list_poll;

#section marcel_functions
static __inline__ int marcel_polling_is_required(unsigned polling_point)
__attribute__ ((unused));
static __inline__ void marcel_check_polling(unsigned polling_point)
__attribute__ ((unused));
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
	struct marcel_ev inst;
	marcel_ev_serverid_t id;
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
#define MARCEL_POLL_SUCCESS(id)           ({MARCEL_EV_POLL_SUCCESS(&(id)->server, &(id)->cur_cell->inst); (void*)1;})
#define MARCEL_POLL_SUCCESS_FOR(pollinst) ({MARCEL_EV_POLL_SUCCESS((pollinst)->id, &(pollinst)->inst); (void*)1;})


// ATTENTION : changement d'interface
// Remplacer : "FOREACH_POLL(id, _arg) { ..."
// par : "FOREACH_POLL(id) { GET_ARG(id, _arg); ..."
// Ou mieux: utiliser FOREACH_EV_POLL[_BASE]
#define FOREACH_POLL(id) \
  FOREACH_EV_POLL((id)->cur_cell, &(id)->server, inst)
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

static __inline__ void * __tbx_deprecated__ 
marcel_pollid_getspecific(marcel_pollid_t id)
{
	return id->specific;
}

