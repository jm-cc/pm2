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
#include "nm_trk.h"
#include "nm_trk_rq.h"
#include "nm_sched.h"
#include "nm_proto.h"
#include "nm_cnx_rq.h"
#include <nm_public.h>
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

        err = p_drv->ops.open_trk(p_trk_rq);
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
        err = p_drv->ops.close_trk(p_trk);

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
        p_sched->submit_aux_recv_req	= tbx_slist_nil();
        p_sched->pending_perm_recv_req	= tbx_slist_nil();
        p_sched->post_perm_recv_req	= tbx_slist_nil();
        p_sched->submit_perm_recv_req	= tbx_slist_nil();

        err = p_sched->ops.init(p_sched);
        if (err != NM_ESUCCESS)
                goto out_free_lists;

        err = NM_ESUCCESS;

 out:
        return err;

 out_free_lists:
        tbx_slist_free(p_sched->pending_aux_recv_req);
        tbx_slist_free(p_sched->post_aux_recv_req);
        tbx_slist_free(p_sched->submit_aux_recv_req);
        tbx_slist_free(p_sched->pending_perm_recv_req);
        tbx_slist_free(p_sched->post_perm_recv_req);
        tbx_slist_free(p_sched->submit_perm_recv_req);

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
        tbx_slist_clear(p_sched->submit_aux_recv_req);
        tbx_slist_clear(p_sched->pending_perm_recv_req);
        tbx_slist_clear(p_sched->post_perm_recv_req);
        tbx_slist_clear(p_sched->submit_perm_recv_req);

        tbx_slist_free(p_sched->pending_aux_recv_req);
        tbx_slist_free(p_sched->post_aux_recv_req);
        tbx_slist_free(p_sched->submit_aux_recv_req);
        tbx_slist_free(p_sched->pending_perm_recv_req);
        tbx_slist_free(p_sched->post_perm_recv_req);
        tbx_slist_free(p_sched->submit_perm_recv_req);

        TBX_FREE(p_sched);
        p_core->p_sched = NULL;

        return err;
}


/** Load and initialize a protocol.
 *
 * out parameters:
 * pp_proto - structure representing the protocol (allocated by nm_core)
 */
int
nm_core_proto_init(struct nm_core	 *p_core,
                   int (*proto_load)(struct nm_proto_ops *),
                   struct nm_proto	**pp_proto) {
        struct nm_proto	*p_proto	= NULL;
        int err;

        p_proto	= TBX_MALLOC(sizeof(struct nm_proto));
        if (!p_proto) {
                err = -NM_ENOMEM;
                goto out;
        }

        p_proto->p_core	= p_core;
        p_proto->priv	= NULL;

        memset(&p_proto->ops, 0, sizeof(struct nm_proto_ops));
        memset(&p_proto->cap, 0, sizeof(struct nm_proto_cap));

        err = proto_load(&p_proto->ops);
        if (err != NM_ESUCCESS) {
                NM_DISPF("proto_load returned %d", err);
                goto out;
        }

        err = p_proto->ops.init(p_proto);
        if (err != NM_ESUCCESS) {
                NM_DISPF("proto.init returned %d", err);
                goto out;
        }

        *pp_proto	= p_proto;
        err = NM_ESUCCESS;

 out:
        return err;
}

/** Load a driver.
 *
 * Out parameters:
 * p_id  - contains the id of the new driver
 */
