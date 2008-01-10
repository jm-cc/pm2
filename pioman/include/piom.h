
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

#ifndef PIOM_EV_SERV
#define PIOM_EV_SERV
#include "pm2_list.h"

#ifdef MARCEL
#include "marcel.h"
#endif


/********************************************************************
 *
 * T Y P E S
 *
 ********************************************************************/


/* Le serveur définit les call-backs et les paramètres de scrutation à
 * utiliser. Il va éventuellement regrouper les ressources
 * enregistrées (en cas de scrutation active par exemple)
 */
typedef struct piom_server *piom_server_t;

/* Une requête définit une entité qui pourra recevoir un événement que
 * l'on pourra attendre.  Diverses requêtes d'un serveur pourront être
 * groupées (afin de déterminer plus rapidement l'ensemble des états
 * de chacune des requêtes à chaque scrutation)
 */
typedef struct piom_req *piom_req_t;

/* Un thread peut attendre l'arrivée effective d'un événement
 * correspondant à une requête. Le thread est déschedulé tant que la
 * requête n'est pas prête (que l'événement n'est pas reçu).
 */
typedef struct piom_wait *piom_wait_t;

#ifdef MA__LWPS
/* LWP spécialisé dans les communications */
typedef struct piom_comm_lwp *piom_comm_lwp_t;
#endif /* MA__LWPS */

#ifndef MARCEL
/* Without Marcel compliance */
#define piom_time_t int*
#else
#define piom_time_t marcel_time_t
#endif				// MARCEL

/* Available kinds of callbacks */
typedef enum {
	/* the callback polls a specific query */
	PIOM_FUNCTYPE_POLL_POLLONE,
	/* the callback groups queries */
	PIOM_FUNCTYPE_POLL_GROUP,
	/* the callback polls every submitted query */
	PIOM_FUNCTYPE_POLL_POLLANY,
	/* the callback exports a syscall to complete a specific query */
	PIOM_FUNCTYPE_BLOCK_WAITONE,
	/* idem with a timeout */
	PIOM_FUNCTYPE_BLOCK_WAITONE_TIMEOUT,
	/* the callback groups queries to be exported */
	PIOM_FUNCTYPE_BLOCK_GROUP,
	/* the callback completes every submitted query using an exported syscall */
	PIOM_FUNCTYPE_BLOCK_WAITANY,
	/* idem with a timeout */
	PIOM_FUNCTYPE_BLOCK_WAITANY_TIMEOUT,
	/* unblocks a query blocked with PIOM_FUNCTYPE_BLOCK_WAITONE */
	PIOM_FUNCTYPE_UNBLOCK_WAITONE,
	/* unblocks a query blocked with PIOM_FUNCTYPE_BLOCK_WAITANY */
	PIOM_FUNCTYPE_UNBLOCK_WAITANY,
	/* PRIVATE */
	PIOM_FUNCTYPE_SIZE
} piom_op_t;


/* Callback's speed */
typedef enum {
	PIOM_CALLBACK_SLOWEST = 1,	// >10 µs
	PIOM_CALLBACK_SLOW = 2,	// several µs
	PIOM_CALLBACK_NORMAL_SPEED = 3,	// 1 µs
	PIOM_CALLBACK_FAST = 4,	// 1/2 µs
	PIOM_CALLBACK_IMMEDIATE = 5,	// immediate
} piom_callback_speed_t;


/* State of the events server */
enum {
	/* Not yet initialized */
	PIOM_SERVER_STATE_NONE,
	/* Being initialized */
	PIOM_SERVER_STATE_INIT = 1,
	/* Running */
	PIOM_SERVER_STATE_LAUNCHED = 2,
	/* Stopped */
	PIOM_SERVER_STATE_HALTED = 3,
};

/* Polling constants. Defines the polling points */
#define PIOM_POLL_AT_TIMER_SIG  1
#define PIOM_POLL_AT_YIELD      2
#define PIOM_POLL_AT_LIB_ENTRY  4
#define PIOM_POLL_AT_IDLE       8
#define PIOM_POLL_WHEN_FORCED   16

/* Query options */
enum {
	/* used with POLL_POLLONE */
	PIOM_OPT_REQ_IS_GROUPED = 1,	/* is the query grouped ? */
	PIOM_OPT_REQ_ITER = 2,	/* is the current polling done for every query ? */
};

