
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

#define MA_FILE_DEBUG polling
#include "marcel.h"

/****************************************************************
 * Voir marcel_io.c pour un exemple d'implémentation
 */

/****************************************************************
 * Compatibility stuff
 * Cette partie disparaîtra bientôt...
 */
#define MAX_POLL_IDS    16
	
static unsigned nb_poll_structs = 0;
static struct poll_struct poll_structs[MAX_POLL_IDS];

static int compat_poll_one(marcel_ev_serverid_t id, 
		    marcel_ev_op_t op,
		    marcel_ev_inst_t ev, 
		    int nb_ev, int option)
{
	marcel_pollinst_t inst=struct_up(ev, poll_cell_t, inst);
	marcel_pollid_t ps=struct_up(id, struct poll_struct, server);
	int first_call=TRUE;

	ps->cur_cell = inst;
	if (option & MARCEL_EV_OPT_EV_IS_GROUPED) {
		first_call=FALSE;
	}
	if ((*ps->fastfunc)(ps, inst->arg, first_call)) {
		MARCEL_EV_POLL_SUCCESS(id, ev);
	}
	return 0;
}

static int compat_poll_all(marcel_ev_serverid_t id, 
		    marcel_ev_op_t op,
		    marcel_ev_inst_t ev, 
		    int nb_ev, int option)
{
	marcel_pollid_t ps=struct_up(id, struct poll_struct, server);
	(*ps->func)(ps,
		    ma_nr_running(),
		    marcel_sleepingthreads(),
		    marcel_blockedthreads());	
	return 0;
}

static int compat_poll_group(marcel_ev_serverid_t id, 
		      marcel_ev_op_t op,
		      marcel_ev_inst_t ev, 
		      int nb_ev, int option)
{
	marcel_pollid_t ps=struct_up(id, struct poll_struct, server);
	(*ps->gfunc)(ps);
	return 0;
}


marcel_pollid_t marcel_pollid_create_X(marcel_pollgroup_func_t g,
				       marcel_poll_func_t f,
				       marcel_fastpoll_func_t h,
				       unsigned polling_points,
				       char* name)
{
	marcel_pollid_t id;

	LOG_IN();

	if(nb_poll_structs == MAX_POLL_IDS) {
		RAISE(CONSTRAINT_ERROR);
	}

#warning non Thread-Safe incrementation here
#warning marcel_pollid_create will be removed (and not corrected) in flavor of marcel_ev_server_init	
	id = &poll_structs[nb_poll_structs++];

	marcel_ev_server_init(&id->server, name);
	marcel_ev_server_set_poll_settings(&id->server, 
					   (polling_points
					    |MARCEL_EV_POLL_AT_IDLE)
					   & MARCEL_EV_POLL_AT_TIMER_SIG, 1);
	if (g) {
		id->gfunc = g;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_GROUP,
					      compat_poll_group);
	}
	if (f) {
		id->func = f;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_ALL,
					      compat_poll_all);
	}
	if (h) {
		id->fastfunc = h;
		marcel_ev_server_add_callback(&id->server,
					      MARCEL_EV_FUNCTYPE_POLL_ONE,
					      compat_poll_one);
	}

	marcel_ev_server_start(&id->server);

	mdebug("registering pollid %p (gr=%p, func=%p, fast=%p, pts=%x)\n",
	       id, g, f, h, id->server.poll_points);

	LOG_OUT();

	return id;
}

void marcel_poll(marcel_pollid_t id, any_t arg)
{
	poll_cell_t cell;

	cell.id=&id->server;
	cell.arg=arg;

	marcel_ev_wait(&id->server, &cell.inst);
}

void marcel_force_check_polling(marcel_pollid_t id)
{
	LOG_IN();
	
	marcel_ev_poll_force(&id->server);
	
	LOG_OUT();
	return;
}

/****************************************************************
 * Implémentation courante
 ****************************************************************/


/****************************************************************
 * Fonctions d'initialisation
 */

