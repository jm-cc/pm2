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

#ifdef PIOMAN
#include "nm_piom.h"
#endif
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>
#ifdef PM2_NUIOA
#include <numa.h>
#endif

#include <pm2_common.h>

#include "nm_private.h"
#include "nm_core.h"
#include "nm_core_inline.h"
#include "nm_drv.h"
#include "nm_drv_cap.h"
#include "nm_trk.h"
#include "nm_trk_rq.h"
#include "nm_sched.h"
#include "nm_cnx_rq.h"
#include <nm_public.h>
#include <nm_drivers.h>
#include "nm_log.h"

/* Macros
 */
#define INITIAL_PW_NUM		16
#define INITIAL_IOV1_NUM	16
#define INITIAL_IOV2_NUM	16

/* fast allocator structs */
p_tbx_memory_t nm_core_pw_mem	= NULL;
p_tbx_memory_t nm_core_iov1_mem	= NULL;
p_tbx_memory_t nm_core_iov2_mem	= NULL;

#ifdef PIOMAN
static piom_spinlock_t piom_big_lock;

void 
nmad_lock() {
	piom_spin_lock(&piom_big_lock);
}

void 
nmad_unlock() {
	piom_spin_unlock(&piom_big_lock);
}
#endif

/* memory management part						*/

/** Allocate a new track.
 *
 * Tracks should be allocated at initialization time, before any
 * connection is open with the corresponding driver. The track is not
 * automatically connected if some connections already are open on the driver.
 */
int
nm_core_trk_alloc(struct nm_core	 * p_core,
		  struct nm_drv		 * p_drv,
		  struct nm_trk_rq	 * p_trk_rq) {
        struct nm_trk		*p_trk;
        uint8_t			 id;
        int	err;

        if (p_drv->nb_tracks == NUMBER_OF_TRACKS) {
                err = -NM_ENOMEM;
                goto out;
        }

        p_trk	= TBX_MALLOC(sizeof(struct nm_trk));
        if (!p_trk) {
                err = -NM_ENOMEM;
                goto out;
        }

        id	= p_drv->nb_tracks++;

        p_trk->id		= id;
        p_trk->p_drv		= p_drv;
        p_trk->url		= NULL;
        p_trk->out_req_nb	=    0;
        p_trk->priv		= NULL;

        memset(&p_trk->cap, 0, sizeof(struct nm_trk_cap));

        if (id) {
                p_drv->p_track_array	=
                        TBX_REALLOC(p_drv->p_track_array,
                                    (id+1) * sizeof (struct nm_trk *));
        } else {
                p_drv->p_track_array	= TBX_MALLOC(sizeof (struct nm_trk *));
        }

        p_drv->p_track_array[id]	= p_trk;
        p_trk_rq->p_trk = p_trk;

	FUT_DO_PROBE3(FUT_NMAD_NIC_NEW_INPUT_LIST, p_drv->id, p_trk->id, p_drv->nb_tracks);
	FUT_DO_PROBE3(FUT_NMAD_NIC_NEW_OUTPUT_LIST, p_drv->id, p_trk->id, p_drv->nb_tracks);

        err = p_drv->driver->open_trk(p_trk_rq);
        if (err != NM_ESUCCESS) {
                goto out_free;
        }

 out:
        return err;

 out_free:
        nm_core_trk_free(p_core, p_trk);
        goto out;
}

/** Clean up a track.
 *
 */
int
nm_core_trk_free(struct nm_core		*p_core TBX_UNUSED,
                 struct nm_trk		*p_trk) {
        struct nm_drv		 *p_drv;
	int	err;

        p_drv = p_trk->p_drv;
        err = p_drv->driver->close_trk(p_trk);

        TBX_FREE(p_trk);
        p_trk = NULL;

        return err;
}

/** Load and initialize a scheduler.
 *
 * This operation should only be performed once per session, at least for now.
 */