/* Query state */
enum {
	PIOM_STATE_OCCURED = 1,
	PIOM_STATE_GROUPED = 2,
	PIOM_STATE_ONE_SHOT = 4,
	PIOM_STATE_NO_WAKE_SERVER = 8,
	PIOM_STATE_REGISTERED = 16,
	PIOM_STATE_EXPORTED = 32,	/* the query has been exported on another LWP */
	PIOM_STATE_DONT_POLL_FIRST = 64,	/* Don't poll when the query is posted (start polling later) */
};

/* Method to use to complete a query */
typedef enum {
	/* Choose automaticaly the best method */
	PIOM_FUNC_AUTO = 1,
	/* Use the polling fonction (ie, the query will probably be completed soon) */
	PIOM_FUNC_POLLING = 2,
	/* Use the syscall method (ie, the query will probably be completed later) */
	PIOM_FUNC_SYSCALL = 3,
} piom_func_to_use_t;

/* Query priority (not yet implemented) */
typedef enum {
	PIOM_REQ_PRIORITY_LOWEST = 0,
	PIOM_REQ_PRIORITY_LOW = 1,
	PIOM_REQ_PRIORITY_NORMAL = 2,
	PIOM_REQ_PRIORITY_HIGH = 3,
	PIOM_REQ_PRIORITY_HIGHEST = 4,
} piom_req_priority_t;

/* Events attributes */
enum {
	/* Disactivate the query next time the event occurs */
	PIOM_ATTR_ONE_SHOT = 1,
	/* Don't wake up the threads waiting for the server (ie piom_server_wait() ) */
	PIOM_ATTR_NO_WAKE_SERVER = 2,
	/* Don't poll when the query is posted (start polling later) */
	PIOM_ATTR_DONT_POLL_FIRST = 4,
};

/********************************************************************
 * Callbacks prototype
 * server : the server
 * op : the operation (callback) requested
 * req : only for *(POLL|WAIT)ONE* : the specific query to test
 * nb_req : only for POLL_* : the amount of grouped queries
 * option : flags depending on the operation
 *  - only for POLL_POLLONE : 
 *     + PIOM_IS_GROUPED : wether the query is already grouped
 *     + PIOM_ITER : if POLL_POLLONE is called for every query
 *                 (ie POLL_POLLANY isn't available)
 *
 * The ret value is ignored
 */
typedef int (piom_callback_t) (piom_server_t server,
				piom_op_t op,
				piom_req_t req, int nb_ev, int option);

typedef struct {
	piom_callback_t *func;
	piom_callback_speed_t speed;
} piom_pcallback_t;


#ifdef MA__LWPS
/* Communications specialized LWP */
struct piom_comm_lwp {
	int vp_nb; 		/* LWP number */
	int fds[2];		/* file descriptor used to unblock the lwp  */
	struct list_head chain_lwp_ready;
	struct list_head chain_lwp_working;
	piom_server_t server;
	marcel_t pid;
};
#endif /* MA__LWPS */

/* Events server structure */
struct piom_server {
#ifdef MARCEL
	/* Spinlock used to access this structure */
	/* TODO : useless without marcel ? */
	ma_spinlock_t lock;
	/* Thread that owns the lock */
	/* TODO useless without marcel ? */
	marcel_task_t *lock_owner;
#else
	/* foo */
	void *lock;
	void *lock_owner;
#endif				// MARCEL

	/* List of submitted queries */
	struct list_head list_req_registered;
	/* List of ready queries */
	struct list_head list_req_ready;
	/* Liste des requêtes pour ceux en attente dans la liste précédente */
	struct list_head list_req_success;
	/* List of waiters */
	struct list_head list_id_waiters;

#ifdef MA__LWPS
	/* Liste des requêtes à exporter */
	struct list_head list_req_to_export;
	/* Liste des requêtes exportées */
	struct list_head list_req_exported;
	/* List of ready LWPs */
	struct list_head list_lwp_ready;
	/* List of working LWPs */
	struct list_head list_lwp_working;

#endif /* MA__LWPS */

#ifdef MARCEL
	/* Spinlock used to access list_req_success */
	ma_spinlock_t req_success_lock;
	ma_spinlock_t req_ready_lock;
#else
	/* foo */
	void *req_success_lock;
#endif				// MARCEL
	/* Polling events registered but not yet polled */
	int registered_req_not_yet_polled;