int marcel_ev_server_set_poll_settings(marcel_ev_serverid_t id, 
					      unsigned poll_points,
					      unsigned frequency)
{
#ifdef MA__DEBUG
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(id->state != MA_EV_SERVER_STATE_INIT);
#endif
	id->poll_points=poll_points;
	if (poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		frequency=frequency?:1;
		id->frequency = frequency * MA_JIFFIES_PER_TIMER_TICK;
	}
	return 0;	
}

int marcel_ev_server_start(marcel_ev_serverid_t id)
{
	/* Cette fonction doit être appelée entre l'initialisation et
	 * le démarrage de ce serveur d'événements */
	MA_BUG_ON(id->state != MA_EV_SERVER_STATE_INIT);
	if (!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]
	    && !id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL]) {
		mdebug("One poll function needed for %s\n", id->name);
		RAISE(PROGRAM_ERROR);
	}
	id->state=MA_EV_SERVER_STATE_LAUNCHED;
	return 0;
}


/****************************************************************
 * Gestion du polling
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
 *     (exemple marcel_ev_wait())
 */

/* Variable contenant la liste chainée des serveurs de polling ayant
 * quelque chose à scruter.
 * Le lock permet de protéger le parcours/modification de la liste
 */
LIST_HEAD(ma_ev_poll_list);
static ma_rwlock_t ev_poll_lock = MA_RW_LOCK_UNLOCKED;


/* On réveille toutes les tâches prêtes
 * On fait un sem_V sur celles qui le demandent
 * On compte les tâches déjà groupées réveillées 
 *   (donc pas celle du POLL_ONE de départ)
 */
inline static int __wake_ready(marcel_ev_serverid_t id)
{
	int waken_some_grouped_task = 0;
	marcel_ev_inst_t ev;

	list_for_each_entry(ev, &id->ev_poll_ready, ev_list) {
		ev->state |= MARCEL_EV_STATE_UNBLOCKED;
		if (ev->state & MARCEL_EV_STATE_GROUPED) {
			waken_some_grouped_task++;
		}
		if (ev->state & MARCEL_EV_STATE_NEED_SEM_V) {
			marcel_sem_V(&ev->sem);
		}
		mdebug("Poll succeed with task %p\n", ev->task);
	}
	id->ev_poll_grouped_nb -= waken_some_grouped_task;
	if (waken_some_grouped_task) {
		mdebug("Nb grouped task set to %i\n", 
		       id->ev_poll_grouped_nb);
	}
	INIT_LIST_HEAD(&id->ev_poll_ready);
	return waken_some_grouped_task;
}

/* On groupe si nécessaire, ie :
 * s'il y a  au moins 2 requetes
 * ou s'il n'y a pas de "fast poll", alors il faut factoriser.
 *
 * Il doit y avoir au moins un tâche à grouper.
 */
inline static int __poll_group(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!id->ev_poll_grouped_nb);
#endif
	if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_GROUP] &&
	    ( id->ev_poll_grouped_nb > 1 
	      || ! id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])) {
		mdebug("Factorizing %i polling(s) with POLL_GROUP\n",
		       id->ev_poll_grouped_nb);
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_GROUP])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
			 ev, id->ev_poll_grouped_nb, 0);
		return 1;
	}
	mdebug("No need to group %i polling(s)\n",
	       id->ev_poll_grouped_nb);
	return 0;
}

/* On démarre un polling (et éventuellement le timer) en l'ajoutant
 * dans la liste
 */
inline static void __poll_start(marcel_ev_serverid_t id)
{
	ma_write_lock_softirq(&ev_poll_lock);
	MA_BUG_ON(!list_empty(&id->poll_list));
	mdebug("Starting polling for %s\n", id->name);
	list_add(&id->poll_list, &ma_ev_poll_list);
	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebug("Starting timer polling for %s at frequency %i\n",
		       id->name, id->frequency);
		/* ma_mod_timer des fois qu'un ancien soit toujours
		 * pending... */
		ma_mod_timer(&id->poll_timer, ma_jiffies+id->frequency);
	}
	ma_write_unlock_softirq(&ev_poll_lock);
}

/* Plus rien à scruter 
 * reste à
 * + arrêter le timer
 * + enlever le serveur de la liste des scrutations en cours
 */