static
int
nm_core_schedule_init(struct nm_core *p_core,
                      int (*sched_load)(struct nm_sched_ops *)) {
        struct nm_sched	*p_sched	= NULL;
        int err;

        if (p_core->p_sched) {
                err	= -NM_EALREADY;
                goto out;
        }

        p_sched	= TBX_MALLOC(sizeof(struct nm_sched));
        if (!p_sched) {
                err = -NM_ENOMEM;
                goto out;
        }

        memset(p_sched, 0, sizeof(struct nm_sched));

        err = sched_load(&p_sched->ops);
        if (err != NM_ESUCCESS)
                goto out_free;

        p_core->p_sched	= p_sched;
        p_sched->p_core	= p_core;

        p_sched->pending_aux_recv_req	= tbx_slist_nil();
        p_sched->post_aux_recv_req	= tbx_slist_nil();
        p_sched->pending_perm_recv_req	= tbx_slist_nil();
        p_sched->post_perm_recv_req	= tbx_slist_nil();

        err = p_sched->ops.init(p_sched);
        if (err != NM_ESUCCESS)
                goto out_free_lists;

        err = NM_ESUCCESS;

 out:
        return err;

 out_free_lists:
        tbx_slist_free(p_sched->pending_aux_recv_req);
        tbx_slist_free(p_sched->post_aux_recv_req);
        tbx_slist_free(p_sched->pending_perm_recv_req);
        tbx_slist_free(p_sched->post_perm_recv_req);

 out_free:
        TBX_FREE(p_sched);
        p_core->p_sched	= NULL;
        goto out;
}

/** Shutdown a scheduler.
 *
 */
static
int
nm_core_schedule_exit(struct nm_core *p_core) {
        struct nm_sched	*p_sched	= p_core->p_sched;
        int err;

        err = p_sched->ops.exit(p_sched);

        tbx_slist_clear(p_sched->pending_aux_recv_req);
        tbx_slist_clear(p_sched->post_aux_recv_req);
        tbx_slist_clear(p_sched->pending_perm_recv_req);
        tbx_slist_clear(p_sched->post_perm_recv_req);

        tbx_slist_free(p_sched->pending_aux_recv_req);
        tbx_slist_free(p_sched->post_aux_recv_req);
        tbx_slist_free(p_sched->pending_perm_recv_req);
        tbx_slist_free(p_sched->post_perm_recv_req);

        TBX_FREE(p_sched);
        p_core->p_sched = NULL;

        return err;
}

#ifdef PIOMAN
int nm_piom_core_poll(piom_server_t            server,
		  piom_op_t                _op,
		  piom_req_t               req, 
		  int                       nb_ev, 
		  int                       option) {
	struct nm_core *p_core=struct_up(server, struct nm_core, server);

	nmad_lock();
	nm_piom_post_all(p_core);
	nmad_unlock();
	
	return 0;
}

/* Initialisation de PIOMan  */
int 
nm_core_init_piom(struct nm_core *p_core){
	LOG_IN();

	piom_spin_lock_init(&piom_big_lock);
	return 0;
}

int 
nm_core_exit_piom(struct nm_core *p_core) {
  int i;
  for(i=0;i<p_core->nb_drivers;i++)
    piom_server_kill(&p_core->driver_array[i].server);
  return NM_ESUCCESS;
}

