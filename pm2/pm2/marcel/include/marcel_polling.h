
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


#section macros
/* Récupère l'adresse d'une structure englobante 
   Il faut donner :
   - l'adresse du champ interne
   - le type de la structure englobante
   - le nom du champ interne dans la structure englobante
 */
#define struct_up(ptr, type, member) \
        ((type *)((char *)(typeof (&((type *)0)->member))(ptr)- \
		  (unsigned long)(&((type *)0)->member)))


#section types
#define MARCEL_EV_POLL_AT_TIMER_SIG  1
#define MARCEL_EV_POLL_AT_YIELD      2
#define MARCEL_EV_POLL_AT_LIB_ENTRY  4
#define MARCEL_EV_POLL_AT_IDLE       8

typedef struct marcel_ev_server *marcel_ev_serverid_t;

enum {
	MARCEL_EV_OPT_EV_IS_GROUPED=1,
	MARCEL_EV_OPT_EV_ITER=2,
};

typedef enum {
	/* id, XX, ev to poll, nb_ev_grouped, EV_IS_GROUPED */
	MARCEL_EV_FUNCTYPE_POLL_ONE,
	/* id, XX, NA, nb_ev_grouped, NA */
	MARCEL_EV_FUNCTYPE_POLL_GROUP,
	/* id, XX, NA, nb_ev_grouped, NA */
	MARCEL_EV_FUNCTYPE_POLL_ALL,
	/* id, XX, ev to block, NA, NA */
	MARCEL_EV_FUNCTYPE_BLOCK_ONE,
	MARCEL_EV_FUNCTYPE_BLOCK_GROUP,
	MARCEL_EV_FUNCTYPE_BLOCK_ALL,
	/* PRIVATE */
	MA_EV_FUNCTYPE_SIZE
} marcel_ev_op_t;

typedef struct marcel_ev *marcel_ev_inst_t;

typedef int (*marcel_ev_func_t)(marcel_ev_serverid_t id, 
				marcel_ev_op_t op,
				marcel_ev_inst_t ev, 
				int nb_ev, int option);

#section structures
#depend "[marcel_structures]"

#section macros
#define MARCEL_EV_SERVER_DECLARE(var) \
  struct marcel_ev_server var
#define MARCEL_EV_SERVER_INIT(var, sname) \
  { \
    .lock=MA_SPIN_LOCK_UNLOCKED, \
    .ev_poll_grouped=LIST_HEAD_INIT((var).ev_poll_grouped), \
    .ev_poll_grouped_nb=0, \
    .ev_poll_ready=LIST_HEAD_INIT((var).ev_poll_ready), \
    .funcs={NULL, }, \
    .poll_points=0, \
    .frequency=0, \
    .poll_list=LIST_HEAD_INIT((var).poll_list), \
    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet, \
                     &marcel_poll_from_tasklet, \
                     (unsigned long)(marcel_ev_serverid_t)&(var) ), \
    .poll_timer= MA_TIMER_INITIALIZER(marcel_poll_timer, 0, \
                   (unsigned long)(marcel_ev_serverid_t)&(var)), \
    .state=MA_EV_SERVER_STATE_INIT, \
    .name=sname, \
  }
#define MARCEL_EV_SERVER_DEFINE(var, sname) \
  MARCEL_EV_SERVER_DECLARE(var) = MARCEL_EV_SERVER_INIT(var, name)

#section functions
inline static void marcel_ev_server_init(marcel_ev_serverid_t id, char* name);
#section inline
inline static void marcel_ev_server_init(marcel_ev_serverid_t id, char* name)
{
	*id=(struct marcel_ev_server)MARCEL_EV_SERVER_INIT(*id, name);
}
#section functions
inline static int marcel_ev_server_add_callback(marcel_ev_serverid_t id, 
						   marcel_ev_op_t op,
						   marcel_ev_func_t func);
int marcel_ev_server_set_poll_settings(marcel_ev_serverid_t id, 
				       unsigned poll_points,
				       unsigned frequency);
