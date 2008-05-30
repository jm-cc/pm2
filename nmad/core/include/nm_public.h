/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifndef __nm_public_h__
#define __nm_public_h__

#include <nm_errno.h>
#include <stdlib.h>
#include <Padico/Puk.h>

struct nm_core;
struct nm_drv_ops;
struct nm_pkt_wrap;
struct nm_sched_ops;
struct nm_trk;
struct nm_trk_rq;
struct nm_drv;

typedef int16_t nm_gate_id_t;                 /* needs to support -1 */

#define NUMBER_OF_GATES          255
#define NUMBER_OF_DRIVERS        255
#define NUMBER_OF_PROTOCOLS      255
#define NUMBER_OF_TRACKS         255

#define NM_ANY_GATE ((nm_gate_id_t)-1)

puk_component_t
nm_core_component_load  (const char*entity, 
			 const char*name);

int
nm_core_init		(int			 *argc,
                         char			 *argv[],
                         struct nm_core		**pp_core,
                         int (*sched_load)(struct nm_sched_ops *));

int
nm_core_exit           (struct nm_core		*p_core);

int
nm_core_driver_load	(struct nm_core		 *p_core,
                         puk_component_t         driver,
                         uint8_t		 *p_id);

int
nm_core_driver_init	(struct nm_core		 *p_core,
                         uint8_t		 id,
			 char			**p_url);

struct nm_driver_query_param {
	enum {
		NM_DRIVER_QUERY_BY_NOTHING=0,
		NM_DRIVER_QUERY_BY_INDEX,
		/* NM_DRIVER_QUERY_BY_NUMA_NODE, */
		/* NM_DRIVER_QUERY_BY_SPEED, */
	} key;
	union {
		uint32_t index;
	} value;
};

int
nm_core_driver_query(struct nm_core	*p_core,
		     uint8_t id,
		     struct nm_driver_query_param *params,
		     int nparam);

int
nm_core_driver_load_init_some_with_params(struct nm_core *p_core,
					  int count,
					  puk_component_t*driver_array,
					  struct nm_driver_query_param **params_array,
					  int *nparam_array,
					  uint8_t *p_id_array,
					  char **p_url_array);

/** Simple helpers to prevent basic applications from having to
 * do load and init when they don't tweak anything in between.
 */
static inline int
nm_core_driver_load_init_some(struct nm_core *p_core,
			      int count,
			      puk_component_t*driver_array,
			      uint8_t *p_id_array,
			      char **p_url_array)
{
  struct nm_driver_query_param * params_array[NUMBER_OF_DRIVERS] = { NULL };
  int nparam_array[NUMBER_OF_DRIVERS] = { 0 };
  return nm_core_driver_load_init_some_with_params(p_core, count, driver_array,
						   params_array, nparam_array,
						   p_id_array, p_url_array);
}

static inline int
nm_core_driver_load_init_with_params(struct nm_core *p_core,
				     puk_component_t driver,
				     struct nm_driver_query_param *params,
				     int nparam,
				     uint8_t *p_id,
				     char **p_url)
{
  struct nm_driver_query_param * params_array[1] = { params };
  int nparam_array[1] = { nparam };
  return nm_core_driver_load_init_some_with_params(p_core, 1, &driver,
						   params_array, nparam_array,
						   p_id, p_url);
}

static inline int
nm_core_driver_load_init(struct nm_core		 *p_core,
			 puk_component_t          driver,
			 uint8_t		 *p_id,
			 char			**p_url)
{
  return nm_core_driver_load_init_with_params(p_core, driver, NULL, 0, p_id, p_url);
}

int
nm_core_driver_exit	(struct nm_core		 *p_core);

int
nm_core_trk_alloc       (struct nm_core		 * p_core,
			 struct nm_drv		 * p_drv,
			 struct nm_trk_rq	 * p_trk_rq);
int
nm_core_trk_free	(struct nm_core		*p_core,
			 struct nm_trk		*p_trk);

int
nm_core_gate_init	(struct nm_core		 *p_core,
                         nm_gate_id_t		 *p_gate_id);

int
nm_core_gate_accept	(struct nm_core		 *p_core,
                         nm_gate_id_t		  gate_id,
                         uint8_t		  drv_id,
                         const char		 *drv_trk_url);

int
nm_core_gate_connect	(struct nm_core		 *p_core,
                         nm_gate_id_t		  gate_id,
                         uint8_t		  drv_id,
                         const char		 *drv_trk_url);

int
nm_core_wrap_buffer	(struct nm_core		 *p_core,
                         nm_gate_id_t		  gate_id,
                         uint8_t		  proto_id,
                         uint8_t		  seq,
                         void			 *buf,
                         uint64_t		  len,
                         struct nm_pkt_wrap	**pp_pw);


int
nm_core_post_send	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw);



int
nm_core_post_recv	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw);

int
nm_schedule		(struct nm_core		 *p_core);

#ifdef PIOMAN
void 
nmad_lock(void);

void 
nmad_unlock(void);

int
nm_piom_post_all        (struct nm_core	         *p_core);

int
nm_core_exit_piom(struct nm_core *p_core);

#else /* PIOMAN */

static inline void 
nmad_lock(void)
{ }

static inline void 
nmad_unlock(void)
{ }

#endif /* PIOMAN */

#endif
