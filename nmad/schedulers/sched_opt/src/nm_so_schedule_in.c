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
#include <nm_rdv_public.h>

#define CHRONO


#ifdef CHRONO
double chrono_take_pre_posted = 0.0;
int    nb_take_pre_posted = 0;
#endif

static int
nm_so_take_pre_posted(struct nm_sched *p_sched,
                      struct nm_trk *trk,
                      struct nm_pkt_wrap **pp_pw){
#ifdef CHRONO
    tbx_tick_t      t1, t2;
    nb_take_pre_posted ++;
    TBX_GET_TICK(t1);
#endif

    struct nm_pkt_wrap	*p_pw = NULL;
    int err;

    err = nm_so_take_pre_posted_pw(p_sched, &p_pw);
    nm_so_control_error("nm_so_take_pre_posted", err);

    /* mise sur la piste demandée */
    p_pw->p_drv  = trk->p_drv;
    p_pw->p_trk  = trk;

    *pp_pw = p_pw;

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_take_pre_posted += TBX_TIMING_DELAY(t1, t2);
#endif

    err = NM_ESUCCESS;
    return err;
}

static int
nm_so_schedule_ack(struct nm_core *p_core,
                   struct nm_gate *p_gate,
                   struct nm_rdv_ack_rq *p_ack){
    struct nm_sched    *p_sched = p_core->p_sched;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    int err;

    if(!so_sched->acks){
        err = nm_so_take_aggregation_pw(p_sched,
                                        &so_sched->acks);
        nm_so_control_error("nm_so_take_aggregation_pw", err);
    }


    // ajout de l'ack du scheduler
    struct nm_so_header *so_ack
        = tbx_malloc(so_sched->header_key);
    if(!so_ack){
        err = nm_so_release_aggregation_pw(p_sched, so_sched->acks);
        nm_so_control_error("nm_so_release_aggregation_pw", err);
        err = -NM_ENOMEM;
        goto out;
    }

    so_ack->proto_id = nm_pi_rdv_ack;
    err = nm_iov_append_buf(p_core, so_sched->acks, so_ack,
                            sizeof(struct nm_so_header));
    nm_so_control_error("nm_iov_append_buf", err);


    //ajout de l'ack du protocole
    err = nm_iov_append_buf(p_core, so_sched->acks, p_ack,
                            nm_rdv_ack_rq_size());
    nm_so_control_error("nm_iov_append_buf", err);

    err = NM_ESUCCESS;
 out:
    return err;
}



static int
nm_so_push_acks(struct nm_gate *p_gate){
    struct nm_so_sched *so_sched = p_gate->p_sched->sch_private;
    struct nm_pkt_wrap *acks = so_sched->acks;
    int err;

    err = nm_so_update_global_header(acks,
                                     acks->v_nb, acks->length);
    nm_so_control_error("nm_so_update_global_header", err);

    /* mise sur la piste des petits */
    struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[0];
    if(p_gtrk == NULL)
        TBX_FAILURE("p_gtrk NULL");

    acks->p_gate = p_gate;
    acks->p_drv  = p_gdrv->p_drv;
    acks->p_trk  = p_gtrk->p_trk;
    acks->p_gdrv = p_gdrv;
    acks->p_gtrk = p_gtrk;

    tbx_slist_append(p_gate->post_sched_out_list, acks);
    so_sched->acks = NULL;

    err = NM_ESUCCESS;
    return err;
}


/************************** Interface ********************/
#ifdef CHRONO
double chrono_treat_unexpected = 0.0;
int    nb_treat_unexpected = 0;
#endif

