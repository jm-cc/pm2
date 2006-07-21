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

#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"
#include "nm_so_strategies/nm_so_strat_aggreg.h"

#include <nm_rdv_public.h>

//=======================================================================
//#define OPTIMISE

#ifdef OPTIMISE
static int
nm_so_search_next(struct nm_gate *p_gate,
                  struct nm_pkt_wrap **pp_pw){
    struct nm_core *p_core = p_gate->p_core;
    p_tbx_slist_t   pre    = p_gate->pre_sched_out_list;

    struct nm_pkt_wrap *p_pw = NULL;
    int pw_len =  sizeof(struct nm_so_sched_header);
    struct nm_pkt_wrap *cur_pw = NULL;
    int idx = 0;
    tbx_bool_t forward = tbx_false;

    struct nm_so_pkt_wrap *so_pw = NULL;
    struct nm_pkt_wrap **aggregated_pws = NULL;

    int pre_len = pre->length;
    int err;

    assert(pre_len);

    tbx_slist_ref_to_head(pre);
    do{
        cur_pw = tbx_slist_ref_get(pre);

        // envoi direct
        if(cur_pw->length < SMALL_THRESHOLD){
            if(cur_pw->length + pw_len < AGGREGATED_PW_MAX_SIZE){
                forward = tbx_slist_ref_extract_and_forward(pre, NULL);

                if(!p_pw){
                    err = nm_so_take_aggregation_pw(p_gate->p_sched, &p_pw);
                    nm_so_control_error("nm_so_take_aggregation_pw", err);

                    so_pw = p_pw->sched_priv;
                    aggregated_pws = so_pw->aggregated_pws;
                    //printf("-----------Construction du pack à  envoyer-----------\n");
                }
                //printf("----------->Ajout de données\n");
                err = nm_so_add_data(p_core, p_pw,
                                     cur_pw->proto_id,
                                     cur_pw->length,
                                     cur_pw->seq,
                                     cur_pw->v[0].iov_base);
                nm_so_control_error("nm_so_add_data", err);

                aggregated_pws[so_pw->nb_aggregated_pws++] = cur_pw;
                pw_len += cur_pw->length;

            } else {
                break;
            }

        // envoi par rdv
        } else if(pw_len
                  + sizeof(struct nm_so_header)
                  + nm_rdv_rdv_rq_size()
                  < AGGREGATED_PW_MAX_SIZE) {

            forward = tbx_slist_ref_extract_and_forward(pre, NULL);
            if(!p_pw){
                err = nm_so_take_aggregation_pw(p_gate->p_sched, &p_pw);
                nm_so_control_error("nm_so_take_aggregation_pw", err);

                so_pw = p_pw->sched_priv;
                aggregated_pws = so_pw->aggregated_pws;
                //printf("-----------Construction du pack-----------\n");
            }

            struct nm_proto *p_proto
                = p_core->p_proto_array[nm_pi_rdv_ack];
            struct nm_rdv_rdv_rq *p_rdv = NULL;

            err = nm_rdv_generate_rdv(p_proto, cur_pw, &p_rdv);
            nm_so_control_error("nm_rdv_generate_rdv", err);

            struct nm_so_sched *so_sched
                = p_gate->p_sched->sch_private;

            //printf("--->ajout du rdv du scheduler\n");
            struct nm_so_header *so_rdv
                = tbx_malloc(so_sched->header_key);
            if(!so_rdv){
                err = -NM_ENOMEM;
                goto out;
            }


            so_rdv->proto_id = nm_pi_rdv_req;
            err = nm_iov_append_buf(p_core, p_pw, so_rdv,
                                    sizeof(struct nm_so_header));
            nm_so_control_error("nm_iov_append_buf", err);

            ///printf("--->ajout du rdv du protocole\n");
            err = nm_iov_append_buf(p_core, p_pw, p_rdv,
                                    nm_rdv_rdv_rq_size());
            nm_so_control_error("nm_iov_append_buf", err);

            pw_len += sizeof(struct nm_so_header) +  nm_rdv_rdv_rq_size();


        } else { // le pw courant ne rentre pas
            break; //forward = tbx_slist_ref_forward(pre);
        }

        idx++;
        if(!forward)
            break;
    } while(tbx_slist_ref_forward(pre));


    if(p_pw){
        err = nm_so_update_global_header(p_pw, p_pw->v_nb, p_pw->length);
        nm_so_control_error("nm_so_update_global_header", err);

        /* je mets le wrap sur la piste des petits (la n°0)*/
        struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
        struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[0];
        if(p_gtrk == NULL)
            TBX_FAILURE("p_gtrk NULL");

        p_pw->p_gate = p_gate;
        p_pw->p_drv  = p_gdrv->p_drv;
        p_pw->p_trk  = p_gtrk->p_trk;
        p_pw->p_gdrv = p_gdrv;
        p_pw->p_gtrk = p_gtrk;
    }

    *pp_pw = p_pw;
    err = NM_ESUCCESS;

 out:
     return err;
}

