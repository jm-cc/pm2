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
#include <nm_rdv_public.h>
#include "nm_so_private.h"

#define CHRONO

static void
print_wrap(struct nm_pkt_wrap* p_pw){
    printf("p_pw->p_drv    = %p\n", p_pw->p_drv);
    printf("p_pw->p_trk    = %p\n", p_pw->p_trk);
    printf("p_pw->p_gate   = %p\n", p_pw->p_gate);
    printf("p_pw->p_gdrv   = %p\n", p_pw->p_gdrv);
    printf("p_pw->p_gtrk   = %p\n", p_pw->p_gtrk);
    printf("p_pw->p_proto  = %p\n", p_pw->p_proto);
    printf("p_pw->proto_id = %d\n", p_pw->proto_id);
    printf("p_pw->seq      = %d\n", p_pw->seq);
    printf("p_pw->sched_priv = %p\n", p_pw->sched_priv);
    printf("p_pw->drv_priv   = %p\n", p_pw->drv_priv);
    printf("p_pw->gate_priv  = %p\n", p_pw->gate_priv);
    printf("p_pw->proto_priv = %p\n", p_pw->proto_priv);
    printf("p_pw->pkt_priv_flags = %d\n", p_pw->pkt_priv_flags);
    printf("p_pw->length         = %lld\n", p_pw->length);
    printf("p_pw->iov_flags  = %d\n", p_pw->iov_flags);
    printf("p_pw->p_pkt_head = %p\n", p_pw->p_pkt_head);
    printf("p_pw->data       = %p\n", p_pw->data);
    printf("p_pw->len_v      = %p\n", p_pw->len_v);
    printf("p_pw->iov_priv_flags = %d\n", p_pw->iov_priv_flags);
    printf("p_pw->v_size     = %d\n", p_pw->v_size);
    printf("p_pw->v_nb       = %d\n", p_pw->v_nb);
    printf("p_pw->v    = %p\n", p_pw->v);
    printf("p_pw->nm_v = %p\n", p_pw->nm_v);
}

static void
nm_so_init_pre_posted(struct nm_core *p_core,
                      struct nm_so_sched *p_priv,
                      int nb_pre_posted){
    struct nm_pkt_wrap * p_pw = NULL;
    int i;

    p_priv->nb_ready_pre_posted_wraps = nb_pre_posted;
    p_priv->ready_pre_posted_wraps
        = nm_so_heap_create(nb_pre_posted);

    for(i = 0; i < nb_pre_posted; i++){
        /* Allocation of the packet wrapper */
        nm_pkt_wrap_alloc(p_core, &p_pw,
                          nm_pi_sched, 0);

        /* Allocation of the iovec */
        nm_iov_alloc(p_core, p_pw, 1);

        /* Allocation of the reception area */
        void * buffer = TBX_MALLOC(AGREGATED_PW_MAX_SIZE);
        nm_iov_append_buf(p_core, p_pw,
                          buffer, AGREGATED_PW_MAX_SIZE);

        nm_so_heap_push(p_priv->ready_pre_posted_wraps, p_pw);
    }
}

static struct nm_pkt_wrap *
nm_so_init_pw(struct nm_core *p_core,
              struct nm_so_sched *p_priv,
              int nb_iov_entries){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    /* allocate packet wrapper */
    err = nm_pkt_wrap_alloc(p_core, &p_pw,
                            nm_pi_sched, 0);
    if (err != NM_ESUCCESS)
        goto out;

    p_pw->v_first = 0;

    /* allocate iovec */
    err = nm_iov_alloc(p_core, p_pw, nb_iov_entries);
    if (err != NM_ESUCCESS) {
        nm_pkt_wrap_free(p_core, p_pw);
        goto out;
    }

    // Ajout de l'entête globale
    nm_so_sched_header_t * global_header =
        tbx_malloc(p_priv->sched_header_key);

    if (!global_header) {
        nm_iov_free(p_core, p_pw);
        nm_pkt_wrap_free(p_core, p_pw);
        goto out;
    }
    global_header->proto_id = nm_pi_sched;
    global_header->nb_seg = 1;
    err = nm_iov_append_buf(p_core, p_pw, global_header,
                            sizeof(nm_so_sched_header_t));
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_iov_append_buf returned %d", err);
    }

    struct nm_pkt_wrap **agregated_pws =
        TBX_MALLOC(nb_iov_entries * sizeof(struct nm_pkt_wrap *));
    int i;
    for(i = 0; i < nb_iov_entries; i++)
        agregated_pws[i] = NULL;

    struct nm_so_pkt_wrap * pw_priv = TBX_MALLOC(sizeof(struct nm_so_pkt_wrap));
    pw_priv->agregated_pws = agregated_pws;
    pw_priv->nb_agregated_pws = 0;

    p_pw->sched_priv = pw_priv;

 out:
    return p_pw;
}

