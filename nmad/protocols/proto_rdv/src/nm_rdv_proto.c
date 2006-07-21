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


#include "nm_protected.h"
#include "nm_rdv_private.h"



#define NB_RDV_RQ 100
#define NB_ACK_RQ 100

#define CHRONO


static void
nm_rdv_control_error(char *fct_name, int err){
    if(err != NM_ESUCCESS){
        NM_DISPF("%s returned %d", fct_name, err);
        TBX_FAILURE("err != NM_ESUCCESS");
    }
}

static int
nm_rdv_get_and_extract(p_tbx_slist_t list,
                       uint8_t proto_id, uint8_t seq,
                       struct nm_pkt_wrap **pp_pw){
    struct nm_pkt_wrap * p_pw = NULL;
    int err;

    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        p_pw = tbx_slist_ref_get(list);

        if(p_pw->proto_id == proto_id && p_pw->seq == seq){
            tbx_slist_ref_extract_and_forward(list, NULL);
            *pp_pw = p_pw;
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

    *pp_pw = NULL;

 end:
    err = NM_ESUCCESS;
    return err;
}

static int
nm_rdv_extract_from_ref(p_tbx_slist_t list,
                        p_tbx_slist_element_t ref,
                        struct nm_pkt_wrap **pp_pw){
    struct nm_pkt_wrap * p_pw = NULL;
    void *ptr = NULL;
    int err;

    list->ref = ref;
    tbx_slist_ref_extract_and_forward(list, &ptr);
    p_pw = ptr;
    assert(p_pw);

    *pp_pw = p_pw;

    err = NM_ESUCCESS;
    return err;
}


static int
nm_rdv_get(p_tbx_slist_t list,
           uint8_t proto_id, uint8_t seq,
           p_tbx_slist_element_t *ref,
           struct nm_pkt_wrap **pp_pw){

    struct nm_pkt_wrap * p_pw = NULL;
    int err;

    *ref = NULL;

    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        p_pw = tbx_slist_ref_get(list);

        if(p_pw->proto_id == proto_id && p_pw->seq == seq){
            *ref = list->ref;
            *pp_pw = p_pw;
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

    *pp_pw = NULL;

 end:
    err = NM_ESUCCESS;
    return err;
}


#ifdef CHRONO
int nb_generate_rdv = 0;
double chrono_generate_rdv = 0;
#endif

int
nm_rdv_generate_rdv(struct nm_proto *p_proto,
                    struct nm_pkt_wrap *p_pw,
                    struct nm_rdv_rdv_rq **pp_rdv_rq){
    struct nm_rdv * nm_rdv = p_proto->priv;
    int err;

#ifdef CHRONO
    tbx_tick_t t1, t2;

    TBX_GET_TICK(t1);
#endif

    // génére la demande de rdv
    struct nm_rdv_rdv_rq *p_rdv_rq
        = tbx_malloc(nm_rdv->rdv_key);
    if(!p_rdv_rq){
        err = -NM_ENOMEM;
        goto out;
    }

    p_rdv_rq->proto_id = p_pw->proto_id;
    p_rdv_rq->seq = p_pw->seq;
    p_rdv_rq->len = p_pw->length;

    // stocke le pw associé
    p_tbx_slist_t large_waiting_ack = nm_rdv->large_waiting_ack;
    tbx_slist_append(large_waiting_ack, p_pw);

    // retourne la demande de rdv
    *pp_rdv_rq = p_rdv_rq;

#ifdef CHRONO
     TBX_GET_TICK(t2);
     chrono_generate_rdv += TBX_TIMING_DELAY(t1, t2);
     nb_generate_rdv++;
#endif

     err = NM_ESUCCESS;
 out:
     return err;
}

#ifdef CHRONO
int    nb_give_free_track = 0;
double chrono_give_free_track = 0;
 tbx_tick_t t111, t222;
#endif

static int
nm_rdv_give_free_track(struct nm_rdv *nm_rdv,
                       struct nm_gate_drv *p_gdrv) {
    struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[1];
    int track_id = -1;

#ifdef CHRONO

    TBX_GET_TICK(t111);
#endif

    /* all the track sending slots are currently in use */
    if (p_gtrk->p_trk->out_req_nb + nm_rdv->out_req_nb[1] < p_gtrk->p_trk->cap.max_pending_send_request){
        nm_rdv->out_req_nb[1]++;
        //printf("Je donne le endpoint n° 1\n");
        track_id = 1;
        goto end;
    }

    p_gtrk = p_gdrv->p_gate_trk_array[2];
    if (p_gtrk->p_trk->out_req_nb + nm_rdv->out_req_nb[2] < p_gtrk->p_trk->cap.max_pending_send_request){
        nm_rdv->out_req_nb[2]++;
        //printf("Je donne le endpoint n° 2\n");
        track_id = 2;
        goto end;
    }

    //p_gtrk = p_gdrv->p_gate_trk_array[3];
    //if (p_gtrk->p_trk->out_req_nb + nm_rdv->out_req_nb[3] < p_gtrk->p_trk->cap.max_pending_send_request){
    //    //printf("Je donne le endpoint n° 3\n");
    //    nm_rdv->out_req_nb[3]++;
    //    return 3;
    //}

    //printf("Tous les endpoints sont occupés!!\n");


 end:

#ifdef CHRONO
    TBX_GET_TICK(t222);
    chrono_give_free_track += TBX_TIMING_DELAY(t111, t222);
    nb_give_free_track++;
#endif


    return track_id;
}

#ifdef CHRONO
int    nb_treat_rdv_rq = 0;
double chrono_treat_rdv_rq = 0;

int    nb_get = 0;
double chrono_get = 0;

int    nb_give_track = 0;
double chrono_give_track_deb = 0;
double chrono_give_track = 0;
double chrono_give_track_fin = 0;

int    nb_get_and_extract = 0;
double chrono_get_and_extract = 0;

int    nb_create_ack = 0;
double chrono_create_ack = 0;

int    nb_stock_rdv = 0;
double chrono_stcok_rdv = 0;

tbx_tick_t t333, t444;
#endif


int
nm_rdv_treat_rdv_rq(struct nm_proto *p_proto,
                    struct nm_rdv_rdv_rq *p_rdv_rq,
                    struct nm_rdv_ack_rq **pp_ack){
    uint8_t  proto_id = p_rdv_rq->proto_id;
    uint8_t  seq  = p_rdv_rq->seq;

    struct nm_pkt_wrap * large_pw = NULL;

    struct nm_rdv *nm_rdv= p_proto->priv;
    struct nm_sched *p_sched = nm_rdv->p_sched;
    int err;

#ifdef CHRONO
    tbx_tick_t t1, t2;
    tbx_tick_t t3, t4, t7, t8, t9, t10, t11;
    TBX_GET_TICK(t1);
#endif

    //if(proto_id == nm_pi_sched){ // rdv sur des données agrégées
    //    //printf("Allocation d'une zone intermédiaire\n");//???
    //    ;
    //
    //} else

    if(proto_id >= 127){

        p_tbx_slist_element_t ref = NULL;

#ifdef CHRONO
        TBX_GET_TICK(t3);
#endif
        //large_pw = get(p_sched->submit_aux_recv_req,
        //               proto_id, seq);
        err = nm_rdv_get(p_sched->submit_aux_recv_req,
                         proto_id, seq, &ref, &large_pw);
        nm_rdv_control_error("nm_rdv_get", err);


#ifdef CHRONO
        TBX_GET_TICK(t4);
        nb_get++;
        chrono_get += TBX_TIMING_DELAY(t3, t4);
#endif


        // si le unpack est prêt
        if(large_pw){
            /* je mets le wrap sur la piste des larges */
            struct nm_gate_drv *p_gdrv = large_pw->p_gate->p_gate_drv_array[0];

#ifdef CHRONO
            TBX_GET_TICK(t333);
#endif
            // recupère l'id de la 1ere piste libre
            int track_id = nm_rdv_give_free_track(nm_rdv, p_gdrv);

#ifdef CHRONO
             TBX_GET_TICK(t444);
             nb_give_track++;
             chrono_give_track_deb += TBX_TIMING_DELAY(t333, t111);
             chrono_give_track += TBX_TIMING_DELAY(t333, t444);
             chrono_give_track_fin += TBX_TIMING_DELAY(t222, t444);
#endif


            if(track_id != -1){ // une est libre
                struct nm_gate_trk *p_gtrk = p_gdrv->p_gate_trk_array[track_id];
                large_pw->p_drv  = p_gdrv->p_drv;
                large_pw->p_trk  = p_gtrk->p_trk;
                large_pw->p_gdrv = p_gdrv;
                large_pw->p_gtrk = p_gtrk;

#ifdef CHRONO
                TBX_GET_TICK(t7);
#endif
                //large_pw = get_and_extract(p_sched->submit_aux_recv_req,
                //                           proto_id, seq);

                err = nm_rdv_extract_from_ref(p_sched->submit_aux_recv_req, ref, &large_pw);
                nm_rdv_control_error("nm_rdv_extract_from_ref returned %d", err);

#ifdef CHRONO
                TBX_GET_TICK(t8);
                nb_get_and_extract++;
                chrono_get_and_extract += TBX_TIMING_DELAY(t7, t8);
#endif

                // création de l'ack
                struct nm_rdv_ack_rq *p_ack
                    = tbx_malloc(nm_rdv->ack_key);
                if(!p_ack){
                    err = -NM_ENOMEM;
                    goto end;
                }

                p_ack->proto_id = proto_id;
                p_ack->seq = seq;
                p_ack->track_id = track_id;
                *pp_ack = p_ack;
                //printf("-->création de l'ack\n");

                //printf("-->dépot de la réceptiondu large \n");
                // initialisation de la réception
                tbx_slist_append(p_sched->post_aux_recv_req,
                                 large_pw);
#ifdef CHRONO
                TBX_GET_TICK(t9);
                nb_create_ack++;
                chrono_create_ack += TBX_TIMING_DELAY(t8, t9);
#endif
                goto end;
            } else {
                //printf("-->stockage du rdv\n");
#ifdef CHRONO
                TBX_GET_TICK(t10);
#endif
                struct nm_rdv *nm_rdv = p_proto->priv;
                struct nm_rdv_rdv_rq *p_rdv_cpy = TBX_MALLOC(sizeof(struct nm_rdv_rdv_rq));
                memcpy(p_rdv_cpy, p_rdv_rq, sizeof(struct nm_rdv_rdv_rq));
                tbx_slist_append(nm_rdv->waiting_rdv, p_rdv_cpy);

#ifdef CHRONO
                TBX_GET_TICK(t11);
                nb_stock_rdv++;
                chrono_stcok_rdv += TBX_TIMING_DELAY(t10, t11);
#endif


            }
        }

    } else {
        TBX_FAILURE("nm_rdv_treat_rdv_rq - incorrect proto_id");
    }


    err = NM_ESUCCESS;
 end:
#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_treat_rdv_rq += TBX_TIMING_DELAY(t1, t2);
    nb_treat_rdv_rq++;
#endif

    return err;
}

#ifdef CHRONO
int    nb_treat_ack_rq = 0;
double chrono_treat_ack_rq = 0;
#endif


int
nm_rdv_treat_ack_rq(struct nm_proto *p_proto,
                    struct nm_rdv_ack_rq *p_ack){
    struct nm_rdv *nm_rdv           = p_proto->priv;
    p_tbx_slist_t large_waiting_ack = nm_rdv->large_waiting_ack;
    struct nm_pkt_wrap * large_pw   = NULL;

    uint8_t proto_id = p_ack->proto_id;
    uint8_t seq       = p_ack->seq;
    uint8_t track_id = p_ack->track_id;

    int err;

#ifdef CHRONO
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
#endif


    err = nm_rdv_get_and_extract(large_waiting_ack, proto_id, seq,
                                 &large_pw);
    if(err != NM_ESUCCESS)
        TBX_FAILURE("Large non retrouvé");

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

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_treat_ack_rq += TBX_TIMING_DELAY(t1, t2);
    nb_treat_ack_rq++;
#endif

    err = NM_ESUCCESS;
    return err;
}

#ifdef CHRONO
int    nb_treat_waiting_rdv = 0;
double chrono_treat_waiting_rdv = 0;
#endif

int
nm_rdv_treat_waiting_rdv(struct nm_proto *p_proto,
                         struct nm_rdv_ack_rq **pp_ack,
                         struct nm_gate **pp_gate){
    struct nm_rdv *nm_rdv = p_proto->priv;
    p_tbx_slist_t waiting_rdv = nm_rdv->waiting_rdv;
    struct nm_rdv_rdv_rq * rdv = NULL;
    struct nm_pkt_wrap * large_pw = NULL;

    uint8_t  proto_id = 0;
    uint8_t  seq  = 0;

    struct nm_sched *p_sched = nm_rdv->p_sched;
    struct nm_gate *p_gate = NULL;

    p_tbx_slist_element_t ref = NULL;
    int err;

#ifdef CHRONO
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
#endif

    if(!waiting_rdv->length)
        goto end;

    tbx_slist_ref_to_head(waiting_rdv);
    do{
        rdv = tbx_slist_ref_get(waiting_rdv);

        proto_id = rdv->proto_id;
        seq  = rdv->seq;

        err = nm_rdv_get(p_sched->submit_aux_recv_req,
                         proto_id, seq, &ref, &large_pw);
        nm_rdv_control_error("nm_rdv_get", err);

        // si le unpack est prêt
        if(large_pw){
            DISP("LA");

            // initialisation de la réception
            p_gate = large_pw->p_gate;
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
                                              NULL);

            err = nm_rdv_extract_from_ref(p_sched->submit_aux_recv_req, ref, &large_pw);
            nm_rdv_control_error("nm_rdv_extract_from_ref", err);

            // création de l'ack
            struct nm_rdv_ack_rq *p_ack
                = tbx_malloc(nm_rdv->ack_key);
            if(!p_ack){
                err = -NM_ENOMEM;
                goto end;
            }

            p_ack->proto_id = proto_id;
            p_ack->seq = seq;
            p_ack->track_id = track_id;
            //printf("p_ack = %p\n", p_ack);
            *pp_ack = p_ack;

            tbx_slist_append(p_sched->post_aux_recv_req,
                             large_pw);

            p_gate = large_pw->p_gate;
            *pp_gate = p_gate;
            goto end;
        }

    }while(tbx_slist_ref_forward(waiting_rdv));
 end:

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_treat_waiting_rdv += TBX_TIMING_DELAY(t1, t2);
    nb_treat_waiting_rdv++;
#endif

    err = NM_ESUCCESS;
    return err;
}

#ifdef CHRONO
int    nb_free_rdv = 0;
double chrono_free_rdv = 0;
#endif

int
nm_rdv_free_rdv(struct nm_proto *p_proto,
                struct nm_rdv_rdv_rq *rdv){
#ifdef CHRONO
    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
#endif
    int err;

    struct nm_rdv *nm_rdv = p_proto->priv;
    tbx_free(nm_rdv->rdv_key, rdv);

#ifdef CHRONO
    TBX_GET_TICK(t2);
    chrono_free_rdv += TBX_TIMING_DELAY(t1, t2);
    nb_free_rdv++;
#endif

    err = NM_ESUCCESS;
    return err;
}

int
nm_rdv_free_ack(struct nm_proto *p_proto,
                struct nm_rdv_ack_rq *ack){
    int err;

    struct nm_rdv *nm_rdv = p_proto->priv;
    tbx_free(nm_rdv->ack_key, ack);

    err = NM_ESUCCESS;
    return err;
}

int
nm_rdv_free_track(struct nm_proto *p_proto,
                  uint8_t trk_id){
    int err;

    struct nm_rdv *nm_rdv = p_proto->priv;
    nm_rdv->out_req_nb[trk_id]--;

    err = NM_ESUCCESS;
    return err;
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
