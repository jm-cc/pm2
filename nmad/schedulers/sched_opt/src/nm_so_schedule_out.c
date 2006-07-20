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

#include <nm_rdv_public.h>


//#define OPTIMISE
//#define PAS_SAUT

//#define CHRONO

#ifdef CHRONO
int nb_search_next = 0;
double chrono_search_next = 0;
#endif

//#ifdef OPTIMISE
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



    //printf("-->nm_so_search_next\n");

#ifdef CHRONO
    tbx_tick_t t1, t2;

    TBX_GET_TICK(t1);
#endif

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
    }while(tbx_slist_ref_forward(pre));


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
    //printf("<--nm_so_search_next\n");

    *pp_pw = p_pw;
    err = NM_ESUCCESS;

 out:

#ifdef CHRONO
     TBX_GET_TICK(t2);
     chrono_search_next += TBX_TIMING_DELAY(t1, t2);
     nb_search_next++;
#endif
     return err;
}

//#else // On prend le premier dans la liste
//
//static struct nm_pkt_wrap *
//nm_so_search_next(struct nm_gate *p_gate){
//    struct nm_core *p_core   = p_gate->p_core;
//    p_tbx_slist_t pre  = p_gate->pre_sched_out_list;
//
//    struct nm_pkt_wrap *p_pw = NULL;
//    struct nm_pkt_wrap *cur_pw = NULL;
//
//    struct nm_so_pkt_wrap *so_pw = NULL;
//
//
//    int pre_len = pre->length;
//
//#ifdef CHRONO
//    tbx_tick_t t1, t2;
//
//    TBX_GET_TICK(t1);
//#endif
//
//    //printf("-->nm_so_search_next\n");
//    if(!pre_len)
//        goto end;
//
//    p_pw = nm_so_take_aggregation_pw(p_gate->p_sched);
//    so_pw = p_pw->sched_priv;
//
//
//    tbx_slist_ref_to_head(pre);
//
//    //cur_pw = tbx_slist_remove_from_head(pre);
//    tbx_slist_ref_extract_and_forward(pre, &cur_pw);
//    if(cur_pw->length < SMALL_THRESHOLD){
//       //printf("-->Ajout de données\n");
//
//        nm_so_add_data(p_core, p_pw,
//                       cur_pw->proto_id, cur_pw->length,
//                       cur_pw->seq, cur_pw->v[0].iov_base);
//
//        so_pw->aggregated_pws[so_pw->nb_aggregated_pws++] = cur_pw;
//
//    } else {
//        struct nm_proto *p_proto
//            = p_core->p_proto_array[nm_pi_rdv_ack];
//        struct nm_rdv_rdv_rq *p_rdv = NULL;
//
//        nm_rdv_generate_rdv(p_proto, cur_pw, &p_rdv);
//
//        //printf("--->ajout du rdv du scheduler\n");
//        struct nm_so_header *so_rdv
//            = TBX_MALLOC(sizeof(struct nm_so_header));
//        so_rdv->proto_id = nm_pi_rdv_req;
//        nm_iov_append_buf(p_core, p_pw, so_rdv,
//                          sizeof(struct nm_so_header));
//
//        //printf("--->ajout du rdv du protocole\n");
//        nm_iov_append_buf(p_core, p_pw, p_rdv,
//                          nm_rdv_rdv_rq_size());
//
//    }
//
//    nm_so_update_global_header(p_pw, p_pw->v_nb);
//
//    /* je mets le wrap sur la piste des petits (la n°0)*/
//    struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
//    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[0];
//    if(p_gtrk == NULL)
//        TBX_FAILURE("p_gtrk NULL");
//
//    p_pw->p_gate = p_gate;
//    p_pw->p_drv  = p_gdrv->p_drv;
//    p_pw->p_trk  = p_gtrk->p_trk;
//    p_pw->p_gdrv = p_gdrv;
//    p_pw->p_gtrk = p_gtrk;
//
//   //printf("<--nm_so_search_next\n");
//
// end:
//#ifdef CHRONO
//    TBX_GET_TICK(t2);
//    chrono_search_next += TBX_TIMING_DELAY(t1, t2);
//    nb_search_next++;
//#endif
//
//    return p_pw;
//}
//#endif