	/* List of grouped queries for polling (or being grouped) */
	struct list_head list_req_poll_grouped;
	/* List of grouped queries for syscall (or being grouped) */
	struct list_head list_req_block_grouped;
	/* Amount of queries in list_req_poll_grouped */
	int req_poll_grouped_nb;
	int req_block_grouped_nb;

	/* Callbacks */
	piom_pcallback_t funcs[PIOM_FUNCTYPE_SIZE];

	/* Polling points and polling period */
	unsigned poll_points;
	unsigned period;	/* polls every 'period' timeslices */
	int stopable; 	       
	/* TODO: ? piom_chooser_t chooser; // a callback used to decide wether to export or not */
	/* Amount of polling loop before exporting a syscall 
	 * ( -1 : never export a syscall) */
	int max_poll;

	/* List of servers currently doing some polling
	   (state == 2 and tasks waiting for polling */
	struct list_head chain_poll;

#ifdef MARCEL
	/* Tasklet and timer used for polling */
	/* TODO : useless without marcel ?  */
	struct ma_tasklet_struct poll_tasklet;
//	struct ma_tasklet_struct manage_tasklet;
	int need_manage;
	struct ma_timer_list poll_timer;
#endif
        /* Max priority for successfull requests */
	/* ie : don't poll if you have a lower priority ! */
	piom_req_priority_t max_priority;
	/* server state */
	int state;
	/* server's name (for debuging) */
	char *name;
};


/* Query structure */
struct piom_req {
	/* Method to use for this query (syscall, polling, etc.) */
	piom_func_to_use_t func_to_use;
	/* Query priority */
	piom_req_priority_t priority;

	/* List of submitted queries */
	struct list_head chain_req_registered;
	/* List of grouped queries */
	struct list_head chain_req_grouped;
	/* List of grouped queries (for a blocking syscall)*/
	struct list_head chain_req_block_grouped;
	/* Chaine des requêtes signalées OK par un call-back */
	struct list_head chain_req_ready;
	/* List of completed queries */
	struct list_head chain_req_success;
	/* List of queries to be exported */
	struct list_head chain_req_to_export;
	/* List of exported queries*/
	struct list_head chain_req_exported;

	/* Query state */
	unsigned state;
	/* Query's server */
	piom_server_t server;
	/* List of waiters for this query */
	struct list_head list_wait;

	/* Amount of polling loops before the query is exported */
	/*  -1 : don't export this query */
	/*   0 : use the server's max_poll var */
	int max_poll;
	/* Amount of polling loops done */
	int nb_polled;
};

struct piom_wait {
	/* List of groupes events we are waiting for */
	struct list_head chain_wait;

#ifdef MARCEL
	/* TODO : Useless without marcel ? */
	marcel_sem_t sem;
	marcel_task_t *task;
#endif
	/* Ret value
	   0: event
	   -ECANCELED: piom_req_cancel called
	 */
	int ret;
};

#include "tbx_compiler.h"
/* List of servers currently doing some polling
 * (state == 2 and tasks waiting for polling) 
 */
extern struct list_head piom_list_poll;



/********************************************************************
 *  MACROS
 */

/*********************************************************************
 * Find the global structure's address
 * It needs :
 * - address of intern field
 * - type of the global structure
 * - name of the intern field of the global structure
 */
#define struct_up(ptr, type, member) \
	tbx_container_of(ptr, type, member)


/* Static initialisation
 * var : the constant variable piom_server_t
 * name: used to identify this server in the debug messages
 */
#define PIOM_SERVER_DEFINE(var, name) \
  struct piom_server ma_s_#var = PIOM_SERVER_INIT(ma_s_#var, name); \
  const PIOM_SERVER_DECLARE(var) = &ma_s_#var


/****************************************************************
 * Iterators over the submitted queries of a server
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_REGISTERED_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_registered, chain_req_registered)

/* Idem but safe (intern use) */
#define FOREACH_REQ_REGISTERED_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_registered, chain_req_registered)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_REGISTERED(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_registered, member.chain_req_registered)

