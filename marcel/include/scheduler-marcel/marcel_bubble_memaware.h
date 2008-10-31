
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

/** \file
 * \brief Bubble steal scheduler functions
 * \defgroup marcel_bubble_steal Bubble Steal scheduler
 * @{
 */


/** \brief Steal work
 *
 * This function may typically be called when a processor becomes idle. It will
 * look for bubbles to steal first locally (neighbour processors), then more
 * and more globally.
 *
 * \return 1 if it managed to steal work, 0 else.
 */
#section functions
int marcel_start_idle_memaware(void);
int marcel_stop_idle_memaware(void);
void marcel_start_checkload(void);
void marcel_stop_checkload(void);
extern void ma_put_in_spread(marcel_entity_t *entity, struct marcel_topo_level *level);
extern void marcel_see_bubble(marcel_bubble_t *bubble, int recurse, int number);
/* @} */

#section marcel_variables
#depend "marcel_bubble_sched_interface.h[types]"
extern marcel_bubble_sched_t marcel_bubble_memaware_sched;

extern int nodelevel;
extern marcel_entity_t * to_spread[256];
extern int num_to_spread;
extern ma_spinlock_t to_spread_lock;

extern int checkload;
extern int maxload;

#section marcel_macros
#define MA_DEFAULT_LOAD 1000
#define MA_ATTRACTION_MIN 10000
#define MA_TO_BIG 1.5

/* Pour les tris et les prises de decisions mSpread */

/* coefficient de charge */
#define MA_LOAD_COEF 5
/* coef d'affinite memoire */
#define MA_AFFINITY_COEF 3
/* coef de poids memoire */
#define MA_VOLUME_COEF 2
/* coef de cache */
#define MA_CACHE_COEF 1

/* Leave small entities */
#define MA_LIFE_LOAD 50
/* We can break thin entities */
#define MA_AFFINITY_BREAK 500
/* If bigger, move to nodes */
#define MA_MOVE_ENTITY_WEIGHT 1000
/* First entities ratio which can choose their level */ 
#define MA_RATIO 0.4

/* Pous mSteal */

/* importance de la charge */
#define MA_LOAD_SCORE 5
/* importance du poids memoire de chaque entites */ 
#define MA_VOLUME_SCORE 2
/* importance du partage memoire du contenant */
#define MA_FATHER_AFFINITY_SCORE 3
/* importance du cache */
#define MA_CACHE_SCORE 1

/* types de penalites dues a l'eloignement dans la machine */
#define MA_VP_LEVEL_PENALTY 1
#define MA_NODE_LEVEL_PENALTY 2
#define MA_DEFAULT_PENALTY 3