static int
nm_so_search_next2(struct nm_gate *p_gate,
                   struct nm_pkt_wrap **pp_pw){

    p_tbx_slist_t       pre    = NULL;
    struct nm_gate_drv *p_gdrv = NULL;
    struct nm_drv      *p_drv  = NULL;
    struct nm_gate_trk *p_gtrk = NULL;
    struct nm_pkt_wrap *p_pw   = NULL;
    int err;

    DISP("-->nm_so_search_next2");

    pre     = p_gate->pre_sched_out_list;
    p_gdrv  = p_gate->p_gate_drv_array[0];
    p_drv   = p_gdrv->p_drv;

    p_gtrk  = p_gdrv->p_gate_trk_array[0];
    if(p_gtrk == NULL)
        TBX_FAILURE("p_gtrk NULL");

    /* construction de prochain pack à émettre */
    err = nm_so_strategy_application(p_gate, p_drv, pre, &p_pw);
    nm_so_control_error("nm_so_strategy_application", err);

    if(!p_pw){
        err = -NM_ENOTFOUND;
        goto out;
    }

    /* je mets le wrap sur la piste des petits (la n°0)*/
    p_pw->p_gate = p_gate;
    p_pw->p_drv  = p_drv;
    p_pw->p_trk  = p_gtrk->p_trk;
    p_pw->p_gdrv = p_gdrv;
    p_pw->p_gtrk = p_gtrk;

    *pp_pw = p_pw;

    err = NM_ESUCCESS;
 out:
    DISP("<--nm_so_search_next2");

    return err;
}


/**************** Interface ****************/

#ifdef CHRONO
int    nb_sched_out = 0;
double chrono_sched_out = 0;
#endif

/* schedule and post new outgoing buffers */
int
nm_so_out_schedule_gate(struct nm_gate *p_gate) {

    struct nm_so_gate *so_gate = p_gate->sch_private;
    p_tbx_slist_t pre  = p_gate->pre_sched_out_list;
    p_tbx_slist_t post = p_gate->post_sched_out_list;
    struct nm_pkt_wrap *p_pw = NULL;
    int	err;

#ifdef CHRONO
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
#endif

    if(tbx_slist_is_nil(pre))
        goto out;

    //printf("-->nm_so_out_schedule_gate\n");
    /* already one pending request or something to send */
    if(p_gate->out_req_nb || !tbx_slist_is_nil(post))
        goto reload;

    if(so_gate->next_pw) {
        p_pw = so_gate->next_pw;
        so_gate->next_pw = NULL;

    } else {
        if(tbx_slist_is_nil(pre)){
            err = -NM_ENOTFOUND;
            goto out;
        }

        err = nm_so_search_next2(p_gate, &p_pw);
        nm_so_control_error("nm_so_schedule_gate", err);

        if(err == -NM_ENOTFOUND){
            goto out;
        }

    }

    if (p_pw->p_trk->cap.rq_type == nm_trk_rq_dgram
        || p_pw->p_trk->cap.rq_type == nm_trk_rq_stream){
        /* append pkt to scheduler post list */
        tbx_slist_append(post, p_pw);

    } else {
        TBX_FAILURE("Track request type is invalid");
    }

 reload:
    //a-t-on un next?
    if(!tbx_slist_is_nil(pre) && !so_gate->next_pw){
        err = nm_so_search_next2(p_gate, &so_gate->next_pw);
        nm_so_control_error("nm_so_search_next", err);
    }

 out:
    //printf("<--nm_so_out_schedule_gate\n");

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_sched_out += TBX_TIMING_DELAY(t1, t2);
    nb_sched_out++;
#endif

    err = NM_ESUCCESS;
    return err;
}


#ifdef CHRONO
int    nb_free_aggregated = 0;
double chrono_free_aggregated = 0;
#endif

static int
nm_so_free_aggregated_pw(struct nm_core *p_core,
                         struct nm_pkt_wrap *p_pw){

    struct nm_so_pkt_wrap * so_pw = p_pw->sched_priv;
    struct nm_pkt_wrap **aggregated_pws = so_pw->aggregated_pws;
    int nb_aggregated_pws = so_pw->nb_aggregated_pws;
    struct nm_pkt_wrap *cur_pw = NULL;
    struct nm_proto *p_proto = NULL;
    int err;
    int i;

#ifdef CHRONO
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
#endif

    //printf("-->nm_so_free_aggregated_pw\n");
    for(i = 0; i < nb_aggregated_pws; i++){
        cur_pw = aggregated_pws[i];

        p_proto	= p_core->p_proto_array[cur_pw->proto_id];
        if (!p_proto) {
            NM_TRACEF("unregistered protocol: %d", p_proto->id);
            continue;
        }
        if (!p_proto->ops.out_success)
            continue;

        err = p_proto->ops.out_success(p_proto, cur_pw);
        nm_so_control_error("proto.ops.out_success", err);

        /* free iovec */
        err = nm_iov_free(p_core, cur_pw);
        nm_so_control_error("nm_iov_free", err);

        /* free pkt_wrap */
        err = nm_pkt_wrap_free(p_core, cur_pw);
        nm_so_control_error("nm_pkt_wrap_free", err);

    }
    //printf("<--nm_so_free_aggregated_pw\n");

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_free_aggregated += TBX_TIMING_DELAY(t1, t2);
    nb_free_aggregated++;
#endif

    err = NM_ESUCCESS;
    return err;
}


