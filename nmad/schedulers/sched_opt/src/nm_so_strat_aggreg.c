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
#include "nm_so_pkt_wrap.h"

#include <nm_rdv_public.h>

//=======================================================================
//#define OPTIMISE

#ifdef OPTIMISE
static int
nm_so_search_next(struct nm_gate *p_gate,
                  struct nm_so_pkt_wrap **pp_so_pw){

    struct nm_core *p_core = p_gate->p_core;
    struct nm_sched *p_sched = p_core->p_sched;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    p_tbx_slist_t   pre    = p_gate->pre_sched_out_list;

    struct nm_so_pkt_wrap *p_so_pw = NULL;
    struct nm_so_pkt_wrap *cur_so_pw = NULL;
    struct nm_so_pkt_wrap **aggregated_pws = NULL;

    int pw_len = 0;
    int sched_header_size = 0;
    int header_size = 0;
    int rdv_req_size = 0;
    int pre_len = 0;

    int cur_len = 0;
    int cur_proto_id = 0;
    int cur_len = 0;
    void *cur_data;
    struct nm_pkt_wrap *cur_pw = NULL;

    struct nm_proto *p_proto_rdv
        = p_core->p_proto_array[nm_pi_rdv_ack];
    struct nm_rdv_rdv_rq *p_rdv = NULL;

    int idx = 0;
    tbx_bool_t forward = tbx_false;
    int err;


    err = nm_so_header_sizeof_sched_header(&sched_header_size);
    pw_len += header_size;

    err = nm_so_header_sizeof_header(&header_size);

    rdv_req_size =  nm_rdv_rdv_rq_size();

    pre_len = pre->length;
    assert(pre_len);

    tbx_slist_ref_to_head(pre);
    do{
        cur_so_pw = tbx_slist_ref_get(pre);

        err = nm_so_pw_get_length(cur_so_pw, &cur_len);
        nm_so_control_error("nm_so_pw_get_length", err);


#warning CST_A_CHANGER
        // envoi direct
        if(cur_len < SMALL_THRESHOLD){


#warning CST_A_CHANGER
            if(header_size + cur_len + pw_len
               < AGGREGATED_PW_MAX_SIZE){


                forward = tbx_slist_ref_extract_and_forward(pre,
                                                            NULL);

                if(!p_so_pw){
                    err = nm_so_pw_take_aggregation_pw(p_sched,
                                                       p_gate->id
                                                       &p_so_pw);
                    nm_so_control_error("nm_so_pw_take_aggregation_pw", err);
                }


                err = nm_so_pw_get_proto_id(cur_so_pw, &cur_proto_id);
                err = nm_so_pw_get_seq(cur_so_pw, &cur_seq);
                err = nm_so_pw_get_data(cur_so_pw, &cur_data);



                //printf("----------->Ajout de données\n");
                err = nm_so_pw_add_data(p_core, p_so_pw,
                                        cur_proto_id, cur_len,
                                        cur_seq, cur_data);
                nm_so_control_error("nm_so_pw_add_data", err);

                //aggregated_pws[so_pw->nb_aggregated_pws++] = cur_pw;
                nm_so_pw_add_aggregated_pws(p_so_pw, cur_so_pw);

                pw_len += cur_len;



            } else {
                break;
            }



        // envoi par rdv
        } else if(pw_len + header_size + rdv_req_size
                  < AGGREGATED_PW_MAX_SIZE) {

            forward = tbx_slist_ref_extract_and_forward(pre, NULL);
            if(!p_so_pw){
                err = nm_so_pw_take_aggregation_pw(p_sched,
                                                   &p_so_pw);
                nm_so_control_error("nm_so_pw_take_aggregation_pw", err);

            }


            err = nm_so_pw_get_pw(cur_so_pw, &cur_pw);
            nm_so_control_error("nm_so_pw_get_pw", err);

            err = nm_rdv_generate_rdv(p_proto, cur_pw, &p_rdv);
            nm_so_control_error("nm_rdv_generate_rdv", err);


            //printf("--->ajout du rdv\n");
            err = nm_so_pw_add_rdv(p_core,
                                   p_so_pw,
                                   p_rdv, rdv_req_size);


            pw_len += header_size + rdv_req_size;


        } else { // le pw courant ne rentre pas
            break; //forward = tbx_slist_ref_forward(pre);
        }

        idx++;
        if(!forward)
            break;
    } while(tbx_slist_ref_forward(pre));


    if(p_so_pw){
        int nb_seg = 0;
        int len = 0;

        err = nm_so_pw_get_v_nb(p_so_pw, &nb_seg);
        err = nm_so_pw_get_length(p_so_pw, &len);

        err = nm_so_pw_update_global_header(p_so_pw, nb_seg, len);
        nm_so_control_error("nm_so_pw_update_global_header", err);

        /* je mets le wrap sur la piste des petits (la n°0)*/
        struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
        struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[0];
        if(p_gtrk == NULL)
            TBX_FAILURE("p_gtrk NULL");

        err = nm_so_pw_config (p_so_pw,
                               p_gdrv->p_drv, p_gtrk->p_trk,
                               p_gate, p_gdrv, p_gtrk);
        nm_so_control_error("nm_so_pw_config", err);
    }

    *pp_so_pw = p_so_pw;
    err = NM_ESUCCESS;

 out:
     return err;
}

