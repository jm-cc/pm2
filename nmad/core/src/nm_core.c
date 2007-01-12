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

#include <pm2_common.h>

#include "nm_private.h"

#include <nm_public.h>

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

/* allocate a new track.
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

        if (p_drv->nb_tracks == 255) {
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

/* clean up a track.
 *
 * not yet implemented
 */
int
nm_core_trk_free(struct nm_core		*p_core,
                 struct nm_trk		*p_trk) {
        struct nm_drv		 *p_drv;
	int	err;

        p_drv = p_trk->p_drv;
        err = p_drv->ops.close_trk(p_trk);

        return err;
}

/* load and initialize a scheduler.
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

/* load and initialize a protocol.
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

/* load and initialize a driver.
 *
 * Out parameters:
 * p_id  - contains the id of the new driver
 * p_url - contains the URL of the driver (memory for the URL is allocated by
 * nm_core)
 */
int
nm_core_driver_init(struct nm_core	 *p_core,
                    int (*drv_load)(struct nm_drv_ops *),
                    uint8_t		 *p_id,
                    char		**p_url) {
        struct nm_drv	*p_drv		= NULL;
        struct nm_sched	*p_sched	= NULL;
        p_tbx_string_t	 url		= NULL;
        int err;

        NM_LOG_IN();
        if (p_core->nb_drivers == 255) {
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

        err = p_drv->ops.init(p_drv);
        if (err != NM_ESUCCESS) {
                NM_DISPF("drv.init returned %d", err);
                goto out;
        }

        if (p_id) {
                *p_id	= p_drv->id;
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

        /* TODO: build full URL with drv and trks URLs
         */
#warning TODO
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

/* shutdown a driver.
 *
 */
int
nm_core_driver_exit(struct nm_core  *p_core) {
  struct nm_drv	  *p_drv    = NULL;
  struct nm_sched *p_sched  = NULL;
  int i, err;

  p_sched = p_core->p_sched;
  for(i=0 ; i<p_core->nb_drivers ; i++) {
    p_drv = p_core->driver_array + i;
    err	= p_sched->ops.close_trks(p_sched, p_drv);
    if (err != NM_ESUCCESS) {
      NM_DISPF("drv.exit returned %d", err);
      return err;
    }
  }
  return err;
}

/* initialize a new gate.
 *
 * out parameters:
 * p_id - id of the gate
 */
int
nm_core_gate_init(struct nm_core	*p_core,
                  uint8_t		*p_id) {
        struct nm_gate	*p_gate		= NULL;
        int err;

        err = NM_ESUCCESS;

        if (p_core->nb_gates == 255) {
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

        /* TODO: initialise gate_drv_array
         */
#warning TODO

        p_core->nb_gates++;

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

/* connect the process through a gate using a specified driver
 */
static
int
nm_core_gate_connect_accept(struct nm_core	*p_core,
                            uint8_t		 gate_id,
                            uint8_t		 drv_id,
                            char		*host_url,
                            char		*drv_trk_url,
                            int			 connect_flag) {
        struct nm_cnx_rq	 rq	= {
                .p_gate			= NULL,
                .p_drv			= NULL,
                .p_trk			= NULL,
                .remote_host_url	= host_url,
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
                /* TODO: free already allocated structs
                 */
#warning TODO
                err	= -NM_ENOMEM;
                goto out;
        }

        /* init gate/driver fields
         */
        p_gdrv->p_drv	= rq.p_drv;
        p_gdrv->p_gate_trk_array	=
                TBX_MALLOC(rq.p_drv->nb_tracks * sizeof(struct nm_gate_trk	*));

        if (!p_gdrv->p_gate_trk_array) {
                /* TODO: free already allocated structs
                 */
#warning TODO
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
                        /* TODO: free already allocated structs
                         */
#warning TODO
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
#ifdef CONFIG_NET_SAMPLING
        err = nm_ns_network_sampling(rq.p_drv, gate_id, connect_flag);
        if (err != NM_ESUCCESS) {
                NM_DISPF("ns_network_sampling returned %d", err);
        }
#endif /* CONFIG_NET_SAMPLING */

        return err;

 out_free:
        if (rq.remote_drv_url) {
                TBX_FREE(rq.remote_drv_url);
        }
        goto out;

}

/* server side of connection establishment.
 */
int
nm_core_gate_accept	(struct nm_core	*p_core,
                         uint8_t	 gate_id,
                         uint8_t	 drv_id,
                         char		*host_url,
                         char		*drv_trk_url) {
        return nm_core_gate_connect_accept(p_core, gate_id, drv_id,
                                           host_url, drv_trk_url,
                                           0);
}

/* client side of connection establishment.
 */
int
nm_core_gate_connect	(struct nm_core	*p_core,
                         uint8_t	 gate_id,
                         uint8_t	 drv_id,
                         char		*host_url,
                         char		*drv_trk_url) {
        return nm_core_gate_connect_accept(p_core, gate_id, drv_id,
                                           host_url, drv_trk_url,
                                           !0);
}

/* TODO: symmetrical connect
 */
#warning TODO

/* public function to wrap a single buffer.
 *
 * Mostly for debugging purpose. The wrapping is usually done by
 * internal code that has at least access to protected methods
 */
int
nm_core_wrap_buffer	(struct nm_core		 *p_core,
                         int16_t		  gate_id,
                         uint8_t		  proto_id,
                         uint8_t		  seq,
                         void			 *buf,
                         uint64_t		  len,
                         struct nm_pkt_wrap	**pp_pw) {
        return __nm_core_wrap_buffer(p_core, gate_id, proto_id, seq, buf, len, pp_pw);
}

/* public function to post a send request.
 *
 * Mostly for debugging purpose. The wrapping is usually done by
 * internal code that has at least access to protected methods
 */
int
nm_core_post_send	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw) {
        return __nm_core_post_send(p_core, p_pw);
}

/* public function to post a receive request.
 *
 * Mostly for debugging purpose. The wrapping is usually done by
 * internal code that has at least access to protected methods
 */
int
nm_core_post_recv	(struct nm_core		*p_core,
                         struct nm_pkt_wrap	*p_pw) {
        return __nm_core_post_recv(p_core, p_pw);
}

/* initialize the core struct and the main scheduler
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