#ifdef CHRONO
int    nb_success_out = 0;
double chrono_success_out = 0;
#endif
/* process complete successful outgoing request */
int
nm_so_out_process_success_rq(struct nm_sched *p_sched,
                             struct nm_pkt_wrap	*p_pw) {
    struct nm_core     *p_core   = p_sched->p_core;
    struct nm_proto    *p_proto  = NULL;
    struct nm_so_sched *so_sched = p_sched->sch_private;

    struct nm_so_header  *header = NULL;
    struct nm_rdv_rdv_rq *rdv = NULL;
    struct nm_rdv_ack_rq *ack = NULL;

    int v_nb = p_pw->v_nb;
    int idx  = 0;
    int err;

    //printf("-->nm_so_out_process_success_rq\n");

#ifdef CHRONO
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
#endif

    if(p_pw->sched_priv){
        //printf("PETIT PACK ENVOYé \n\n");

        idx++;
        v_nb--;

        while(v_nb > 0){
            header = p_pw->v[idx].iov_base;

            if(header->proto_id == nm_pi_rdv_req){
                printf("--------------->Envoi d'un rdv\n");
                p_proto = p_core->p_proto_array[nm_pi_rdv_req];
                if (!p_proto) {
                    NM_TRACEF("unregistered protocol: %d",
                              p_proto->id);
                    continue;
                }

                idx++;

                rdv = p_pw->v[idx].iov_base;
                err = nm_rdv_free_rdv(p_proto, rdv);
                nm_so_control_error("nm_rdv_free_rdv", err);

                tbx_free(so_sched->header_key, header);
                idx++;
                v_nb-=2;

            } else if(header->proto_id == nm_pi_rdv_ack){
                printf("------------->Envoi d'un ack\n");
                p_proto = p_core->p_proto_array[nm_pi_rdv_ack];
                if (!p_proto) {
                    NM_TRACEF("unregistered protocol: %d",
                              p_proto->id);
                    continue;
                }

                idx++;

                ack = p_pw->v[idx].iov_base;
                err = nm_rdv_free_ack(p_proto, ack);
                nm_so_control_error("nm_rdv_free_ack", err);

                tbx_free(so_sched->header_key, header);
                idx++;
                v_nb-=2;

            } else if(header->proto_id >= 127){
                tbx_free(so_sched->header_key, header);
                idx+= 2;
                v_nb-=2;
            }
        }

        err = nm_so_free_aggregated_pw(p_core, p_pw);
        nm_so_control_error("nm_so_free_aggregated_pw", err);

        err = nm_so_release_aggregation_pw(p_sched, p_pw);
        nm_so_control_error("nm_so_release_aggregation_pw", err);

    } else {
        printf("----------------LARGE PACK ENVOYé - p_pw = %p\n\n", p_pw);

        p_proto	= p_core->p_proto_array[p_pw->proto_id];
        if (!p_proto) {
            NM_TRACEF("unregistered protocol: %d", p_proto->id);
            goto free;
        }
        if (!p_proto->ops.out_success)
            goto free;

        err = p_proto->ops.out_success(p_proto, p_pw);
        nm_so_control_error("proto.ops.out_success", err);

    free:
        /* free iovec */
        err = nm_iov_free(p_core, p_pw);
        nm_so_control_error("nm_iov_free", err);

        /* free pkt_wrap */
        err = nm_pkt_wrap_free(p_core, p_pw);
        nm_so_control_error("nm_pkt_wrap_free", err);

    }
    //printf("<--nm_so_out_process_success_rq\n");
    err	= NM_ESUCCESS;

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_success_out += TBX_TIMING_DELAY(t1, t2);
    nb_success_out++;
#endif

    return err;
}

/* process complete failed outgoing request */
int
nm_so_out_process_failed_rq(struct nm_sched		*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err) {
    TBX_FAILURE("nm_so_out_process_failed_rq");
    return nm_so_out_process_success_rq(p_sched,p_pw);
}