#else // On prend le premier dans la liste

static int
nm_so_search_next(struct nm_gate *p_gate,
                  struct nm_so_pkt_wrap **pp_so_pw){
    struct nm_core *p_core   = p_gate->p_core;
    struct nm_sched *p_sched = p_gate->p_sched;
    p_tbx_slist_t pre  = p_gate->pre_sched_out_list;

    struct nm_so_pkt_wrap *p_so_pw = NULL;
    struct nm_so_pkt_wrap *cur_so_pw = NULL;

    int pre_len = pre->length;

    uint64_t cur_len = 0;
    uint8_t cur_proto_id = 0;
    uint8_t cur_seq = 0;
    void *cur_data = NULL;
    struct nm_pkt_wrap *cur_pw = NULL;

    int err;

    int rdv_req_size =  nm_rdv_rdv_rq_size();

    if(!pre_len){
        err = -NM_ENOTFOUND;
        goto end;
    }

    err = nm_so_pw_take_aggregation_pw(p_sched, p_gate, &p_so_pw);
    nm_so_control_error("nm_so_take_aggregation_pw", err);

    cur_so_pw = tbx_slist_remove_from_head(pre);

    err = nm_so_pw_get_length(cur_so_pw, &cur_len);

    if(cur_len < SMALL_THRESHOLD){

        err = nm_so_pw_get_proto_id(cur_so_pw, &cur_proto_id);
        err = nm_so_pw_get_seq(cur_so_pw, &cur_seq);
        err = nm_so_pw_get_data(cur_so_pw, 0, &cur_data);


        err = nm_so_pw_add_data(p_core, p_so_pw,
                                cur_proto_id, cur_len,
                                cur_seq, cur_data);
        nm_so_control_error("nm_so_pw_add_data", err);


        err = nm_so_pw_add_aggregated_pw(p_so_pw, cur_so_pw);


    } else {

        struct nm_proto *p_proto
            = p_core->p_proto_array[nm_pi_rdv_ack];
        struct nm_rdv_rdv_rq *p_rdv = NULL;

        err = nm_so_pw_get_pw(cur_so_pw, &cur_pw);
        nm_so_control_error("nm_so_pw_get_pw", err);

        err = nm_rdv_generate_rdv(p_proto, cur_pw, &p_rdv);
        nm_so_control_error("nm_rdv_generate_rdv", err);

        err = nm_so_pw_add_rdv(p_core,
                               p_so_pw,
                               p_rdv, rdv_req_size);

    }


    if(p_so_pw){
        uint8_t nb_seg = 0;
        uint64_t len = 0;

        err = nm_so_pw_get_v_nb(p_so_pw, &nb_seg);
        err = nm_so_pw_get_length(p_so_pw, &len);

        err = nm_so_pw_update_global_header(p_so_pw, nb_seg, len);
        nm_so_control_error("nm_so_pw_update_global_header", err);

        /* je mets le wrap sur la piste des petits (la n°0)*/
        struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
        struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[0];
        if(p_gtrk == NULL)
            TBX_FAILURE("p_gtrk NULL");

        err = nm_so_pw_config (p_so_pw,
                               p_gdrv->p_drv, p_gtrk->p_trk,
                               p_gate, p_gdrv, p_gtrk);
        nm_so_control_error("nm_so_pw_config", err);
    }

    *pp_so_pw = p_so_pw;
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
			  struct nm_so_pkt_wrap **pp_so_pw)
{
    return nm_so_search_next(p_gate, pp_so_pw);
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