/****************************************************************
 * Iterator over the grouped polling queries
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_POLL_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Idem but safe (intern use) */
#define FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_poll_grouped, chain_req_grouped)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_POLL(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_poll_grouped, member.chain_req_grouped)

/****************************************************************
 * Iterator over the grouped polling queries
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_BLOCKING_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_block_grouped,  chain_req_block_grouped)

/* Idem but safe (intern use) */
#define FOREACH_REQ_BLOCKING_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_block_grouped, chain_req_block_grouped)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_BLOCKING(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_block_grouped, member.chain_req_block_grouped)

#define FOREACH_REQ_BLOCKING_SAFE(req, tmp, server, member) \
  list_for_each_entry_safe((req),(tmp), &(server)->list_req_block_grouped, member.chain_req_block_grouped)

/****************************************************************
 * Iterator over the queries to be exported
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_TO_EXPORT_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_to_export, chain_req_to_export)

/* Idem but safe (intern use) */
#define FOREACH_REQ_TO_EXPORT_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_to_export, chain_req_to_export)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_TO_EXPORT(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_to_export, member.chain_req_to_export)

/****************************************************************
 * Iterator over the exported queries
 */

/* Base iterator
 *   piom_req_t req : iterator
 *   piom_server_t server : server
 */
#define FOREACH_REQ_EXPORTED_BASE(req, server) \
  list_for_each_entry((req), &(server)->list_req_exported, chain_req)

/* Idem but safe (intern use) */
#define FOREACH_REQ_EXPORTED_BASE_SAFE(req, tmp, server) \
  list_for_each_entry_safe((req), (tmp), &(server)->list_req_exported, chain_req)

/* Iterates using a custom type 
 *
 * [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
 */
#define FOREACH_REQ_EXPORTED(req, server, member) \
  list_for_each_entry((req), &(server)->list_req_exported, member.chain_req)


/****************************************************************
 * Iterator over the waiters attached to a query
 */

/* Base iterator
 *   piom_wait_t wait : iterator
 *   piom_req_t req : query
 */
#define FOREACH_WAIT_BASE(wait, req) \
  list_for_each_entry((wait), &(req)->list_wait, chain_wait)

/* Idem but safe (intern use) */
#define FOREACH_WAIT_BASE_SAFE(wait, tmp, req) \
  list_for_each_entry_safe((wait), (tmp), &(req)->list_wait, chain_wait)

/* Iterates using a custom type
 *
 *   [User Type] req : pointer to a structure contening a struct piom_req
 *                   (iterator)
 *   piom_server_t server : server
 *   member : name of struct piom in the structure pointed by req
*/
#define FOREACH_WAIT(wait, req, member) \
  list_for_each_entry((wait), &(req)->list_wait, member.chain_wait)

/* Macro used in callbacks to indicate that a query received an event.
 *
 *   * the query is transmited to _piom_get_success_req() (until it
 *   is desactivated)
 *   * threads waiting for an event attached to this query are waken up 
 *   and return 0.
 *
 *   piom_req_t req : query
*/
void piom_req_success(piom_req_t req);
/*
 \
  do { \
//	piom_spin_lock_softirq(req.server->req_ready_lock); \
        list_move(&(req)->chain_req_ready, &(req)->server->list_req_ready); \
//	piom_spin_unlock_softirq(req.server->req_ready_lock); \
  } while(0)
*/

/* Macro used in callbacks to wake up a waiting thread, giving it the ret value
 *   piom_wait_t wait : waiter to wake up
 */
#define PIOM_WAIT_SUCCESS(wait, code) \
  do { \
        list_del(&(wait)->chain_wait); \
        (wait)->ret_code=(code); \
  } while(0)


#define PIOM_SERVER_DECLARE(var) \
  const piom_server_t var