inline static void __poll_stop(marcel_ev_serverid_t id)
{
	ma_write_lock_softirq(&ev_poll_lock);
	mdebug("Stopping polling for %s\n", id->name);
	list_del_init(&id->poll_list);
	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebug("Stopping timer polling for %s\n", id->name);
		ma_del_timer(&id->poll_timer);
	}
	ma_write_unlock_softirq(&ev_poll_lock);
}

/* On vient de faire une scrutation complète (pas un POLL_ONE sur le
 * premier), on réenclanche un timer si nécessaire.
 */
inline static void __update_timer(marcel_ev_serverid_t id)
{
	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
		mdebugl(7, "Update timer polling for %s at frequency %i\n",
			id->name, id->frequency);
		ma_mod_timer(&id->poll_timer, ma_jiffies+id->frequency);
	}
}

//inline static void __ensure_timer(marcel_ev_serverid_t id)
//{
//	if (id->poll_points & MARCEL_EV_POLL_AT_TIMER_SIG) {
//		ma_debug(polling,
//			 "Ensuring timer polling for %s at frequency %i\n",
//			 id->name, id->frequency);
//		ma_mod_timer(&id->poll_timer, id->poll_timer.expires);
//	}
//}

// Checks to see if some polling jobs should be done.
static void check_polling_for(marcel_ev_serverid_t id)
{
	int nb=id->ev_poll_grouped_nb;
#ifdef MA__DEBUG
	static int count=0;

	mdebugl(7, "Check polling for %s\n", id->name);
	
	if (! count--) {
		mdebugl(6, "Check polling 50000 for %s\n", id->name);
		count=50000;
	}
#endif

	if (!nb) 
		return;

	if(nb == 1 && id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE, 
			 list_entry(id->ev_poll_grouped.next,
				    struct marcel_ev, ev_list),
			 nb, MARCEL_EV_OPT_EV_IS_GROUPED);

	} else if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL]) {
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL])
			(id, MARCEL_EV_FUNCTYPE_POLL_ALL,
			 NULL, nb, 0);
		
	} else if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		marcel_ev_inst_t ev, temp;
		FOREACH_EV_POLL_BASE(ev, temp, id){
			(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
				(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
				 ev, nb,
				 MARCEL_EV_OPT_EV_IS_GROUPED
				 |MARCEL_EV_OPT_EV_ITER);
		}
	} else {
		/* Pas de méthode disponible ! */
		RAISE(PROGRAM_ERROR);
	}

	if (__wake_ready(id)) {
		/* Le nombre de tache en cours de polling a varié */
		if(!id->ev_poll_grouped_nb) {
			__poll_stop(id);
			return;
		} else {
			__poll_group(id, NULL);
		}
	}
	__update_timer(id);
	return;
}

/* call-back pour la tasklet */
void marcel_poll_from_tasklet(unsigned long hid)
{
	marcel_ev_serverid_t id=(marcel_ev_serverid_t)hid;
	mdebugl(7, "Tasklet function for %s\n", id->name);
	check_polling_for(id);
	return;
}

/* call-back pour le timer */
void marcel_poll_timer(unsigned long hid)
{
	marcel_ev_serverid_t id=(marcel_ev_serverid_t)hid;
	mdebugl(7, "Timer function for %s\n", id->name);
	ma_tasklet_schedule(&id->poll_tasklet);
	return;
}

/* Appelé par le code en divers points (yield et lib_entry
 * principalement) */
void __marcel_check_polling(unsigned polling_point)
{

	marcel_ev_serverid_t id;

	//debug("Checking polling at %i\n", polling_point);
	ma_read_lock_softirq(&ev_poll_lock);
	list_for_each_entry(id, &ma_ev_poll_list, poll_list) {
		if (id->poll_points & polling_point) {
			mdebugl(7, "Scheduling polling for %s at point %i\n",
				id->name, polling_point);
			ma_tasklet_schedule(&id->poll_tasklet);
		}
	}
	ma_read_unlock_softirq(&ev_poll_lock);
}