/* Initialisation du serveur utilisé pour les drivers */
int
nm_core_init_piom_drv(struct nm_core*p_core,struct nm_drv *p_drv) {
	LOG_IN();
	piom_server_init(&p_drv->server, "NMad IO Server");

#ifdef  MARCEL_REMOTE_TASKLETS
	marcel_vpset_t vpset;
	marcel_vpset_vp(&vpset, 0);
	ma_remote_tasklet_set_vpset(&p_drv->server.poll_tasklet, &vpset);
#endif

	piom_server_set_poll_settings(&p_drv->server,
				       PIOM_POLL_AT_TIMER_SIG
				       | PIOM_POLL_AT_IDLE
				       | PIOM_POLL_WHEN_FORCED, 1, -1);
	
	/* Définition des callbacks */
	piom_server_add_callback(&p_drv->server,
				  PIOM_FUNCTYPE_POLL_POLLONE,
				  (piom_pcallback_t) {
		.func = &nm_piom_poll,
			 .speed = PIOM_CALLBACK_NORMAL_SPEED});
	
#warning "TODO: Fix the poll_any issues."
//	piom_server_add_callback(&p_drv->server,
//				  PIOM_FUNCTYPE_POLL_POLLANY,
//				  (piom_pcallback_t) {
//		.func = &nm_piom_poll_any,
//			 .speed = PIOM_CALLBACK_NORMAL_SPEED});

#ifdef PIOM_BLOCKING_CALLS
	if((p_drv->driver->get_capabilities(p_drv))->is_exportable){
          //		piom_server_start_lwp(&p_drv->server, 1);
		piom_server_add_callback(&p_drv->server,
					  PIOM_FUNCTYPE_BLOCK_WAITONE,
					  (piom_pcallback_t) {
			.func = &nm_piom_block,
				 .speed = PIOM_CALLBACK_NORMAL_SPEED});
		piom_server_add_callback(&p_drv->server,
					 PIOM_FUNCTYPE_BLOCK_WAITANY,
					 (piom_pcallback_t) {
			.func = &nm_piom_block_any,
				 .speed = PIOM_CALLBACK_NORMAL_SPEED});
		
	}
#endif

	piom_server_start(&p_drv->server);

#ifdef PIO_OFFLOAD
	/* Very ugly for now */
	struct nm_pkt_wrap *post_rq = TBX_MALLOC(sizeof(struct nm_pkt_wrap));
	post_rq->gate_priv	= NULL;
	post_rq->drv_priv	= NULL;


	post_rq->p_drv  = p_drv;
	post_rq->p_trk  = NULL;
	post_rq->p_gate = NULL;
	post_rq->p_gdrv = NULL;
	post_rq->p_gtrk = NULL;
	post_rq->proto_id = 0;
	post_rq->seq = 0;
	post_rq->sched_priv = NULL;
	post_rq->drv_priv   = NULL;
	post_rq->gate_priv  = NULL;
	post_rq->proto_priv = NULL;

	post_rq->pkt_priv_flags = 0;
	post_rq->length = 0;
	post_rq->iov_flags = 0;

	post_rq->p_pkt_head = NULL;
	post_rq->data = NULL;
	post_rq->len_v = NULL;

	post_rq->iov_priv_flags  = 0;
	post_rq->v_size          = 0;
	post_rq->v_first         = 0;
	post_rq->v_nb            = 0;

	post_rq->v = NULL;
	post_rq->nm_v = NULL;

	post_rq->slist=NULL;
	
	piom_req_init(&post_rq->inst);
	post_rq->inst.server=&p_drv->server;
	post_rq->which=NONE;
	post_rq->err = -NM_EAGAIN;
	post_rq->inst.state|=PIOM_STATE_DONT_POLL_FIRST|PIOM_STATE_ONE_SHOT;
	piom_req_submit(&p_drv->server, &post_rq->inst);
#endif  

	return 0;
}
#endif

/** Load a driver.
 *
 * Out parameters:
 * p_id  - contains the id of the new driver
 */
int
nm_core_driver_load(struct nm_core	 *p_core,
                    puk_component_t       driver,
                    uint8_t		 *p_id) {
        struct nm_drv	*p_drv		= NULL;
        int err;

        NM_LOG_IN();
        if (p_core->nb_drivers == NUMBER_OF_DRIVERS) {
                err	= -NM_ENOMEM;
                goto out;
        }
	assert(driver != NULL);
        p_drv = p_core->driver_array + p_core->nb_drivers;
        memset(p_drv, 0, sizeof(struct nm_drv));
	p_drv->p_core   = p_core;
        p_drv->id       = p_core->nb_drivers;
	p_drv->assembly = driver;
	p_drv->driver   = puk_adapter_get_driver_NewMad_Driver(p_drv->assembly, NULL);

        if (p_id) {
                *p_id	= p_drv->id;
        }
        p_core->nb_drivers++;

        err = NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}


/** Query resources and register them for a driver.
 *
 */