static void
nm_so_init_agregation_pw(struct nm_core *p_core,
                         struct nm_so_sched *p_priv,
                         int nb_pw){
    struct nm_pkt_wrap *p_pw = NULL;
    int i;

    p_priv->nb_ready_wraps = nb_pw;
    p_priv->ready_wraps = nm_so_heap_create(nb_pw);

    for(i = 0; i < nb_pw; i++){
        p_pw = nm_so_init_pw(p_core, p_priv,
                             NB_ENTRIES_IN_AGREGATED_PW);

        nm_so_heap_push(p_priv->ready_wraps, p_pw);
    }
}

static int
nm_so_schedule_init (struct nm_sched *p_sched) {
    struct nm_core *p_core = p_sched->p_core;
    int	err;

    struct nm_so_sched *p_priv = TBX_MALLOC(sizeof(struct nm_so_sched));
    if (!p_priv) {
        err = -NM_ENOMEM;
        goto out;
    }
    memset(p_priv, 0, sizeof(struct nm_so_sched));

    p_priv->trks_to_update = tbx_slist_nil();

    // initialisation des à pré-poster
    nm_so_init_pre_posted(p_core, p_priv, NB_CREATED_PRE_POSTED);


    // allocateur rapide des entetes globales et de données
    tbx_malloc_init(&p_priv->sched_header_key,
                    sizeof(struct nm_so_sched_header),
                    NB_CREATED_SCHED_HEADER, "so_sched_header");

    tbx_malloc_init(&p_priv->header_key,
                    sizeof(struct nm_so_header),
                    NB_CREATED_HEADER, "so_header");

    // initialisation de pack d'agrégation à envoyer
    nm_so_init_agregation_pw(p_core, p_priv, NB_AGREGATION_PW);

    /* Enregistrement de protocole de rdv */
    err = nm_core_proto_init(p_core, nm_rdv_load,
                             &p_priv->p_proto_rdv);
    if (err != NM_ESUCCESS) {
        printf("nm_core_proto_init returned err = %d\n", err);
        goto out;
    }

    p_priv->unexpected_list = tbx_slist_nil();

    p_priv->acks = NULL;

    p_sched->sch_private	= p_priv;

    err	= NM_ESUCCESS;

 out:
    return err;
}

static
int
nm_so_init_trks	(struct nm_sched	*p_sched,
                 struct nm_drv		*p_drv) {
    struct nm_core *p_core	= p_sched->p_core;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    p_tbx_slist_t trks_to_update = so_sched->trks_to_update;
    int	err;

    /* Track 0 - piste réservée aux unexpected */
    struct nm_trk_rq trk_rq_0 = {
        .p_trk	= NULL,
        .cap	= {
            .rq_type			= nm_trk_rq_unspecified,
            .iov_type			= nm_trk_iov_unspecified,
            .max_pending_send_request	= 0,
            .max_pending_recv_request	= 0,
            .min_single_request_length	= 0,
            .max_single_request_length	= 0,
            .max_iovec_request_length	= 0,
            .max_iovec_size		= 0
        },
        .flags	= 0
    };

    err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_0.p_trk);
    if (err != NM_ESUCCESS)
        goto out;

    err = p_drv->ops.open_trk(&trk_rq_0);
    if (err != NM_ESUCCESS) {
        goto out_free;
    }

    if(trk_rq_0.p_trk->cap.rq_type == nm_trk_rq_stream
       || trk_rq_0.p_trk->cap.rq_type == nm_trk_rq_dgram){
        tbx_slist_append(trks_to_update, trk_rq_0.p_trk);
    }

    /* Track 1 - piste pour les longs*/
    struct nm_trk_rq trk_rq_1 = {
        .p_trk	= NULL,
        .cap	= {
            .rq_type			= nm_trk_rq_rdv,
            .iov_type			= nm_trk_iov_unspecified,
            .max_pending_send_request	= 0,
            .max_pending_recv_request	= 0,
            .min_single_request_length	= 0,
            .max_single_request_length	= 0,
            .max_iovec_request_length	= 0,
            .max_iovec_size		= 0
        },
        .flags	= 0
    };

    err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_1.p_trk);
    if (err != NM_ESUCCESS)
        goto out_free;

    err = p_drv->ops.open_trk(&trk_rq_1);
    if (err != NM_ESUCCESS) {
        goto out_free_2;
    }
    trk_rq_1.p_trk->cap.rq_type = nm_trk_rq_rdv;


    /* Track 2 - piste pour les longs*/
    struct nm_trk_rq trk_rq_2 = {
        .p_trk	= NULL,
        .cap	= {
            .rq_type			= nm_trk_rq_rdv,
            .iov_type			= nm_trk_iov_unspecified,
            .max_pending_send_request	= 0,
            .max_pending_recv_request	= 0,
            .min_single_request_length	= 0,
            .max_single_request_length	= 0,
            .max_iovec_request_length	= 0,
            .max_iovec_size		= 0
        },
        .flags	= 0
    };

    err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_2.p_trk);
    if (err != NM_ESUCCESS)
        goto out_free;

    err = p_drv->ops.open_trk(&trk_rq_2);
    if (err != NM_ESUCCESS) {
        goto out_free_3;
    }
    trk_rq_2.p_trk->cap.rq_type = nm_trk_rq_rdv;

    ///* Track 3 - piste pour les longs*/
    //struct nm_trk_rq trk_rq_3 = {
    //    .p_trk	= NULL,
    //    .cap	= {
    //        .rq_type			= nm_trk_rq_rdv,
    //        .iov_type			= nm_trk_iov_unspecified,
    //        .max_pending_send_request	= 0,
    //        .max_pending_recv_request	= 0,
    //        .min_single_request_length	= 0,
    //        .max_single_request_length	= 0,
    //        .max_iovec_request_length	= 0,
    //        .max_iovec_size		= 0
    //    },
    //    .flags	= 0
    //};
    //
    //err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_3.p_trk);
    //if (err != NM_ESUCCESS)
    //    goto out_free;
    //
    //err = p_drv->ops.open_trk(&trk_rq_3);
    //if (err != NM_ESUCCESS) {
    //    goto out_free_4;
    //}
    //trk_rq_3.p_trk->cap.rq_type = nm_trk_rq_rdv;

    err	= NM_ESUCCESS;

 out:
    return err;