/* Force une scrutation sur un serveur */
void marcel_ev_poll_force(marcel_ev_serverid_t id)
{
	LOG_IN();
	mdebug("Poll forced for %s\n", id->name);
	/* Pas très important si on loupe quelque chose ici (genre
	 * liste modifiée au même instant)
	 */
	if (!list_empty(&id->ev_poll_grouped)) {
		ma_tasklet_schedule(&id->poll_tasklet);
	}
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

int marcel_ev_wait(marcel_ev_serverid_t id, marcel_ev_inst_t ev)
{
	int old_nb_grouped;
	int waken_up=0;
	LOG_IN();
	
#ifdef MA__DEBUG
	MA_BUG_ON(id->state!=MA_EV_SERVER_STATE_LAUNCHED);
#endif
	mdebug("Marcel_poll (thread %p)...\n", marcel_self());
	mdebug("using pollid %s\n", id->name);

	INIT_LIST_HEAD(&ev->ev_list);
	ev->state=0;
#ifdef MA__DEBUG
	ev->task=MARCEL_SELF;
#endif

	ma_spin_lock_softirq(&id->lock);
	ma_tasklet_disable(&id->poll_tasklet);

	old_nb_grouped=id->ev_poll_grouped_nb;

	if (id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		mdebug("Using Immediate POLL_ONE\n");
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE])
			(id, MARCEL_EV_FUNCTYPE_POLL_ONE,
			 ev, id->ev_poll_grouped_nb, 0);
		waken_up=__wake_ready(id);
	}
	if (!(ev->state & MARCEL_EV_STATE_UNBLOCKED)) {
		/* On doit ajouter la requête à celles en attente */
		mdebug("Adding request\n");
		list_add(&ev->ev_list, &id->ev_poll_grouped);
		marcel_sem_init(&ev->sem, 0);
		id->ev_poll_grouped_nb++;
		ev->state |= (MARCEL_EV_STATE_GROUPED|
			      MARCEL_EV_STATE_NEED_SEM_V);
		__poll_group(id, ev);
	}
	if (!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
		/* On a pas encore testé. On tente ici */
#ifdef MA__DEBUG
		MA_BUG_ON(!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL]);
#endif
		mdebug("Using Immediate POLL_ALL\n");
		(*id->funcs[MARCEL_EV_FUNCTYPE_POLL_ALL])
			(id, MARCEL_EV_FUNCTYPE_POLL_ALL,
			 ev, id->ev_poll_grouped_nb, 0);	
		waken_up=__wake_ready(id);
	}
	/* On a fait un poll d'une manière ou d'une autre */
	/* On teste si des requêtes groupées sont finies */
	if (waken_up) {
		/* Le nombre de tache en cours de polling a varié */
		if(!id->ev_poll_grouped_nb) {
			/* Rien a scruter */
			if (old_nb_grouped) {
				/* Il y avait du polling. On l'arrête */
				__poll_stop(id);
			}
		} else {
			/* Des choses à scruter */
			__poll_group(id, NULL);
			if (!id->funcs[MARCEL_EV_FUNCTYPE_POLL_ONE]) {
				/* On a fait un poll global, on
				 * s'occupe du timer.
				 *
				 * Le timer était nécessairement actif car
				 * 1) il y a eu au moins une tâche réveillée
				 * 2) il reste encore une tâche en attente
				 * 3) donc il y avait une tâche en attente avant
				 */
				__update_timer(id);
			}
		}
	} else {
		/* Rien de réveillé. Que doit-on faire ? */
		if(id->ev_poll_grouped_nb) {
			/* Il y a du polling a faire */
			if(old_nb_grouped==0) {
				/* On vient de mettre la première tâche */
				__poll_start(id);
			} else {
				__update_timer(id);
			}
		}
	}
	ma_tasklet_enable(&id->poll_tasklet);
	ma_spin_unlock_softirq(&id->lock);
	//__ensure_timer(id);

	if (ev->state & MARCEL_EV_STATE_UNBLOCKED) {
		return 0;
	}
	marcel_sem_P(&ev->sem);
	return 0;
}