#else // On prend le premier dans la liste

static int
nm_so_search_next(struct nm_gate *p_gate,
                  struct nm_pkt_wrap **pp_pw){
    struct nm_core *p_core   = p_gate->p_core;
    p_tbx_slist_t pre  = p_gate->pre_sched_out_list;

    struct nm_pkt_wrap *p_pw = NULL;
    struct nm_pkt_wrap *cur_pw = NULL;

    struct nm_so_pkt_wrap *so_pw = NULL;


    int pre_len = pre->length;
    int err;

    if(!pre_len)
        goto end;

    err = nm_so_take_aggregation_pw(p_gate->p_sched, &p_pw);
    nm_so_control_error("nm_so_take_aggregation_pw", err);

    so_pw = p_pw->sched_priv;

    //tbx_slist_ref_to_head(pre);
    //tbx_slist_ref_extract_and_forward(pre, (void *)&cur_pw);

    cur_pw = tbx_slist_remove_from_head(pre);

    if(cur_pw->length < SMALL_THRESHOLD){

        err = nm_so_add_data(p_core, p_pw,
                             cur_pw->proto_id, cur_pw->length,
                             cur_pw->seq, cur_pw->v[0].iov_base);
        nm_so_control_error("nm_so_add_data", err);

        so_pw->aggregated_pws[so_pw->nb_aggregated_pws++] = cur_pw;

    } else {
        struct nm_proto *p_proto
            = p_core->p_proto_array[nm_pi_rdv_ack];
        struct nm_rdv_rdv_rq *p_rdv = NULL;

        err = nm_rdv_generate_rdv(p_proto, cur_pw, &p_rdv);
        nm_so_control_error("nm_rdv_generate_rdv", err);

        struct nm_so_sched *so_sched
            = p_gate->p_sched->sch_private;

        struct nm_so_header *so_rdv
            = tbx_malloc(so_sched->header_key);
        if(!so_rdv){
            err = -NM_ENOMEM;
            goto end;
        }

        so_rdv->proto_id = nm_pi_rdv_req;

        err = nm_iov_append_buf(p_core, p_pw, so_rdv,
                                sizeof(struct nm_so_header));
        nm_so_control_error("nm_iov_append_buf", err);

        err = nm_iov_append_buf(p_core, p_pw, p_rdv,
                                nm_rdv_rdv_rq_size());
        nm_so_control_error("nm_iov_append_buf", err);

    }

    err = nm_so_update_global_header(p_pw, p_pw->v_nb,
                                     p_pw->length);
    nm_so_control_error("nm_so_update_global_header", err);

    /* je mets le wrap sur la piste des petits (la n°0)*/
    struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[0];
    if(p_gtrk == NULL)
        TBX_FAILURE("p_gtrk NULL");

    p_pw->p_gate = p_gate;
    p_pw->p_drv  = p_gdrv->p_drv;
    p_pw->p_trk  = p_gtrk->p_trk;
    p_pw->p_gdrv = p_gdrv;
    p_pw->p_gtrk = p_gtrk;

    *pp_pw = p_pw;
    err = NM_ESUCCESS;

 end:
    return err;
}
#endif

//=======================================================================

// Compute and apply the best possible packet rearrangement, 
// then return next packet to send
static int try_and_commit(nm_so_strategy *strat,
			  struct nm_gate *p_gate,
			  struct nm_drv *driver,
			  p_tbx_slist_t pre_list,
			  struct nm_pkt_wrap **pp_pw)
{
  return nm_so_search_next(p_gate, pp_pw);
}

// Initialization
static int init(nm_so_strategy *strat)
{
  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_aggreg = {
  .init = init,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .priv = NULL,
};