// out_free_4:
//    p_core->ops.trk_free(p_core, trk_rq_3.p_trk);

 out_free_3:
    p_core->ops.trk_free(p_core, trk_rq_2.p_trk);

 out_free_2:
    p_core->ops.trk_free(p_core, trk_rq_1.p_trk);

 out_free:
    p_core->ops.trk_free(p_core, trk_rq_0.p_trk);
    goto out;
}

static
int
nm_so_init_gate	(struct nm_sched	*p_sched,
                 struct nm_gate		*p_gate) {
    struct nm_so_gate	*p_gate_priv	=	NULL;
    int	err;

    p_gate_priv	= TBX_MALLOC(sizeof(struct nm_so_gate));
    if (!p_gate_priv) {
        err = -NM_ENOMEM;
        goto out;
    }
    p_gate_priv->next_pw = NULL;

    p_gate->sch_private	= p_gate_priv;

    err	= NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_load		(struct nm_sched_ops	*p_ops) {
    p_ops->init			= nm_so_schedule_init;

    p_ops->init_trks		= nm_so_init_trks;
    p_ops->init_gate		= nm_so_init_gate;

    p_ops->out_schedule_gate	  = nm_so_out_schedule_gate;
    p_ops->out_process_success_rq = nm_so_out_process_success_rq;
    p_ops->out_process_failed_rq  = nm_so_out_process_failed_rq;

    p_ops->in_schedule		 = nm_so_in_schedule;
    p_ops->in_process_success_rq = nm_so_in_process_success_rq;
    p_ops->in_process_failed_rq	 = nm_so_in_process_failed_rq;

    return NM_ESUCCESS;
}

/******* Utilitaires **********/
struct nm_pkt_wrap *
nm_so_take_agregation_pw(struct nm_sched *p_sched){
    struct nm_so_sched *p_priv = p_sched->sch_private;

    //printf("nm_so_take_agregation_pw - heap_pop\n");
    return nm_so_heap_pop(p_priv->ready_wraps);
}

void
nm_so_release_agregation_pw(struct nm_sched *p_sched,
                            struct nm_pkt_wrap *p_pw){
    struct nm_so_sched *p_priv = p_sched->sch_private;
    struct nm_so_pkt_wrap * so_pw = p_pw->sched_priv;

    int i;
    for(i = 1; i < p_pw->v_nb; i++){
        p_pw->v[i].iov_len = 0;
        p_pw->v[i].iov_base = NULL;
    }

    //reset
    nm_so_update_global_header(p_pw, 1, sizeof(struct nm_so_sched_header));
    p_pw->v_nb = 1;
    p_pw->length = sizeof(struct nm_so_sched_header);
    p_pw->p_drv    = NULL;
    p_pw->p_trk    = NULL;
    p_pw->p_gate   = NULL;
    p_pw->p_gdrv   = NULL;
    p_pw->p_gtrk   = NULL;
    p_pw->p_proto  = NULL;
    p_pw->drv_priv   = NULL;
    p_pw->gate_priv  = NULL;
    p_pw->proto_priv = NULL;
    p_pw->pkt_priv_flags = 0;
    p_pw->length         = 8;
    p_pw->iov_flags  = 0;
    p_pw->p_pkt_head = NULL;
    p_pw->len_v      = 0;
    p_pw->iov_priv_flags = 0;
    p_pw->v_nb = 1;
    p_pw->nm_v = NULL;

    so_pw->nb_agregated_pws = 0;

    //printf("nm_so_release_agregation_pw - heap_push\n");

    nm_so_heap_push(p_priv->ready_wraps, p_pw);
}

struct nm_pkt_wrap *
nm_so_take_pre_posted_pw(struct nm_sched *p_sched){
    struct nm_so_sched *p_priv = p_sched->sch_private;

    //printf("nm_so_take_pre_posted_pw - heap_pop\n");

    return nm_so_heap_pop(p_priv->ready_pre_posted_wraps);
}

#ifdef CHRONO
int nb_release_pre_posted = 0;
double release_1_2 = 0.0;
double release_2_4 = 0.0;
#endif

void
nm_so_release_pre_posted_pw(struct nm_sched *p_sched,
                            struct nm_pkt_wrap *p_pw){
#ifdef CHRONO
    tbx_tick_t  t1, t2, t3, t4;
    nb_release_pre_posted++;
    TBX_GET_TICK(t1);
#endif

    struct nm_so_sched *p_priv = p_sched->sch_private;

    p_pw->p_drv    = NULL;
    p_pw->p_trk    = NULL;
    p_pw->p_gate   = NULL;
    p_pw->p_gdrv   = NULL;
    p_pw->p_gtrk   = NULL;
    p_pw->p_proto  = NULL;
    p_pw->sched_priv = NULL;
    p_pw->drv_priv   = NULL;
    p_pw->gate_priv  = NULL;
    p_pw->proto_priv = NULL;
    p_pw->pkt_priv_flags = 0;
    p_pw->length         = 8;
    p_pw->iov_flags  = 0;
    p_pw->p_pkt_head = NULL;
    p_pw->len_v      = 0;
    p_pw->iov_priv_flags = 0;
    p_pw->v_nb = 1;
    p_pw->nm_v = NULL;

#ifdef CHRONO
    TBX_GET_TICK(t2);
#endif

    //printf("nm_so_release_pre_posted_pw - heap_push\n");

    nm_so_heap_push(p_priv->ready_pre_posted_wraps, p_pw);

#ifdef CHRONO
    TBX_GET_TICK(t4);
    release_1_2 += TBX_TIMING_DELAY(t1, t2);
    release_2_4 += TBX_TIMING_DELAY(t2, t4);
#endif
}

void
nm_so_update_global_header(struct nm_pkt_wrap *p_pw,
                           uint8_t nb_seg,
                           uint32_t len){
    struct nm_so_sched_header *gh =
        p_pw->v[0].iov_base;
    gh->nb_seg = nb_seg;
    gh->len = len;
}

void
nm_so_add_data(struct nm_core *p_core,
               struct nm_pkt_wrap *p_pw,
               int proto_id, int len, int seq, void * data){
    struct nm_sched *p_sched = p_core->p_sched;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    int err;

    // Ajout de l'entête de données
    nm_so_header_t * data_header
        = tbx_malloc(so_sched->header_key);
    data_header->proto_id = proto_id;
    data_header->seq = seq;
    data_header->len = len;

    err = nm_iov_append_buf(p_core, p_pw, data_header,
                            sizeof(nm_so_header_t));
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_iov_append_buf returned %d", err);
    }

    /* Ajout des données */
    err = nm_iov_append_buf(p_core, p_pw,
                            data, len);
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_iov_append_buf returned %d", err);
    }
}

struct nm_pkt_wrap *
nm_so_search_and_extract_pw(p_tbx_slist_t list,
                            uint8_t proto_id, uint8_t seq){
    struct nm_pkt_wrap * p_pw = NULL;

    if(!list->length){
        goto end;
    }

    tbx_slist_ref_to_head(list);
    do{
        p_pw = tbx_slist_ref_get(list);
        if(!p_pw)
            FAILURE("nm_so_search_and_extract_pw - p_pw NULL");

        if(p_pw->proto_id == proto_id && p_pw->seq == seq){
            tbx_slist_ref_extract_and_forward(list, &p_pw);
            goto end;
        }
    }while(tbx_slist_ref_forward(list));

    p_pw = NULL;

 end:
    return p_pw;
}