int
nm_core_driver_load(struct nm_core	 *p_core,
                    int (*drv_load)(struct nm_drv_ops *),
                    uint8_t *p_id) {
        struct nm_drv	*p_drv		= NULL;
        int err;

        NM_LOG_IN();
        if (p_core->nb_drivers == NUMBER_OF_DRIVERS) {
                err	= -NM_ENOMEM;
                goto out;
        }

        p_drv	= p_core->driver_array + p_core->nb_drivers;

        memset(p_drv, 0, sizeof(struct nm_drv));

        p_drv->id	= p_core->nb_drivers;
        p_drv->p_core	= p_core;

        p_core->nb_drivers++;

        err = drv_load(&p_drv->ops);
        if (err != NM_ESUCCESS) {
                NM_DISPF("drv_load returned %d", err);
                goto out;
        }

        if (p_id) {
                *p_id	= p_drv->id;
        }

	/*TODO : add url */
	FUT_DO_PROBE1(FUT_NMAD_INIT_NIC, p_drv->id);

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

	if (!p_drv->ops.query) {
		err = -NM_EINVAL;
                goto out;
        }

        err = p_drv->ops.query(p_drv, params, nparam);
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
        int err;

        NM_LOG_IN();
        p_drv	= p_core->driver_array + id;

	if (!p_drv->ops.init) {
		err = -NM_EINVAL;
                goto out;
        }

        err = p_drv->ops.init(p_drv);
        if (err != NM_ESUCCESS) {
                NM_DISPF("drv.init returned %d", err);
                goto out;
        }

        if (p_url) {
                *p_url	= p_drv->url;
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
                if (p_drv->url) {
                        url = tbx_string_init_to_cstring(p_drv->url);
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
					  int (**drv_load_array)(struct nm_drv_ops *),
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
#endif /* PM2_NUIOA */

	for(i=0; i<count; i++) {
		int err;

		err = nm_core_driver_load(p_core, drv_load_array[i], &id);
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
			int node = (p_core->driver_array + id)->cap.numa_node;
			if (node != PM2_NUIOA_ANY_NODE) {
				/* if this driver wants something */
				DISP("marking nuioa node %d as preferred for driver %d", node, id);

				if (preferred_node == PM2_NUIOA_ANY_NODE) {
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
          rq.p_drv->ops.disconnect(&rq);
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

    tbx_slist_free(p_gate->pre_sched_out_list);
    tbx_slist_free(p_gate->post_sched_out_list);
  }

  for(i=0 ; i<p_core->nb_drivers ; i++) {
    p_drv = p_core->driver_array + i;
    err = p_drv->ops.exit(p_drv);
    if (err != NM_ESUCCESS) {
      NM_DISPF("drv.exit returned %d", err);
      return err;
    }
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
        p_gate->pre_sched_out_list	= tbx_slist_nil();
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
                            char		*drv_trk_url,
                            int			 connect_flag) {
        struct nm_cnx_rq	 rq	= {
                .p_gate			= NULL,
                .p_drv			= NULL,
                .p_trk			= NULL,
                .remote_drv_url		= NULL,
                .remote_trk_url		= NULL
        };

        struct nm_gate_drv	*p_gdrv	= NULL;

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
        p_gdrv	= TBX_MALLOC(sizeof(struct nm_gate_drv));
        memset(p_gdrv, 0, sizeof(struct nm_gate_drv));

        if (!p_gdrv) {
                err	= -NM_ENOMEM;
                goto out;
        }

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
                        err = rq.p_drv->ops.connect(&rq);
                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv.ops.connect returned %d", err);
                                goto out_free;
                        }
                } else {
                        err = rq.p_drv->ops.accept(&rq);
                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv.ops.accept returned %d", err);
                                goto out_free;
                        }
                }

        }

        /* update the number of gates
         */
        rq.p_drv->nb_gates++;

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
                         char		*drv_trk_url) {
        return nm_core_gate_connect_accept(p_core, gate_id, drv_id,
                                           drv_trk_url, 0);
}

/** Client side of connection establishment.
 */
int
nm_core_gate_connect	(struct nm_core	*p_core,
                         nm_gate_id_t	 gate_id,
                         uint8_t	 drv_id,
                         char		*drv_trk_url) {
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

/** Public function to post a send request.
 *
 * Mostly for debugging purpose. The wrapping is usually done by
 * internal code that has at least access to protected methods
 */
int
nm_core_post_send	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw) {
        return __nm_core_post_send(p_core, p_pw);
}

/** Public function to post a receive request.
 *
 * Mostly for debugging purpose. The wrapping is usually done by
 * internal code that has at least access to protected methods
 */
int
nm_core_post_recv	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw) {
        return __nm_core_post_recv(p_core, p_pw);
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
        int err;

        common_pre_init(argc, argv, NULL);
        common_post_init(argc, argv, NULL);

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