int
nm_core_driver_query(struct nm_core	 *p_core,
		     uint8_t id,
		     struct nm_driver_query_param *params,
		     int nparam) {
        struct nm_drv	*p_drv		= NULL;
        int err;

        NM_LOG_IN();
        p_drv	= p_core->driver_array + id;

	if (!p_drv->driver->query) {
		err = -NM_EINVAL;
                goto out;
        }

        err = p_drv->driver->query(p_drv, params, nparam);
       	if (err != NM_ESUCCESS) {
                NM_DISPF("drv.query returned %d", err);
       	        goto out;
        }

        err = NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}

/** Initialize a driver using previously registered resources.
 *
 * Out parameters:
 * p_url - contains the URL of the driver (memory for the URL is allocated by
 * nm_core)
 */
int
nm_core_driver_init(struct nm_core	 *p_core,
                    uint8_t id,
                    char		**p_url) {
        struct nm_drv	*p_drv		= NULL;
        struct nm_sched	*p_sched	= NULL;
        p_tbx_string_t	 url		= NULL;
	const char *tmp_url             = NULL;
        int err;

        NM_LOG_IN();
        p_drv	= p_core->driver_array + id;
	p_drv->p_core = p_core;
	if (!p_drv->driver->init) {
		err = -NM_EINVAL;
                goto out;
        }

        err = p_drv->driver->init(p_drv);
        if (err != NM_ESUCCESS) {
                NM_DISPF("drv.init returned %d", err);
                goto out;
        }

	if(p_drv->driver->get_driver_url != NULL) {
	  tmp_url = p_drv->driver->get_driver_url(p_drv);
	}

        if (p_url) {
	         *p_url	= (char*)tmp_url;
        }

        p_sched	= p_core->p_sched;
        err	= p_sched->ops.init_trks(p_sched, p_drv);
        if (err != NM_ESUCCESS) {
                NM_DISPF("sched.init_trks returned %d", err);
                goto out;
        }

        if (p_url) {
                int i;

                /* driver url
                 */
                if (tmp_url) {
                        url = tbx_string_init_to_cstring(tmp_url);
                } else {
                        url = tbx_string_init_to_cstring("-");
                }

                for (i = 0; i < p_drv->nb_tracks; i++) {
                        tbx_string_append_char(url, '#');

                        if (p_drv->p_track_array[i]->url) {
                                tbx_string_append_cstring(url, p_drv->p_track_array[i]->url);
                        } else {
                                tbx_string_append_cstring(url, "-");
                        }
                }

                *p_url = tbx_string_to_cstring(url);
                tbx_string_free(url);
        }
	FUT_DO_PROBE1(FUT_NMAD_INIT_NIC, p_drv->id);
	FUT_DO_PROBESTR(FUT_NMAD_INIT_NIC_URL, p_drv->assembly->name);

        err = NM_ESUCCESS;

 out:
        NM_LOG_OUT();

        return err;
}

/** Helper to load and init several drivers at once,
 * with an array of parameters for each driver,
 * and applying numa binding in-between.
 */