#ifdef MARCEL
#ifdef MA__LWPS
#define PIOM_SERVER_INIT(var, sname) \
  { \
    .lock=MA_SPIN_LOCK_UNLOCKED, \
    .lock_owner=NULL, \
    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered), \
    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready), \
    .list_req_success=LIST_HEAD_INIT((var).list_req_success), \
    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters), \
    .req_success_lock=MA_SPIN_LOCK_UNLOCKED, \
    .req_ready_lock=MA_SPIN_LOCK_UNLOCKED, \
    .registered_req_not_yet_polled=0, \
    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped), \
    .list_req_block_grouped=LIST_HEAD_INIT((var).list_req_block_grouped), \
    .list_req_to_export=LIST_HEAD_INIT((var).list_req_to_export), \
    .list_req_exported=LIST_HEAD_INIT((var).list_req_exported), \
    .list_lwp_ready=LIST_HEAD_INIT((var).list_lwp_ready), \
    .list_lwp_working=LIST_HEAD_INIT((var).list_lwp_working), \
    .req_poll_grouped_nb=0, \
    .poll_points=0, \
    .period=0, \
    .stopable=0, \
    .max_poll=-1, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &piom_poll_from_tasklet, \
                     (unsigned long)(piom_server_t)&(var) ), \
    .need_manage=0, \
    .poll_timer= MA_TIMER_INITIALIZER(piom_poll_timer, 0, \
                   (unsigned long)(piom_server_t)&(var)), \
    .state=PIOM_SERVER_STATE_INIT, \
    .name=sname, \
  }
#else  /* MA__LWPS */
#define PIOM_SERVER_INIT(var, sname) \
  { \
    .lock=MA_SPIN_LOCK_UNLOCKED, \
    .lock_owner=NULL, \
    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered), \
    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready), \
    .list_req_success=LIST_HEAD_INIT((var).list_req_success), \
    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters), \
    .req_success_lock=MA_SPIN_LOCK_UNLOCKED, \
    .req_ready_lock=MA_SPIN_LOCK_UNLOCKED, \
    .registered_req_not_yet_polled=0, \
    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped), \
    .req_poll_grouped_nb=0, \
    .poll_points=0, \
    .period=0, \
    .stopable=0, \
    .max_poll=-1, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &piom_poll_from_tasklet, \
                     (unsigned long)(piom_server_t)&(var) ), \
    .need_manage=0, \
    .poll_timer= MA_TIMER_INITIALIZER(piom_poll_timer, 0, \
                   (unsigned long)(piom_server_t)&(var)), \
    .state=PIOM_SERVER_STATE_INIT, \
    .name=sname, \
  }
#endif /* MA__LWPS */
#else  /* MARCEL */
#define PIOM_SERVER_INIT(var, sname) \
  { \
    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered), \
    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready), \
    .list_req_success=LIST_HEAD_INIT((var).list_req_success), \
    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters), \
    .registered_req_not_yet_polled=0, \
    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped), \
    .req_poll_grouped_nb=0, \
    .poll_points=0, \
    .period=0, \
    .stopable=0, \
    .max_poll=-1, \
    .chain_poll=LIST_HEAD_INIT((var).chain_poll), \
    .state=PIOM_SERVER_STATE_INIT, \
    .name=sname, \
  }
#endif				// MARCEL

#define PIOM_EXCEPTION_RAISE(e) abort();

/********************************************************************
 *  INLINE FUNCTIONS
 */
/* Callback registration */
__tbx_inline__ static int piom_server_add_callback(piom_server_t server,
						    piom_op_t op,
						    piom_pcallback_t func)
{
#ifdef PIOM_FILE_DEBUG
	/* This function must be called between initialization and 
	 * events server startup*/
	PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_INIT);
	/* Verifies the index is ok */
	PIOM_BUG_ON(op >= PIOM_FUNCTYPE_SIZE || op < 0);
	/* Verifies the function isn't yet registered */
	PIOM_BUG_ON(server->funcs[op].func != NULL);
#endif
	server->funcs[op] = func;
	return 0;
}

static __tbx_inline__ int piom_polling_is_required(unsigned polling_point)
    TBX_UNUSED;
static __tbx_inline__ void piom_check_polling(unsigned polling_point)
    TBX_UNUSED;
void __piom_check_polling(unsigned polling_point);

static __tbx_inline__ int piom_polling_is_required(unsigned polling_point)
{
	return !list_empty(&piom_list_poll);
}

static __tbx_inline__ void piom_check_polling(unsigned polling_point)
{
	if (piom_polling_is_required(polling_point))
		__piom_check_polling(polling_point);
}

/********************************************************************
 * Information functions about the machine's state
 */
