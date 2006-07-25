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

#include <tbx.h>

#include "nm_so_private.h"
#include "nm_so_strategies.h"
#include "nm_so_optimizer.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"

#include <nm_rdv_public.h>


/* schedule and post new outgoing buffers */
int
nm_so_out_schedule_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *so_gate = p_gate->sch_private;
  struct nm_gate_drv *p_gdrv = NULL;
  struct nm_drv      *p_drv  = NULL;
  struct nm_gate_trk *p_gtrk = NULL;
  struct nm_trk      *p_trk  = NULL;
  p_tbx_slist_t pre  = p_gate->pre_sched_out_list;
  p_tbx_slist_t post = p_gate->post_sched_out_list;
  struct nm_so_pkt_wrap *p_so_pw = NULL;
  int err;

    if(tbx_slist_is_nil(pre))
        goto out;

    p_gdrv  = p_gate->p_gate_drv_array[0];
    p_drv   = p_gdrv->p_drv;
    p_gtrk  = p_gdrv->p_gate_trk_array[0];
    p_trk   = p_gtrk->p_trk;

    /* already one pending request or something to send */
    if(p_gate->out_req_nb || !tbx_slist_is_nil(post))
        goto reload;

    if(so_gate->next_pw) {
        p_so_pw = so_gate->next_pw;
        so_gate->next_pw = NULL;

    } else {
        if(tbx_slist_is_nil(pre)){
            err = -NM_ENOTFOUND;
            goto out;
        }

        err = nm_so_optimizer_schedule_out(p_gate, p_drv, pre,
                                           &p_so_pw);
        nm_so_control_error("nm_so_optimizer_schedule_out", err);
        if(err == -NM_ENOTFOUND){
            goto out;
        }

	/* Add packet on track 0 (small)*/
        err = nm_so_pw_config (p_so_pw, p_drv, p_trk,
                               p_gate, p_gdrv, p_gtrk);
        nm_so_control_error("nm_so_pw_config", err);

    }



    if (p_trk->cap.rq_type == nm_trk_rq_dgram
        || p_trk->cap.rq_type == nm_trk_rq_stream) {

        struct nm_pkt_wrap *p_pw = NULL;
        err = nm_so_pw_get_pw(p_so_pw, &p_pw);

        /* append pkt to scheduler post list */
        tbx_slist_append(post, p_pw);


    } else {
      TBX_FAILURE("Track request type is invalid");
    }

 reload:
    //a-t-on un next?
    if(!tbx_slist_is_nil(pre) && !so_gate->next_pw){
        err = nm_so_optimizer_schedule_out(p_gate, p_drv, pre,
                                           &p_so_pw);
        nm_so_control_error("nm_so_optimizer_schedule_out", err);
        if(err == -NM_ENOTFOUND){
            goto out;
        }

	/* je mets le wrap sur la piste des petits (la n°0)*/
        err = nm_so_pw_config (p_so_pw, p_drv, p_trk,
                               p_gate, p_gdrv, p_gtrk);
        nm_so_control_error("nm_so_pw_config", err);
    }

 out:
    err = NM_ESUCCESS;
    return err;
}


static int
nm_so_free_aggregated_pw(struct nm_core *p_core,
                         struct nm_so_pkt_wrap *p_so_pw){

    struct nm_so_pkt_wrap **aggregated_pws = NULL;
    int nb_aggregated_pws = 0;

    struct nm_so_pkt_wrap *cur_so_pw = NULL;
    struct nm_pkt_wrap *p_pw = NULL;
    struct nm_proto *p_proto = NULL;
    uint8_t proto_id = 0;

    int err;
    int i;


    err = nm_so_pw_get_aggregated_pws(p_so_pw, aggregated_pws);
    nm_so_control_error("nm_so_pw_get_aggregated_pws", err);
    err = nm_so_pw_get_nb_aggregated_pws(p_so_pw, &nb_aggregated_pws);
    nm_so_control_error("nm_so_pw_get_nb_aggregated_pws", err);



    for(i = 0; i < nb_aggregated_pws; i++){
        cur_so_pw = aggregated_pws[i];

        err = nm_so_pw_get_proto_id(cur_so_pw, &proto_id);
        nm_so_control_error("nm_so_pw_get_proto_id", err);

        p_proto	= p_core->p_proto_array[proto_id];
        if (!p_proto) {
            NM_TRACEF("unregistered protocol: %d", proto_id);
            continue;
        }
        if (!p_proto->ops.out_success)
            continue;


        err = nm_so_pw_get_pw(cur_so_pw, &p_pw);
        nm_so_control_error("nm_so_pw_get_pw", err);

        err = p_proto->ops.out_success(p_proto, p_pw);
        nm_so_control_error("proto.ops.out_success", err);

        err = nm_so_pw_free(p_core, cur_so_pw);
        nm_so_control_error("nm_so_pw_free", err);
    }

    err = NM_ESUCCESS;
    return err;
}