int
nm_core_driver_load_init_some_with_params(struct nm_core *p_core,
					  int count,
					  puk_component_t*driver_array,
					  struct nm_driver_query_param **params_array,
					  int *nparam_array,
					  uint8_t *p_id_array,
					  char **p_url_array)
{
	uint8_t id;
	int i;
#ifdef PM2_NUIOA
	int preferred_node = PM2_NUIOA_ANY_NODE;
	int nuioa = (numa_available() >= 0);
	char * nuioa_criteria = getenv("PM2_NUIOA_CRITERIA");
	int nuioa_with_latency = ((nuioa_criteria != NULL) && !strcmp(nuioa_criteria, "latency"));
	int nuioa_with_bandwidth = ((nuioa_criteria != NULL) && !strcmp(nuioa_criteria, "bandwidth"));
	int nuioa_current_best = 0;
#endif /* PM2_NUIOA */

	for(i=0; i<count; i++) {
		int err;

		err = nm_core_driver_load(p_core, driver_array[i], &id);
		if (err != NM_ESUCCESS) {
			NM_DISPF("nm_core_driver_load returned %d", err);
			return err;
		}

		p_id_array[i] = id;

		err = nm_core_driver_query(p_core, id, params_array[i], nparam_array[i]);
		if (err != NM_ESUCCESS) { 
			NM_DISPF("nm_core_driver_query returned %d", err);
			return err;
		}

#ifdef PM2_NUIOA
		if (nuioa) {
			struct nm_drv *p_drv = p_core->driver_array + id;
			int node = p_drv->driver->get_capabilities(p_drv)->numa_node;
			if (node != PM2_NUIOA_ANY_NODE) {
				/* if this driver wants something */
				DISP("marking nuioa node %d as preferred for driver %d", node, id);

				if (nuioa_with_latency) {
					/* choosing by latency: take this network if it's the first one
					 * or if its latency is lower than the previous one */
					if (preferred_node == PM2_NUIOA_ANY_NODE
					    || p_drv->driver->get_capabilities(p_drv)->latency < nuioa_current_best) {
						preferred_node = node;
						nuioa_current_best = p_drv->driver->get_capabilities(p_drv)->latency;
					}

				} else if (nuioa_with_bandwidth) {
					/* choosing by bandwidth: take this network if it's the first one
					 * or if its bandwidth is higher than the previous one */
					if (preferred_node == PM2_NUIOA_ANY_NODE
					    || p_drv->driver->get_capabilities(p_drv)->bandwidth > nuioa_current_best) {
						preferred_node = node;
						nuioa_current_best = p_drv->driver->get_capabilities(p_drv)->bandwidth;
					}

				} else if (preferred_node == PM2_NUIOA_ANY_NODE) {
					/* if it's the first driver, take its preference for now */
					preferred_node = node;

				} else if (preferred_node != node) {
					/* if the first driver wants something else, it's a conflict,
					 * display a message once */
					if (preferred_node != PM2_NUIOA_CONFLICTING_NODES)
						DISP("found conflicts between preferred nuioa nodes of drivers");
					preferred_node = PM2_NUIOA_CONFLICTING_NODES;
				}
			}
		}
#endif /* PM2_NUIOA */
	}

#ifdef PM2_NUIOA
	if (nuioa && preferred_node != PM2_NUIOA_ANY_NODE && preferred_node != PM2_NUIOA_CONFLICTING_NODES) {
		nodemask_t mask;
		nodemask_zero(&mask);
		DISP("binding to nuioa node %d", preferred_node);
		nodemask_set(&mask, preferred_node);
		numa_bind(&mask);
	}
#endif /* PM2_NUIOA */

	for(i=0; i<count; i++) {
		int err;

		err = nm_core_driver_init(p_core, p_id_array[i], &p_url_array[i]);
		if (err != NM_ESUCCESS) {
			NM_DISPF("nm_core_driver_init returned %d", err);
			return err;
		}

#ifndef CONFIG_PROTO_MAD3
		printf("driver #%d url: [%s]\n", i, p_url_array[i]);
#endif
	}
	return NM_ESUCCESS;
}

/** Shutdown a driver.
 *
 */
