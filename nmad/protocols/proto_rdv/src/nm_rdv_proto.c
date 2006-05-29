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

#include <nm_public.h>
#include "nm_rdv_private.h"

#define NB_RDV_RQ 100
#define NB_ACK_RQ 100

static struct nm_pkt_wrap *
get_and_extract(p_tbx_slist_t list,
                uint8_t proto_id, uint8_t seq){
    struct nm_pkt_wrap * p_pw = NULL;

    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        p_pw = tbx_slist_ref_get(list);

        if(p_pw->proto_id == proto_id && p_pw->seq == seq){
            tbx_slist_ref_extract_and_forward(list, &p_pw);
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

    p_pw = NULL;

 end:
    return p_pw;
}

static struct nm_pkt_wrap *
get(p_tbx_slist_t list,
                uint8_t proto_id, uint8_t seq){
    struct nm_pkt_wrap * p_pw = NULL;

    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        p_pw = tbx_slist_ref_get(list);

        if(p_pw->proto_id == proto_id && p_pw->seq == seq){
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

    p_pw = NULL;

 end:
    return p_pw;
}



void
nm_rdv_generate_rdv(struct nm_proto *p_proto,
                    struct nm_pkt_wrap *p_pw,
                    struct nm_rdv_rdv_rq **pp_rdv_rq){
    struct nm_rdv * nm_rdv = p_proto->priv;

    // génére la demande de rdv
    struct nm_rdv_rdv_rq *p_rdv_rq
        = tbx_malloc(nm_rdv->rdv_key);
    p_rdv_rq->proto_id = p_pw->proto_id;
    p_rdv_rq->seq = p_pw->seq;
    p_rdv_rq->len = p_pw->length;

    // stocke le pw associé
    p_tbx_slist_t large_waiting_ack = nm_rdv->large_waiting_ack;
    tbx_slist_append(large_waiting_ack, p_pw);

    // retourne la demande de rdv
    *pp_rdv_rq = p_rdv_rq;
}

//static int
//nm_rdv_check_track(struct nm_gate_drv *const p_gdrv,
//                   struct nm_gate_trk *const p_gtrk) {
//    int	err = -NM_EAGAIN;
//
//    /* all the track sending slots are currently in use */
//    if (p_gtrk->p_trk->cap.max_pending_send_request &&
//        p_gtrk->p_trk->out_req_nb >= p_gtrk->p_trk->cap.max_pending_send_request)
//        goto out;
//
//    err = NM_ESUCCESS;
//
// out:
//    return err;
//}

//void
//nm_rdv_treat_rdv_rq(struct nm_proto *p_proto,
//                    struct nm_rdv_rdv_rq *p_rdv_rq,
//                    struct nm_rdv_ack_rq **pp_ack){
//    uint8_t  proto_id = p_rdv_rq->proto_id;
//    uint8_t  seq  = p_rdv_rq->seq;
//
//    struct nm_pkt_wrap * large_pw = NULL;
//
//    struct nm_rdv *nm_rdv= p_proto->priv;
//    struct nm_sched *p_sched = nm_rdv->p_sched;
//
//
//    if(proto_id == nm_pi_sched){ // rdv sur des données agrégées
//        //printf("Allocation d'une zone intermédiaire\n");//???
//        ;
//
//    } else if(proto_id >= 127){
//        large_pw = get(p_sched->submit_aux_recv_req,
//                       proto_id, seq);
//
//        // si le unpack est prêt
//        if(large_pw){
//            /* je mets le wrap sur la piste des larges */
//            struct nm_gate_drv *p_gdrv = large_pw->p_gate->p_gate_drv_array[0];
//            struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[1];
//            large_pw->p_drv  = p_gdrv->p_drv;
//            large_pw->p_trk  = p_gtrk->p_trk;
//            large_pw->p_gdrv = p_gdrv;
//            large_pw->p_gtrk = p_gtrk;
//
//            // si la piste est libre
//            if(nm_rdv_check_track(large_pw->p_gdrv, large_pw->p_gtrk) == NM_ESUCCESS){
//                //printf("-->Large trouvé\n");
//                large_pw = get_and_extract(p_sched->submit_aux_recv_req,
//                                           proto_id, seq);
//
//
//                // création de l'ack
//                struct nm_rdv_ack_rq *p_ack
//                    = tbx_malloc(nm_rdv->ack_key);
//                p_ack->proto_id = proto_id;
//                p_ack->seq = seq;
//                *pp_ack = p_ack;
//                //printf("-->création de l'ack\n");
//
//                //printf("-->dépot de la réceptiondu large \n");
//                // initialisation de la réception
//                tbx_slist_append(p_sched->post_aux_recv_req,
//                                 large_pw);
//                return;
//            }
//        }
//        //printf("-->stockage du rdv\n");
//
//        struct nm_rdv *nm_rdv = p_proto->priv;
//        struct nm_rdv_rdv_rq *p_rdv_cpy = TBX_MALLOC(sizeof(struct nm_rdv_rdv_rq));
//        memcpy(p_rdv_cpy, p_rdv_rq, sizeof(struct nm_rdv_rdv_rq));
//        tbx_slist_append(nm_rdv->waiting_rdv, p_rdv_cpy);
//
//    }
//}
//void
//nm_rdv_treat_ack_rq(struct nm_proto *p_proto,
//                    struct nm_rdv_ack_rq *p_ack){
//    struct nm_rdv *nm_rdv = p_proto->priv;
//    p_tbx_slist_t large_waiting_ack = nm_rdv->large_waiting_ack;
//    struct nm_pkt_wrap * large_pw = NULL;
//
//    uint8_t proto_id = p_ack->proto_id;
//    uint8_t seq = p_ack->seq;
//
//    large_pw = get_and_extract(large_waiting_ack, proto_id, seq);
//    if(!large_pw)
//        FAILURE("Large non retrouvé");
//
//    // on envoie le large
//    struct nm_gate *p_gate = large_pw->p_gate;
//
//    /* je mets le wrap sur la piste des larges */
//    struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
//    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[1];
//    if(p_gtrk == NULL)
//        printf("p_gtrk NULL");
//
//    large_pw->p_drv  = p_gdrv->p_drv;
//    large_pw->p_trk  = p_gtrk->p_trk;
//    large_pw->p_gdrv = p_gdrv;
//    large_pw->p_gtrk = p_gtrk;
//
//    tbx_slist_append(p_gate->post_sched_out_list,
//                     large_pw);
//
//}
//struct nm_gate *
//nm_rdv_treat_waiting_rdv(struct nm_proto *p_proto,
//                         struct nm_rdv_ack_rq **pp_ack){
//    struct nm_rdv *nm_rdv = p_proto->priv;
//    p_tbx_slist_t waiting_rdv = nm_rdv->waiting_rdv;
//    struct nm_rdv_rdv_rq * rdv = NULL;
//    struct nm_pkt_wrap * large_pw = NULL;
//
//    uint8_t  proto_id = 0;
//    uint8_t  seq  = 0;
//
//    struct nm_sched *p_sched = nm_rdv->p_sched;
//
//    if(!waiting_rdv->length)
//        return NULL;
//
//    tbx_slist_ref_to_head(waiting_rdv);
//    do{
//        rdv = tbx_slist_ref_get(waiting_rdv);
//
//        proto_id = rdv->proto_id;
//        seq  = rdv->seq;
//
//        large_pw = get(p_sched->submit_aux_recv_req,
//                       proto_id, seq);
//
//        // si le unpack est prêt
//        if(large_pw){
//            // initialisation de la réception
//            struct nm_gate *p_gate = large_pw->p_gate;
//            struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
//            struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[1];
//            if(p_gtrk == NULL)
//                printf("p_gtrk NULL");
//
//            large_pw->p_drv  = p_gdrv->p_drv;
//            large_pw->p_trk  = p_gtrk->p_trk;
//            large_pw->p_gdrv = p_gdrv;
//            large_pw->p_gtrk = p_gtrk;
//
//            // si la piste est libre
//            if(nm_rdv_check_track(large_pw->p_gdrv, large_pw->p_gtrk)
//               == NM_ESUCCESS){
//
//                //printf("-->DEStockage du rdv\n");
//                tbx_slist_ref_extract_and_forward(waiting_rdv,
//                                                  &rdv);
//
//                large_pw = get_and_extract(p_sched->submit_aux_recv_req, proto_id, seq);
//
//                // création de l'ack
//                struct nm_rdv_ack_rq *p_ack
//                    = tbx_malloc(nm_rdv->ack_key);
//                p_ack->proto_id = proto_id;
//                p_ack->seq = seq;
//                //printf("p_ack = %p\n", p_ack);
//                *pp_ack = p_ack;
//
//                tbx_slist_append(p_sched->post_aux_recv_req,
//                                 large_pw);
//
//                return large_pw->p_gate;
//            }
//        }
//
//    }while(tbx_slist_ref_forward(waiting_rdv));
//    return NULL;
//}



static int
nm_rdv_give_free_track(struct nm_rdv *nm_rdv,
                       struct nm_gate_drv *const p_gdrv) {
    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[1];

    /* all the track sending slots are currently in use */
    if (p_gtrk->p_trk->out_req_nb + nm_rdv->out_req_nb[1] < p_gtrk->p_trk->cap.max_pending_send_request){
        nm_rdv->out_req_nb[1]++;
        //printf("Je donne le endpoint n° 1\n");
        return 1;
    }

    p_gtrk = p_gdrv->p_gate_trk_array[2];
    if (p_gtrk->p_trk->out_req_nb + nm_rdv->out_req_nb[2] < p_gtrk->p_trk->cap.max_pending_send_request){
        nm_rdv->out_req_nb[2]++;
        //printf("Je donne le endpoint n° 2\n");
        return 2;
    }

    p_gtrk = p_gdrv->p_gate_trk_array[3];
    if (p_gtrk->p_trk->out_req_nb + nm_rdv->out_req_nb[3] < p_gtrk->p_trk->cap.max_pending_send_request){
        printf("Je donne le endpoint n° 3\n");
        nm_rdv->out_req_nb[3]++;
        return 3;
    }

    printf("Tous les endpoints sont occupés!!\n");
    return -1;
}

void
nm_rdv_treat_rdv_rq(struct nm_proto *p_proto,
                    struct nm_rdv_rdv_rq *p_rdv_rq,
                    struct nm_rdv_ack_rq **pp_ack){
    uint8_t  proto_id = p_rdv_rq->proto_id;
    uint8_t  seq  = p_rdv_rq->seq;

    struct nm_pkt_wrap * large_pw = NULL;

    struct nm_rdv *nm_rdv= p_proto->priv;
    struct nm_sched *p_sched = nm_rdv->p_sched;


    if(proto_id == nm_pi_sched){ // rdv sur des données agrégées
        //printf("Allocation d'une zone intermédiaire\n");//???
        ;

    } else if(proto_id >= 127){
        large_pw = get(p_sched->submit_aux_recv_req,
                       proto_id, seq);

        // si le unpack est prêt
        if(large_pw){
            /* je mets le wrap sur la piste des larges */
            struct nm_gate_drv *p_gdrv = large_pw->p_gate->p_gate_drv_array[0];

            // recupère l'id de la 1ere piste libre
            int track_id = nm_rdv_give_free_track(nm_rdv, p_gdrv);

            if(track_id != -1){ // une est libre
                struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[track_id];
                large_pw->p_drv  = p_gdrv->p_drv;
                large_pw->p_trk  = p_gtrk->p_trk;
                large_pw->p_gdrv = p_gdrv;
                large_pw->p_gtrk = p_gtrk;


                large_pw = get_and_extract(p_sched->submit_aux_recv_req,
                                           proto_id, seq);

                // création de l'ack
                struct nm_rdv_ack_rq *p_ack
                    = tbx_malloc(nm_rdv->ack_key);
                p_ack->proto_id = proto_id;
                p_ack->seq = seq;
                p_ack->track_id = track_id;
                *pp_ack = p_ack;
                //printf("-->création de l'ack\n");

                //printf("-->dépot de la réceptiondu large \n");
                // initialisation de la réception
                tbx_slist_append(p_sched->post_aux_recv_req,
                                 large_pw);
                return;
            } else {
                //printf("-->stockage du rdv\n");

                struct nm_rdv *nm_rdv = p_proto->priv;
                struct nm_rdv_rdv_rq *p_rdv_cpy = TBX_MALLOC(sizeof(struct nm_rdv_rdv_rq));
                memcpy(p_rdv_cpy, p_rdv_rq, sizeof(struct nm_rdv_rdv_rq));
                tbx_slist_append(nm_rdv->waiting_rdv, p_rdv_cpy);
            }
        }
    }
}


void
nm_rdv_treat_ack_rq(struct nm_proto *p_proto,
                    struct nm_rdv_ack_rq *p_ack){
    struct nm_rdv *nm_rdv = p_proto->priv;
    p_tbx_slist_t large_waiting_ack = nm_rdv->large_waiting_ack;
    struct nm_pkt_wrap * large_pw = NULL;

    uint8_t proto_id = p_ack->proto_id;
    uint8_t seq = p_ack->seq;
    uint8_t track_id = p_ack->track_id;

    large_pw = get_and_extract(large_waiting_ack, proto_id, seq);
    if(!large_pw)
        FAILURE("Large non retrouvé");

    // on envoie le large
    struct nm_gate *p_gate = large_pw->p_gate;

    /* je mets le wrap sur la piste des larges */
    struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];
    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[track_id];
    if(p_gtrk == NULL)
        printf("p_gtrk NULL");

    large_pw->p_drv  = p_gdrv->p_drv;
    large_pw->p_trk  = p_gtrk->p_trk;
    large_pw->p_gdrv = p_gdrv;
    large_pw->p_gtrk = p_gtrk;

    tbx_slist_append(p_gate->post_sched_out_list, large_pw);

}

struct nm_gate *
nm_rdv_treat_waiting_rdv(struct nm_proto *p_proto,
                         struct nm_rdv_ack_rq **pp_ack){
    struct nm_rdv *nm_rdv = p_proto->priv;
    p_tbx_slist_t waiting_rdv = nm_rdv->waiting_rdv;
    struct nm_rdv_rdv_rq * rdv = NULL;
    struct nm_pkt_wrap * large_pw = NULL;

    uint8_t  proto_id = 0;
    uint8_t  seq  = 0;

    struct nm_sched *p_sched = nm_rdv->p_sched;

    if(!waiting_rdv->length)
        return NULL;

    tbx_slist_ref_to_head(waiting_rdv);
    do{
        rdv = tbx_slist_ref_get(waiting_rdv);

        proto_id = rdv->proto_id;
        seq  = rdv->seq;

        large_pw = get(p_sched->submit_aux_recv_req,
                       proto_id, seq);

        // si le unpack est prêt
        if(large_pw){
            // initialisation de la réception
            struct nm_gate *p_gate = large_pw->p_gate;
            struct nm_gate_drv *p_gdrv = p_gate->p_gate_drv_array[0];

            // recupère l'id de la 1ere piste libre
            int track_id = nm_rdv_give_free_track(nm_rdv, p_gdrv);
            if(track_id == -1) // aucune n'est libre
                continue;

            struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[track_id];
            if(p_gtrk == NULL)
                printf("p_gtrk NULL");

            large_pw->p_drv  = p_gdrv->p_drv;
            large_pw->p_trk  = p_gtrk->p_trk;
            large_pw->p_gdrv = p_gdrv;
            large_pw->p_gtrk = p_gtrk;

            //printf("-->DEStockage du rdv\n");
            tbx_slist_ref_extract_and_forward(waiting_rdv,
                                              &rdv);

            large_pw = get_and_extract(p_sched->submit_aux_recv_req, proto_id, seq);

            // création de l'ack
            struct nm_rdv_ack_rq *p_ack
                = tbx_malloc(nm_rdv->ack_key);
            p_ack->proto_id = proto_id;
            p_ack->seq = seq;
            p_ack->track_id = track_id;
            //printf("p_ack = %p\n", p_ack);
            *pp_ack = p_ack;

            tbx_slist_append(p_sched->post_aux_recv_req,
                             large_pw);

            return large_pw->p_gate;
        }

    }while(tbx_slist_ref_forward(waiting_rdv));
    return NULL;
}

void
nm_rdv_free_rdv(struct nm_proto *p_proto,
                struct nm_rdv_rdv_rq *rdv){
    struct nm_rdv *nm_rdv = p_proto->priv;
    tbx_free(nm_rdv->rdv_key, rdv);
}

void
nm_rdv_free_ack(struct nm_proto *p_proto,
                struct nm_rdv_ack_rq *ack){
    struct nm_rdv *nm_rdv = p_proto->priv;
    tbx_free(nm_rdv->ack_key, ack);
}

void
nm_rdv_free_track(struct nm_proto *p_proto,
                  uint8_t trk_id){
    struct nm_rdv *nm_rdv = p_proto->priv;
    nm_rdv->out_req_nb[trk_id]--;
}

int
nm_rdv_rdv_rq_size(void){
    return sizeof(struct nm_rdv_rdv_rq);
}

int
nm_rdv_ack_rq_size(void){
    return sizeof(struct nm_rdv_ack_rq);
}



/* successful outgoing request
 */
int
nm_rdv_out_success (struct nm_proto 	* const p_proto,
                    struct nm_pkt_wrap	*p_pw) {
    return 0;
}

/* failed outgoing request
 */
int
nm_rdv_out_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
    return nm_rdv_out_success (p_proto, p_pw);
}