/* process complete successful outgoing request */
int
nm_so_out_process_success_rq(struct nm_sched *p_sched,
                             struct nm_pkt_wrap	*p_pw) {
    struct nm_core     *p_core   = p_sched->p_core;
    struct nm_proto    *p_proto  = NULL;
    struct nm_proto    *p_proto_rdv  = NULL;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    struct nm_so_pkt_wrap *p_so_pw = p_pw->sched_priv;

    void *data = NULL;
    struct nm_so_header  *header = NULL;
    struct nm_rdv_rdv_rq *rdv = NULL;
    struct nm_rdv_ack_rq *ack = NULL;

    tbx_bool_t is_small = tbx_false;

    uint8_t v_nb = 0;
    uint8_t proto_id = 0;
    int idx  = 0;
    int err;

    p_proto_rdv = p_core->p_proto_array[nm_pi_rdv_ack];
    if (!p_proto_rdv) {
        NM_TRACEF("unregistered protocol: %d",
                  p_proto->id);
    }


    err = nm_so_pw_get_v_nb(p_so_pw, &v_nb);
    nm_so_control_error("nm_so_pw_get_v_nb", err);

    err = nm_so_pw_is_small(p_so_pw, &is_small);
    nm_so_control_error("nm_so_pw_is_small", err);

    if(is_small){

        idx++;
        v_nb--;

        while(v_nb > 0){
            err = nm_so_pw_get_data(p_so_pw, idx, &data);

            header = data;

            err = nm_so_header_get_proto_id(header, &proto_id);

            if(proto_id == nm_pi_rdv_req){
                //printf("--------------->Envoi d'un rdv\n");

                if(!p_proto_rdv)
                    continue;

                idx++;

                err = nm_so_pw_get_data(p_so_pw, idx, &data);
                rdv = data;

                err = nm_rdv_free_rdv(p_proto, rdv);
                nm_so_control_error("nm_rdv_free_rdv", err);


                err = nm_so_header_free_header(so_sched, header);
                nm_so_control_error("nm_so_header_free_header", err);
                idx++;
                v_nb-=2;



            } else if(proto_id == nm_pi_rdv_ack){
                //printf("------------->Envoi d'un ack\n");
                if (!p_proto_rdv)
                    continue;

                idx++;

                err = nm_so_pw_get_data(p_so_pw, idx, &data);
                ack = data;

                err = nm_rdv_free_ack(p_proto, ack);
                nm_so_control_error("nm_rdv_free_ack", err);

                err = nm_so_header_free_header(so_sched, header);
                nm_so_control_error("nm_so_header_free_header", err);
                idx++;
                v_nb-=2;


            } else if(proto_id >= 127){

                err = nm_so_header_free_header(so_sched, header);
                nm_so_control_error("nm_so_header_free_header", err);
                idx+= 2;
                v_nb-=2;

            }
        }

        err = nm_so_free_aggregated_pw(p_core, p_so_pw);
        nm_so_control_error("nm_so_free_aggregated_pw", err);

        err = nm_so_pw_release_aggregation_pw(p_sched, p_so_pw);
        nm_so_control_error("nm_so_pw_release_aggregation_pw", err);




    } else {
        //printf("----------------LARGE PACK ENVOYé - p_pw = %p\n\n", p_pw);
        err = nm_so_header_get_proto_id(header, &proto_id);

        p_proto	= p_core->p_proto_array[proto_id];
        if (!p_proto) {
            NM_TRACEF("unregistered protocol: %d", p_proto->id);
            goto free;
        }
        if (!p_proto->ops.out_success)
            goto free;

        err = p_proto->ops.out_success(p_proto, p_pw);
        nm_so_control_error("proto.ops.out_success", err);

    free:
        err = nm_so_pw_free(p_core, p_so_pw);
        nm_so_control_error("nm_so_pw_free", err);
    }

    err	= NM_ESUCCESS;
    return err;
}

/* process complete failed outgoing request */
int
nm_so_out_process_failed_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err)
{
    TBX_FAILURE("nm_so_out_process_failed_rq");
    return nm_so_out_process_success_rq(p_sched,p_pw);
}