static int
nm_so_treat_unexpected(struct nm_sched *p_sched){
#ifdef CHRONO
    tbx_tick_t      t1;
    tbx_tick_t      t2;
    nb_treat_unexpected ++;
    TBX_GET_TICK(t1);
#endif

    struct nm_so_sched *p_so_sched = p_sched->sch_private;
    p_tbx_slist_t unexpected_list  = p_so_sched->unexpected_list;
    int err;

    if(!tbx_slist_get_length(unexpected_list))
        goto end;

    struct nm_so_unexpected *unexpected = NULL;
    uint8_t proto_id  = 0;
    uint8_t seq       = 0;
    struct nm_pkt_wrap *p_dest_pw = NULL;

    tbx_slist_ref_to_head(unexpected_list);
    do{
        unexpected = tbx_slist_ref_get(unexpected_list);
        proto_id   = unexpected->proto_id;
        seq        = unexpected->seq;

        err = nm_so_search_and_extract_pw(p_sched->submit_aux_recv_req,
                                          proto_id, seq,
                                          &p_dest_pw);
        nm_so_control_error("nm_so_search_and_extract_pw", err);


        if(p_dest_pw){
            tbx_slist_ref_extract_and_forward(unexpected_list,
                                              NULL);

            memcpy(p_dest_pw->v[0].iov_base, unexpected->data,
                   unexpected->len);

            // appel au handler de succès du protocole associé
            struct nm_proto * p_proto
                = p_sched->p_core->p_proto_array[p_dest_pw->proto_id];
            if (!p_proto) {
                NM_TRACEF("unregistered protocol: %d", p_proto->id);
                goto end;
            }
            if (!p_proto->ops.in_success)
                goto end;

            err = p_proto->ops.in_success(p_proto, p_dest_pw);
            nm_so_control_error("proto.ops.in_success", err);

            /* free iovec */
            err = nm_iov_free(p_sched->p_core, p_dest_pw);
            nm_so_control_error("nm_iov_free", err);

            /* free pkt_wrap */
            err= nm_pkt_wrap_free(p_sched->p_core, p_dest_pw);
            nm_so_control_error("nm_pkt_wrap_free", err);

            TBX_FREE(unexpected->data);
            TBX_FREE(unexpected);

            break;
        }
    } while(tbx_slist_ref_forward(unexpected_list));

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_treat_unexpected += TBX_TIMING_DELAY(t1, t2);
#endif

 end:
    err = NM_ESUCCESS;
    return err;
}

#ifdef CHRONO
double chrono_sched_in = 0.0;
int    nb_sched_in = 0;
double chrono_treatment_waiting_rdv = 0.0;
int    nb_treatment_waiting_rdv = 0;
#endif

/* schedule and post new incoming buffers */
int
nm_so_in_schedule(struct nm_sched *p_sched) {
    struct nm_core *p_core = p_sched->p_core;
    struct nm_so_sched *p_so_sched = p_sched->sch_private;
    p_tbx_slist_t trks_to_update = p_so_sched->trks_to_update;
    struct nm_trk *trk = NULL;
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

#ifdef CHRONO
    tbx_tick_t      t1, t2, t3, t4;
    TBX_GET_TICK(t1);
#endif

    // manage the unexpected
    if(tbx_slist_get_length(p_so_sched->unexpected_list)){
        err = nm_so_treat_unexpected(p_sched);
        nm_so_control_error("nm_so_treat_unexpected", err);
    }

#ifdef CHRONO
    TBX_GET_TICK(t3);
#endif
    // traitement de rdv mis en attente
    struct nm_proto *p_proto = p_core->p_proto_array[nm_pi_rdv_req];
    if (!p_proto) {
        NM_TRACEF("unregistered protocol: %d", p_proto->id);
        goto next_step;
    }

    struct nm_rdv_ack_rq *p_ack = NULL;
    struct nm_gate       *ack_gate = NULL;
    err = nm_rdv_treat_waiting_rdv(p_proto, &p_ack, &ack_gate);
    nm_so_control_error("nm_rdv_treat_waiting_rdv", err);

    if(p_ack && ack_gate){
        err = nm_so_schedule_ack(p_core, ack_gate, p_ack);
        nm_so_control_error("nm_so_schedule_ack", err);

        err = nm_so_push_acks(ack_gate);
        nm_so_control_error("nm_so_push_acks", err);

    }
#ifdef CHRONO
    TBX_GET_TICK(t4);
#endif

 next_step:
    while(!tbx_slist_is_nil(trks_to_update)){
        trk = tbx_slist_remove_from_head(trks_to_update);

        if(trk->cap.rq_type == nm_trk_rq_stream){
            struct nm_so_trk *so_trk =
                &(p_so_sched->so_trks[trk->id]);

            assert(!so_trk->pending_stream_intro);

            so_trk->pending_stream_intro = tbx_true;

            // réception de la taille des données
            err = nm_so_take_pre_posted(p_sched, trk, &p_pw);
            nm_so_control_error("nm_so_take_pre_posted", err);

            p_pw->v[p_pw->v_first].iov_len
                = sizeof(nm_so_sched_header_t);

            tbx_slist_append(p_sched->post_perm_recv_req, p_pw);


        } else if(trk->cap.rq_type == nm_trk_rq_dgram){
            err = nm_so_take_pre_posted(p_sched, trk, &p_pw);
            nm_so_control_error("nm_so_take_pre_posted", err);

            tbx_slist_append(p_sched->post_perm_recv_req, p_pw);


        } else if (trk->cap.rq_type == nm_trk_rq_rdv) {
            TBX_FAILURE("nm_so_in_schedule - prendre en cpte les rdv non traités");
        } else if(trk->cap.rq_type ==  nm_trk_rq_put){
            TBX_FAILURE("nm_so_in_schedule - PUT not implemented");
        } else if(trk->cap.rq_type ==  nm_trk_rq_get){
            TBX_FAILURE("nm_so_in_schedule - GET not implemented");
        } else {
            TBX_FAILURE("nm_so_in_schedule - type de track not supported");
        }

        trk = NULL;
    }

    err	= NM_ESUCCESS;

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_sched_in += TBX_TIMING_DELAY(t1, t2);
    nb_sched_in++;
    chrono_treatment_waiting_rdv += TBX_TIMING_DELAY(t3, t4);
    nb_treatment_waiting_rdv++;
#endif

    return err;
}