/* successful incoming request
 */
int
nm_rdv_in_success	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw) {

    return 0;
}

/* failed incoming request
 */
int
nm_rdv_in_failed	(struct nm_proto 	* const p_proto,
                         struct nm_pkt_wrap	*p_pw,
                         int			_err) {
    return nm_rdv_in_success (p_proto, p_pw);
}


/* protocol initialisation
 */
int
nm_rdv_proto_init(struct nm_proto * const p_proto) {
    struct nm_core	 *p_core	= NULL;
    struct nm_rdv	 *p_rdv	= NULL;
    int err;

    p_core	= p_proto->p_core;

    p_rdv	= TBX_MALLOC(sizeof(struct nm_rdv));
    if (!p_rdv) {
        err = -NM_ENOMEM;
        goto out;
    }

    p_rdv->p_sched = p_proto->p_core->p_sched;

    tbx_malloc_init(&p_rdv->rdv_key,
                    sizeof(struct nm_rdv_rdv_rq),
                    NB_RDV_RQ, "rdv_header");

    tbx_malloc_init(&p_rdv->ack_key,
                    sizeof(struct nm_rdv_ack_rq),
                    NB_ACK_RQ, "ack_header");

    p_rdv->waiting_rdv = tbx_slist_nil();
    p_rdv->large_waiting_ack = tbx_slist_nil();

    p_rdv->out_req_nb[0] = 0;
    p_rdv->out_req_nb[1] = 0;
    p_rdv->out_req_nb[2] = 0;
    p_rdv->out_req_nb[3] = 0;

    p_proto->priv = p_rdv;

    if (p_core->p_proto_array[nm_pi_rdv_req]){
        NM_DISPF("nm_basic_proto_init: protocol entry %d already used",
                 nm_pi_rdv_req);
        err	= -NM_EALREADY;
        goto out;
    }

    if (p_core->p_proto_array[nm_pi_rdv_ack]){
        NM_DISPF("nm_basic_proto_init: protocol entry %d already used",
                 nm_pi_rdv_ack);
        err	= -NM_EALREADY;
        goto out;
    }

    p_core->p_proto_array[nm_pi_rdv_req] = p_proto;
    p_core->p_proto_array[nm_pi_rdv_ack] = p_proto;

    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_rdv_load		(struct nm_proto_ops	*p_ops) {
    p_ops->init		= nm_rdv_proto_init;

    p_ops->out_success	= nm_rdv_out_success;
    p_ops->out_failed	= nm_rdv_out_failed;

    p_ops->in_success	= nm_rdv_in_success;
    p_ops->in_failed	= nm_rdv_in_failed;

    return NM_ESUCCESS;
}