int
nm_core_driver_exit(struct nm_core  *p_core) {
  struct nm_drv	     *p_drv    = NULL;
  struct nm_sched    *p_sched  = NULL;
  struct nm_gate     *p_gate   = NULL;
  struct nm_gate_drv *p_gdrv	= NULL;
  int i, j, k, err = NM_ESUCCESS;

#ifdef PIOMAN
  for(i=0;i<p_core->nb_drivers;i++)
	piom_server_stop(&p_core->driver_array[i].server);
#endif

  p_sched = p_core->p_sched;
  for(i=0 ; i<p_core->nb_gates ; i++) {
    p_gate = p_core->gate_array + i;

    for(j=0 ; j<NUMBER_OF_DRIVERS ; j++) {
      if (p_gate->p_gate_drv_array[j] != NULL) {
	p_gdrv = p_gate->p_gate_drv_array[j];
        p_drv = p_gdrv->p_drv;
        for(k=0 ; k<p_drv->nb_tracks ; k++) {
          struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[k];
          struct nm_trk *p_trk = p_drv->p_track_array[k];
          struct nm_cnx_rq	 rq	= {
                .p_gate			= p_gate,
                .p_drv			= p_drv,
                .p_trk			= p_trk,
                .remote_drv_url		= NULL,
                .remote_trk_url		= NULL
            };
          p_gdrv->receptacle.driver->disconnect(p_gdrv->receptacle._status, &rq);
          TBX_FREE(p_gtrk);
          p_gdrv->p_gate_trk_array[k] = NULL;
        }
        TBX_FREE(p_gdrv->p_gate_trk_array);
        p_gdrv->p_gate_trk_array = NULL;
      }
    }
  }

  for(i=0 ; i<p_core->nb_gates ; i++) {
    p_gate = p_core->gate_array + i;

    for(j=0 ; j<NUMBER_OF_DRIVERS ; j++) {
      if (p_gate->p_gate_drv_array[j] != NULL) {
        p_gdrv = p_gate->p_gate_drv_array[j];
        p_drv = p_gdrv->p_drv;

        if (p_drv->nb_tracks != 0) {
          err	= p_sched->ops.close_trks(p_sched, p_drv);
          if (err != NM_ESUCCESS) {
            NM_DISPF("close.trks returned %d", err);
            return err;
          }
        }

        TBX_FREE(p_drv->p_track_array);
        p_drv->p_track_array = NULL;
        p_drv->nb_tracks = 0;
	
	// Faire destroy sur chaque instance
	if(p_gdrv->instance != NULL)
	  {
	    puk_instance_destroy(p_gdrv->instance);
	    p_gdrv->instance = NULL;
	  }
	
        TBX_FREE(p_gdrv);
        p_gate->p_gate_drv_array[j] = NULL;
      }
    }
  }

  for(i=0 ; i<p_core->nb_gates ; i++) {
    p_gate = p_core->gate_array + i;

    err = p_sched->ops.close_gate(p_sched, p_gate);
    if (err != NM_ESUCCESS) {
      NM_DISPF("gate.exit returned %d", err);
      return err;
    }

    tbx_slist_free(p_gate->post_sched_out_list);
  }

  for(i=0 ; i<p_core->nb_drivers ; i++) {
    p_drv = p_core->driver_array + i;
  }

  return err;
}

/** Initialize a new gate.
 *
 * out parameters:
 * p_id - id of the gate
 */
int
nm_core_gate_init(struct nm_core	*p_core,
                  nm_gate_id_t		*p_id) {
        struct nm_gate	*p_gate		= NULL;
        int err;
#ifdef PIOMAN
	piom_server_stop(&p_core->server);
#endif
        err = NM_ESUCCESS;

        if (p_core->nb_gates == NUMBER_OF_GATES) {
                err	= -NM_ENOMEM;
                goto out;
        }

        p_gate	= p_core->gate_array + p_core->nb_gates;

        memset(p_gate, 0, sizeof(struct nm_gate));

        p_gate->id	= p_core->nb_gates;
        p_gate->p_core	= p_core;
        p_gate->p_sched	= p_core->p_sched;
        p_gate->post_sched_out_list	= tbx_slist_nil();

        p_core->nb_gates++;

	FUT_DO_PROBE1(FUT_NMAD_INIT_GATE, p_gate->id);

        err = p_core->p_sched->ops.init_gate(p_core->p_sched, p_gate);
        if (err != NM_ESUCCESS) {
                NM_DISPF("sched.init_gate returned %d", err);
                goto out;
        }

        if (p_id) {
                *p_id	= p_gate->id;
        }

#ifdef PIOMAN
	int i;
	for(i=0;i<p_core->nb_drivers;i++)
		nm_core_init_piom_drv(p_core,& p_core->driver_array[i]);

	nm_core_init_piom(p_core);
#endif

  out:
        return err;
}

/** Connect the process through a gate using a specified driver.
 */