#ifdef CHRONO
double chrono_open_data = 0.0;
int    nb_open_data = 0;
double chrono_rdv = 0.0;
int    nb_rdv = 0;
double chrono_data = 0.0;
int    nb_data = 0;
#endif

static int
nm_so_open_data(struct nm_sched *p_sched,
                struct nm_gate *p_gate,
                uint8_t  nb_seg,
                void * data){
    struct nm_core *p_core = p_sched->p_core;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    struct nm_pkt_wrap *p_dest_pw = NULL;
    uint8_t seq = 0;
    uint32_t len = 0;
    nm_so_header_t *header = NULL;
    struct nm_proto *p_proto = NULL;
    uint8_t proto_id = -1;
    int err;

#ifdef CHRONO
    tbx_tick_t      t1, t2, t3, t4;
    nb_open_data ++;
    TBX_GET_TICK(t1);
#endif


    while(nb_seg){
        header = (nm_so_header_t *)data;

        proto_id = header->proto_id;

        if(proto_id == nm_pi_rdv_req){
#ifdef CHRONO
            nb_rdv++;
            TBX_GET_TICK(t3);
#endif

            printf("------------> Reception d'un rdv\n");
            struct nm_rdv_ack_rq *p_ack = NULL;

            p_proto = p_core->p_proto_array[nm_pi_rdv_req];

            if (!p_proto) {
                TBX_FAILURE("unregistered protocol");
            }


            err = nm_rdv_treat_rdv_rq(p_proto,
                                      data + sizeof(nm_so_header_t),
                                      &p_ack);
            nm_so_control_error("nm_rdv_treat_rdv_rq", err);

            if(p_ack){
                err = nm_so_schedule_ack(p_core, p_gate, p_ack);
                nm_so_control_error("nm_so_schedule_ack", err);
            }

            data += sizeof(nm_so_header_t) +
                nm_rdv_rdv_rq_size();
            nb_seg -= 2;

#ifdef CHRONO
            TBX_GET_TICK(t4);
            chrono_rdv += TBX_TIMING_DELAY(t3, t4);
#endif


        } else if (proto_id == nm_pi_rdv_ack) {
            printf("------------> Reception d'un ack\n");
            p_proto = p_core->p_proto_array[nm_pi_rdv_req];
            if (!p_proto) {
                TBX_FAILURE("unregistered protocol");
            }

            err = nm_rdv_treat_ack_rq(p_proto,
                                      data + sizeof(nm_so_header_t));
            nm_so_control_error("nm_rdv_treat_ack_rq", err);

            data += sizeof(nm_so_header_t) +
                nm_rdv_ack_rq_size();
            nb_seg -= 2;

        } else if (proto_id >= 127) {
#ifdef CHRONO
            nb_data++;
            TBX_GET_TICK(t3);
#endif
            //printf("------------> Reception de données - ch_id = %d\n", proto_id);
            seq = header->seq;
            len = header->len;

            err = nm_so_search_and_extract_pw(p_sched->submit_aux_recv_req,
                                              proto_id, seq,
                                              &p_dest_pw);
            nm_so_control_error("nm_so_search_and_extract_pw", err);

            data += sizeof(nm_so_header_t);

            if(p_dest_pw){
                memcpy(p_dest_pw->v[0].iov_base, data, len);

                int err;
                p_proto	= p_core->p_proto_array[p_dest_pw->proto_id];
                if (!p_proto) {
                    NM_TRACEF("unregistered protocol: %d", p_proto->id);
                    goto discard;
                }
                if (!p_proto->ops.in_success)
                    goto discard;

                //printf("appel à p_proto->in_success\n");


                err = p_proto->ops.in_success(p_proto, p_dest_pw);
                nm_so_control_error("proto.ops.in_success", err);

            discard:

                /* free iovec */
                err = nm_iov_free(p_core, p_dest_pw);
                nm_so_control_error("nm_iov_free", err);

                /* free pkt_wrap */
                err = nm_pkt_wrap_free(p_core, p_dest_pw);
                nm_so_control_error("nm_pkt_wrap_free", err);

            } else {
                //printf("Pack NON Trouvé\n");
                struct nm_so_unexpected *unexpected
                    = TBX_MALLOC(sizeof(struct nm_so_unexpected));
                if(!unexpected){
                    TBX_FAILURE("TBX_MALLOC of unexpected failed");
                }

                unexpected->proto_id = proto_id;
                unexpected->seq = seq;
                unexpected->len = len;
                unexpected->data = TBX_MALLOC(len);
                memcpy(unexpected->data, data, len);

                tbx_slist_append(so_sched->unexpected_list,
                                 unexpected);
            }

            data += len;

            nb_seg -= 2;

#ifdef CHRONO
            TBX_GET_TICK(t4);
            chrono_data += TBX_TIMING_DELAY(t3, t4);
#endif

        } else {
            printf("proto_id failed = %d\n", proto_id);
            TBX_FAILURE("Entête non supportée");
        }
    }
    if(so_sched->acks){
        err = nm_so_push_acks(p_gate);
        nm_so_control_error("nm_so_push_acks", err);
    }


#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_open_data += TBX_TIMING_DELAY(t1, t2);
#endif
    //printf("<--open_received_data_complete\n");
    err = NM_ESUCCESS;
    return err;
}