/*
#ifdef MARCEL
__tbx_inline__ static int NB_RUNNING_THREADS(void)
{
	int res;
	marcel_threadslist(0, NULL, &res, NOT_BLOCKED_ONLY);
	return res - nb_comm_threads;
}
#endif				// MARCEL
*/
/********************************************************************
 *  FUNCTIONS PROTOTYPES 
 */
int piom_test_activity(void);
/* Static initialization
 * var : the constant var piom_server_t
 * name: for debug messages
 */
void piom_server_init(piom_server_t server, char *name);

void piom_server_start_lwp(piom_server_t server, unsigned nb_lwps);

/* Polling setup */
int piom_server_set_poll_settings(piom_server_t server,
				   unsigned poll_points,
				   unsigned poll_period, int max_poll);

/* Start the server
 * It is then possible to wait for events
 */
int piom_server_start(piom_server_t server);

/* Stop the server
 * Reinitialize it before restarting it
 */
int piom_server_stop(piom_server_t server);

// TODO timeout support
int piom_wait(piom_server_t server, piom_req_t req,
	       piom_wait_t wait, piom_time_t timeout);

/* Un raccourci pratique des fonctions suivantes, utile si l'on ne
 * soumet la requête qu'une seule fois. Les opérations suivantes sont
 * effectuées : initialisation, soumission et attente d'un
 * événement avec ONE_SHOT positionné */

/* Query initialization
 * (call it first if piom_wait is not used) */
int piom_req_init(piom_req_t req);

/* Add a specific attribute to a query */
int piom_req_attr_set(piom_req_t req, int attr);

/* Submit a query
 *   the server CAN start polling if it "want" */
int piom_req_submit(piom_server_t server, piom_req_t req);

/* Test a submitted query (return 1 if the query was completed, 0 otherwise) */
__tbx_inline__ static int piom_test(piom_req_t req)
{
	return req->state & PIOM_STATE_OCCURED;
}

/* Cancel a query.
 *   threads waiting for it are woken up and ret_code is returned */
int piom_req_cancel(piom_req_t req, int ret_code);

/* Waits until an already registered query is completed */
/* TODO : timeout support */
int piom_req_wait(piom_req_t req, piom_wait_t wait,
		   piom_time_t timeout);

/* Waits until any query is completed*/
int piom_server_wait(piom_server_t server, piom_time_t timeout);

/* Returns a completed query without the NO_WAKE_SERVER attribute
 *  (usefull when server_wait() returns)
 * When a query fails (wait, req_cancel or ONE_SHOT), it is also
 *  removed from this list */
piom_req_t piom_get_success_req(piom_server_t server);

/* Events server's Mutex
 *
 * - Callbacks are ALWAYS called inside this mutex
 * - BLOCK_ONE|ALL callbacks have to unlock it before the syscall 
 *     and lock it after using piom_callback_[un]lock().
 * - Previous functions can be called with or without this lock
 * - The mutex is unlocked in the waiting functions (piom_*wait*())
 * - The callbacks and the wake up of the threads waiting for the
 *    events signaled by the callback are atomic.
 * - If the mutex is held before the waiting function ((piom_wait_*),
 *    the wake up will be atomic (ie: a callback signaling the waited 
 *    event will wake up the thread).
 * - If a query has the ONE_SHOT attribute, the disactivation is atomic
 */
int piom_lock(piom_server_t server);
int piom_unlock(piom_server_t server);
/* Used by BLOCK_ONE and BLOCK_ALL before and after the syscall */
int piom_callback_will_block(piom_server_t server, piom_req_t req);
int piom_callback_has_blocked(piom_server_t server);

/* Forced polling */
void piom_poll_force(piom_server_t server);
/* Poll a specified query */
void piom_poll_req(piom_req_t req, unsigned usec);
/* Stop polling a specified query */
void piom_poll_stop(piom_req_t req);
/* Resume polling a specified query */
void piom_poll_resume(piom_req_t req);

/* Idem, ensures the polling is done before it returns */
void piom_poll_force_sync(piom_server_t server);

/* Used by initializers */
void piom_poll_from_tasklet(unsigned long id);
void piom_poll_timer(unsigned long id);

int piom_req_free(piom_req_t req);
#endif