static
int
nm_core_gate_connect_accept(struct nm_core	*p_core,
                            nm_gate_id_t	 gate_id,
                            uint8_t		 drv_id,
                            const char		*drv_trk_url,
                            int			 connect_flag) {
        struct nm_cnx_rq	 rq	= {
                .p_gate			= NULL,
                .p_drv			= NULL,
                .p_trk			= NULL,
                .remote_drv_url		= NULL,
                .remote_trk_url		= NULL
        };

        struct nm_gate_drv	*p_gdrv	= TBX_MALLOC(sizeof(struct nm_gate_drv));

        char	*urls[255];
        int i;
        int err;

        /* check gate arg
         */
        if (gate_id >= p_core->nb_gates) {
                err = -NM_EINVAL;
                goto out;
        }

        rq.p_gate	= p_core->gate_array + gate_id;

        if (!rq.p_gate) {
                err = -NM_EINVAL;
                goto out;
        }

        /* check drv arg
         */
        if (drv_id >= p_core->nb_drivers) {
                err = -NM_EINVAL;
                goto out;
        }

        rq.p_drv	= p_core->driver_array + drv_id;

        /* allocate gate/driver mem
         */
	p_gdrv->instance = puk_adapter_instanciate(rq.p_drv->assembly);
	puk_instance_indirect_NewMad_Driver(p_gdrv->instance, NULL, &p_gdrv->receptacle);

        /* init gate/driver fields
         */
        p_gdrv->p_drv	= rq.p_drv;
        p_gdrv->p_gate_trk_array	=
                TBX_MALLOC(rq.p_drv->nb_tracks * sizeof(struct nm_gate_trk	*));

        if (!p_gdrv->p_gate_trk_array) {
                err	= -NM_ENOMEM;
                goto out;
        }

	/* store gate/driver struct
         */
        rq.p_gate->p_gate_drv_array[drv_id]	 = p_gdrv;

        /* init track related gate/driver sub-fields
         */
        for (i = 0; i < rq.p_drv->nb_tracks; i++) {
                struct nm_gate_trk	*p_gtrk	= NULL;

                p_gtrk	= TBX_MALLOC(sizeof(struct nm_gate_trk));
                memset(p_gtrk, 0, sizeof(struct nm_gate_trk));

                if (!p_gtrk) {
                        err	= -NM_ENOMEM;
                        goto out;
                }

                p_gtrk->p_gdrv	= p_gdrv;
                p_gtrk->p_trk	= rq.p_drv->p_track_array[i];

                p_gdrv->p_gate_trk_array[i]	= p_gtrk;
        }

        /* split drv/trk url
         */
        if (drv_trk_url) {
                rq.remote_drv_url	= tbx_strdup(drv_trk_url);
                {
                        char *ptr = rq.remote_drv_url;

                        for (i = 0 ; ptr && i < 255; i++) {
                                ptr = strchr(ptr, '#');

                                if (ptr) {
                                        *ptr	= '\0';
                                        urls[i]	= ++ptr;
                                }
                        }
                }
        } else {
                rq.remote_drv_url	= NULL;
                memset(urls, 0, 255*sizeof(NULL));
        }

        /* connect/accept connections
         */
        for (i = 0; i < rq.p_drv->nb_tracks; i++) {
                rq.p_trk		= rq.p_drv->p_track_array[i];
                rq.remote_trk_url	= urls[i];

                if (connect_flag) {
		        err = p_gdrv->receptacle.driver->connect(p_gdrv->receptacle._status, &rq);
                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv.ops.connect returned %d", err);
                                goto out_free;
                        }
                } else {
		        err = p_gdrv->receptacle.driver->accept(p_gdrv->receptacle._status, &rq);
                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv.ops.accept returned %d", err);
                                goto out_free;
                        }
                }

        }

        err = NM_ESUCCESS;
        goto out_free;

 out:
        return err;

 out_free:
        if (rq.remote_drv_url) {
                TBX_FREE(rq.remote_drv_url);
        }
        goto out;

}

/** Server side of connection establishment.
 */
int
nm_core_gate_accept	(struct nm_core	*p_core,
                         nm_gate_id_t	 gate_id,
                         uint8_t	 drv_id,
                         const char	*drv_trk_url) {
        return nm_core_gate_connect_accept(p_core, gate_id, drv_id,
                                           drv_trk_url, 0);
}

/** Client side of connection establishment.
 */
int
nm_core_gate_connect	(struct nm_core	*p_core,
                         nm_gate_id_t	 gate_id,
                         uint8_t	 drv_id,
                         const char	*drv_trk_url) {
        return nm_core_gate_connect_accept(p_core, gate_id, drv_id,
                                           drv_trk_url, !0);
}