static int
nm_so_open_received_data_complete(struct nm_sched *p_sched,
                                  struct nm_gate *p_gate,
                                  void * data){
    int err;
    nm_so_sched_header_t * global_header
        = (nm_so_sched_header_t *)data;

    uint8_t proto_id = global_header->proto_id;
    assert(proto_id == nm_pi_sched);

    uint8_t  nb_seg = global_header->nb_seg;
    nb_seg--;
    assert(nb_seg > 0);

    data += sizeof(nm_so_sched_header_t);


    err = nm_so_open_data(p_sched, p_gate, nb_seg, data);
    nm_so_control_error("nm_so_open_data", err);

    err = NM_ESUCCESS;
    return err;
}



#ifdef CHRONO
double chrono_success_in = 0.0;
int    nb_success_in = 0;
#endif
/* process complete successful incoming request */
int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw) {
    //printf("-->nm_so_in_process_success_rq\n");
#ifdef CHRONO
    tbx_tick_t      t1, t2;
    nb_success_in ++;
    TBX_GET_TICK(t1);
#endif

    struct nm_core *p_core = p_sched->p_core;
    struct nm_so_sched *p_so_sched = p_sched->sch_private;
    struct nm_trk *p_trk = p_pw->p_trk;
    struct nm_so_trk *so_trk = &(p_so_sched->so_trks[p_trk->id]);
    struct nm_so_pkt_wrap * so_wrap = NULL;

    void * data = p_pw->v[p_pw->v_first].iov_base;
    int err;

    if(p_trk->cap.rq_type == nm_trk_rq_stream){
        if(so_trk->pending_stream_intro){ // on attend l'entete
            //DISP("-------------ENTETE----------------");

            so_trk->pending_stream_intro = tbx_false;

            struct nm_pkt_wrap	*p_data_pw = NULL;
            int len = 0;

            struct nm_so_sched_header * header
                = (struct nm_so_sched_header *)data;

            len = header->len - sizeof(struct nm_so_sched_header);

            so_wrap = TBX_MALLOC(sizeof(struct nm_so_pkt_wrap));
            if(!so_wrap){
                err = -NM_ENOMEM;
                goto end;
            }
            so_wrap->nb_seg = header->nb_seg - 1;

            // remise du wrapper d'intro
            err = nm_so_release_pre_posted_pw(p_sched, p_pw);
            nm_so_control_error("nm_so_release_pre_posted_pw", err);

            // on dépose la réception des données
            err=nm_so_take_pre_posted(p_sched, p_trk, &p_data_pw);
            nm_so_control_error("nm_so_take_pre_posted", err);

            p_data_pw->v[p_data_pw->v_first].iov_len = len;
            p_data_pw->sched_priv = so_wrap;

            tbx_slist_append(p_sched->post_aux_recv_req, p_data_pw);

        } else { // on attend les données
            //DISP("-------------DONNEES----------------");

            so_wrap = p_pw->sched_priv;

            // déballage des données
            err = nm_so_open_data(p_sched, p_pw->p_gate,
                                  so_wrap->nb_seg, data);
            nm_so_control_error("nm_so_open_data", err);

            // piste à mettre à jour
            tbx_slist_append(p_so_sched->trks_to_update,
                             p_pw->p_trk);

            // remise du wrapper
            err = nm_so_release_pre_posted_pw(p_sched, p_pw);
            nm_so_control_error("nm_so_release_pre_posted_pw", err);

            TBX_FREE(so_wrap);
        }


    } else if(p_trk->cap.rq_type == nm_trk_rq_dgram){

        //printf("-------------PETIT Pack RECU----------------\n\n");
        // déballage des données
        err = nm_so_open_received_data_complete(p_sched,
                                                p_pw->p_gate, data);
        nm_so_control_error("nm_so_open_received_data_complete", err);

        // piste à mettre à jour
        tbx_slist_append(p_so_sched->trks_to_update, p_pw->p_trk);

        // remise du wrapper
        err = nm_so_release_pre_posted_pw(p_sched, p_pw);
        nm_so_control_error("nm_so_release_pre_posted_pw", err);

    } else if (p_trk->cap.rq_type == nm_trk_rq_rdv) {
        printf("-------------LARGE Pack RECU----------------\n\n");
        // signal au protocole de rdv que la piste est libérée
         struct nm_proto * p_proto_rdv
             = p_core->p_proto_array[nm_pi_rdv_req];
         if(!p_proto_rdv){
             TBX_FAILURE("unregistered protocol");
         }

         err = nm_rdv_free_track(p_proto_rdv, p_trk->id);
         nm_so_control_error("nm_rdv_free_track", err);

         // appel au handler de succès du protocole associé
         struct nm_proto * p_proto = p_core->p_proto_array[p_pw->proto_id];
        if (!p_proto) {
            NM_TRACEF("unregistered protocol: %d", p_proto->id);
            goto end;
        }
        if (!p_proto->ops.in_success)
            goto end;

        err = p_proto->ops.in_success(p_proto, p_pw);
        nm_so_control_error("proto.ops.in_success", err);

        /* free iovec */
        err = nm_iov_free(p_core, p_pw);
        nm_so_control_error("nm_iov_free", err);

        /* free pkt_wrap */
        err = nm_pkt_wrap_free(p_core, p_pw);
        nm_so_control_error("nm_pkt_wrap_free", err);


    } else if(p_trk->cap.rq_type ==  nm_trk_rq_put){
        TBX_FAILURE("nm_so_in_process_success_rq - PUT not implemented");
    } else if(p_trk->cap.rq_type ==  nm_trk_rq_get){
        TBX_FAILURE("nm_so_in_process_success_rq - GET not implemented");
    } else {
        TBX_FAILURE("nm_so_in_process_success_rq - type de track not supported");
    }

    err = NM_ESUCCESS;
 end:
#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_success_in += TBX_TIMING_DELAY(t1, t2);
#endif

    return err;
}



/* process complete failed incoming request */
int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		_err) {
    TBX_FAILURE("nm_so_in_process_failed_rq");
    return nm_so_in_process_success_rq(p_sched, p_pw);
}