#section inline
inline static int marcel_ev_server_add_callback(marcel_ev_serverid_t id, 
						   marcel_ev_op_t op,
						   marcel_ev_func_t func)
{
#ifdef MA__DEBUG
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(id->state != MA_EV_SERVER_STATE_INIT);
	/* On vérifie que l'index est correct */
	MA_BUG_ON(op>=MA_EV_FUNCTYPE_SIZE || op<0);
	/* On vérifie que la fonction n'est pas déjà là */
	MA_BUG_ON(id->funcs[op]!=NULL);
#endif
	id->funcs[op]=func;
	return 0;
}

#section functions
int marcel_ev_server_start(marcel_ev_serverid_t id);

int marcel_ev_wait(marcel_ev_serverid_t id, marcel_ev_inst_t ev);

void marcel_ev_poll_force(marcel_ev_serverid_t id);

void marcel_poll_from_tasklet(unsigned long id);
void marcel_poll_timer(unsigned long id);

#section macros
/* marcel_ev_inst_t ev : iterateur
   marcel_ev_inst_t tmp : variable utilisée en interne
   marcel_ev_serverid_t ps : serveur id
*/
#define FOREACH_EV_POLL_BASE(ev, tmp, ps) \
  list_for_each_entry_safe(ev, tmp, &ps->ev_poll_grouped, ev_list)
/* marcel_ev_inst_t ev, tmp : pointeur sur structure contenant un
                              struct marcel_ev (itérateur et temporaire)
   marcel_ev_serverid_t ps : serveur id
   member : nom de struct marcel_ev dans la structure pointée par ev (ou tmp)
*/
#define FOREACH_EV_POLL(ev, tmp, ps, member) \
  list_for_each_entry_safe(ev, tmp, &(ps)->ev_poll_grouped, member.ev_list)

#define MARCEL_EV_POLL_SUCCESS(id, ev) \
  do { \
        list_del(&(ev)->ev_list); \
        list_add(&(ev)->ev_list, &(id)->ev_poll_ready); \
  } while(0)
 
// =============== PRIVATE ===============

#section marcel_types
enum {
	MA_EV_SERVER_STATE_NONE,
	MA_EV_SERVER_STATE_INIT=1,
	MA_EV_SERVER_STATE_LAUNCHED=2,
};

enum {
	MARCEL_EV_STATE_UNBLOCKED=1,
	MARCEL_EV_STATE_GROUPED=2,
	MARCEL_EV_STATE_NEED_SEM_V=4,
};

#section marcel_structures
#depend "pm2_list.h"
#depend "marcel_sem.h[]"
#depend "marcel_descr.h[types]"
#depend "linux_interrupt.h[]"
#depend "linux_timer.h[]"

struct marcel_ev_server {
	ma_spinlock_t lock;
	struct list_head ev_poll_grouped;
	int ev_poll_grouped_nb;
	struct list_head ev_poll_ready;
	marcel_ev_func_t funcs[MA_EV_FUNCTYPE_SIZE];
	unsigned poll_points;
	unsigned frequency;
	/* Chaine des serveurs en cours de polling (state == 2) */
	struct list_head poll_list;
	struct ma_tasklet_struct poll_tasklet;
	struct ma_timer_list poll_timer;
	int state;
	char* name;
};

struct marcel_ev {
	struct list_head ev_list;
	int state;
	marcel_sem_t sem;
#ifdef MA__DEBUG
	marcel_task_t *task;
#endif
};

#section marcel_variables
extern struct list_head ma_ev_poll_list;

#section marcel_functions
static __inline__ int marcel_polling_is_required(unsigned polling_point)
__attribute__ ((unused));
static __inline__ void marcel_check_polling(unsigned polling_point)
__attribute__ ((unused));
#section marcel_inline
void __marcel_check_polling(unsigned polling_point);

static __inline__ int marcel_polling_is_required(unsigned polling_point)
{
	return !list_empty(&ma_ev_poll_list);
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
	marcel_pollinst_t safe_cur_cell, cur_cell;
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
  FOREACH_EV_POLL((id)->cur_cell, (id)->safe_cur_cell, &(id)->server, inst)
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