/** Public function to wrap a single buffer.
 *
 * Mostly for debugging purpose. The wrapping is usually done by
 * internal code that has at least access to protected methods
 */
int
nm_core_wrap_buffer	(struct nm_core		 *p_core,
                         nm_gate_id_t		  gate_id,
                         uint8_t		  proto_id,
                         uint8_t		  seq,
                         void			 *buf,
                         uint64_t		  len,
                         struct nm_pkt_wrap	**pp_pw) {
        return __nm_core_wrap_buffer(p_core, gate_id, proto_id, seq, buf, len, pp_pw);
}


/** Load a newmad component from disk. The actual path loaded is
 * ${PM2_CONF_DIR}/'entity'/'entity'_'driver'.xml
 */
puk_component_t
nm_core_component_load(const char*entity, const char*name){
  char filename[1024];
  int rc = 0;
  const char*pm2_conf_dir = getenv("PM2_CONF_DIR");
  if(pm2_conf_dir) {
      rc = snprintf(filename, 1024, "%s/%s/%s_%s.xml", pm2_conf_dir, entity, entity, name);
  } else {
    const char*home = getenv("PM2_HOME");
    if (!home) {
      home = getenv("HOME");
    }
    assert(home != NULL);
    rc = snprintf(filename, 1024, "%s/.pm2/%s/%s_%s.xml", home, entity, entity, name);
  }
  assert(rc < 1024);
  puk_component_t component = puk_adapter_parse_file(filename);
  assert(component != NULL);
  return component;
}



/** Initialize the core struct and the main scheduler.

   - sched_load is function that should initialize the
   sched_ops struct it receives with the implementation
   dependent scheduler methods
   - the scheduler is subsequently initialized by a call
   to sched_ops.init
 */
int
nm_core_init		(int			 *argc,
                         char			 *argv[],
                         struct nm_core		**pp_core,
                         int (*sched_load)(struct nm_sched_ops *)) {
        struct nm_core *p_core	= NULL;
        int err = NM_ESUCCESS;

	/*
	 * Lazy Puk initialization (it may already have been initialized in PadicoTM or MPI_Init)
	 */
	if(!padico_puk_initialized())
	  {
	    padico_puk_init(*argc, argv);
	  }

	/*
	 * Lazy PM2 initialization (it may already have been initialized in PadicoTM)
	 */
	if(!tbx_initialized())
	  {
	    common_pre_init(argc, argv, NULL);
	    common_post_init(argc, argv, NULL);
	  }

	FUT_DO_PROBE0(FUT_NMAD_INIT_CORE);

        p_core	= TBX_MALLOC(sizeof(struct nm_core));

        p_core->nb_gates	= 0;
        p_core->nb_drivers	= 0;
        p_core->p_sched		= NULL;

        tbx_malloc_init(&nm_core_pw_mem,   sizeof(struct nm_pkt_wrap),
                        INITIAL_PW_NUM,   "nmad/pw");
        tbx_malloc_init(&nm_core_iov1_mem,   sizeof(struct iovec),
                        INITIAL_IOV1_NUM, "nmad/iov1");
        tbx_malloc_init(&nm_core_iov2_mem, 2*sizeof(struct iovec),
                        INITIAL_IOV2_NUM, "nmad/iov2");

        err = nm_core_schedule_init(p_core, sched_load);
        if (err != NM_ESUCCESS)
                goto out_free;

        *pp_core	= p_core;

	nm_core_drivers_load();

        err = NM_ESUCCESS;

 out:
        return err;

 out_free:
        TBX_FREE(p_core);
        goto out;
}

/** Shutdown the core struct and the main scheduler.
 */
int
nm_core_exit           (struct nm_core		*p_core) {
  nm_core_schedule_exit(p_core);
  TBX_FREE(p_core);

  tbx_malloc_clean(nm_core_pw_mem);
  tbx_malloc_clean(nm_core_iov1_mem);
  tbx_malloc_clean(nm_core_iov2_mem);

  return NM_ESUCCESS;
}

