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
#include "nm_rdv_public.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"


#define NM_SO_PW_BUF_LENGTH 64
#define NM_SO_PW_NB_SEG     32


/********* Main Structures *******/
struct nm_so_pkt_wrap{
    struct nm_pkt_wrap *p_pw;

    /* buffer de 64 octets pour copier les petits */
    uint64_t buf;
    void * buf_cursor;

    /* iovec prédéfini à N entrées*/
    struct iovec v[NM_SO_PW_NB_SEG];


    /*stock de la ref de chaque pw agrégé dans ce wrapper*/
    struct nm_so_pkt_wrap *aggregated_pws[NM_SO_PW_NB_SEG];
    int nb_aggregated_pws;

    /* réception anticipée du global header (stream)
       - stock du nb de seg */
    int nb_seg;
};

/*********************************/


int
nm_so_pw_init(struct nm_core *p_core){
    int err;

    err = nm_so_header_init(p_core);
    nm_so_control_error("nm_header_init", err);

    return err;
}



/* -------------- Allocation/Libération ---------------*/
int
nm_so_pw_alloc(struct nm_core *p_core,
               struct nm_gate *p_gate,
               uint8_t tag_id,
               uint8_t seq,
               struct nm_so_pkt_wrap **pp_so_pw){

    struct nm_so_pkt_wrap *p_so_pw = NULL;
    struct nm_pkt_wrap	  *p_pw = NULL;
    int err;
    int i;


    /* allocate packet wrapper */
    err = nm_pkt_wrap_alloc(p_core, &p_pw, tag_id, seq);
    if (err != NM_ESUCCESS){
        NM_DISPF("nm_pkt_wrap_alloc returned %d", err);
        goto out;
    }
    p_pw->p_gate = p_gate;


    /* allocate main structure */
    p_so_pw = TBX_MALLOC(sizeof(*p_so_pw));
    if(!p_so_pw){
        err = -NM_ENOMEM;
        goto free;
    }

    p_so_pw->p_pw = p_pw;

    for(i = 0; i < NM_SO_PW_NB_SEG; i++)
        p_so_pw->aggregated_pws[i] = NULL;
    p_so_pw->nb_aggregated_pws = 0;



    p_pw->sched_priv = p_so_pw;

    *pp_so_pw = p_so_pw;
    err = NM_ESUCCESS;

 out:
    return err;

 free:
    nm_pkt_wrap_free(p_core, p_pw);
    goto out;
}


int
nm_so_pw_free(struct nm_core *p_core,
              struct nm_so_pkt_wrap *p_so_pw){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    // libération de wrapper
    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    err = nm_pkt_wrap_free(p_core, p_pw);
    nm_so_control_error("nm_pkt_wrap_free", err);


    // libération de la partie spécifique
    TBX_FREE(p_so_pw);

    err = NM_ESUCCESS;
    return err;
}



/* -------------- Ajout d'entrée dans l'iovec ---------------*/
int
nm_so_pw_add_buf(struct nm_core *p_core,
                 struct nm_so_pkt_wrap *p_so_pw,
                 void * data, int len){
    struct nm_trk *trk = NULL;
    int err;

    // si len < NM_SO_PW_BUF_LENGTH et libre

    trk = p_so_pw->p_pw->p_trk;
    switch(trk->cap.iov_type){

    case nm_trk_iov_none :
        // si pas d'extension de faite
        // -> malloc(taille max des unexpected

        // memcpy des données en contigü
        break;

    case nm_trk_iov_both_sym :
        // si plus d'entrées dans l'iovec prédéfini
        // -> realloc

        // ajout dans l'iovec
        break;


    case nm_trk_iov_send_only :
    case nm_trk_iov_recv_only :
    case nm_trk_iov_both_assym  :
    case nm_trk_iov_unspecified :
    default:
        TBX_FAILURE("nm_so_pw_add_buf - iov_type not supported");
    }


    err = NM_ESUCCESS;
    return err;
}


int
nm_so_pw_add_data(struct nm_core *p_core,
                  struct nm_so_pkt_wrap *p_so_pw,
                  int proto_id, int len, int seq, void * data){

    TBX_FAILURE("nm_so_pw_add_data pas implémenter");

    struct nm_so_sched *so_sched = p_core->p_sched->sch_private;
    int err;

#warning Ecriture_directe_si_static
    // si < 64 o ou si pas d'iovec


    // Ajout de l'entête de données
    struct nm_so_header * data_header = NULL;
    err = nm_so_header_alloc_data_header(so_sched,
                                         proto_id,
                                         seq, len,
                                         &data_header);

    /* Ajout de l'entête */
    int size = 0;
    err = nm_so_header_sizeof_header(&size);
    nm_so_control_error("nm_so_header_sizeof_header", err);
    err = nm_so_pw_add_buf(p_core, p_so_pw,
                           data_header, size);
    nm_so_control_error("nm_so_pw_add_buf", err);

    /* Ajout des données */
    err = nm_so_pw_add_buf(p_core, p_so_pw, data, len);
    nm_so_control_error("nm_so_pw_add_buf", err);

    err = NM_ESUCCESS;


    return err;
}


static int
nm_so_pw_add_control(struct nm_core *p_core,
                     struct nm_so_pkt_wrap *p_so_pw,
                     int proto_id, void * control_data, int len){

    struct nm_so_sched *so_sched = p_core->p_sched->sch_private;
    int err;

#warning Ecriture_directe_si_static
    // si < 64 o ou si pas d'iovec

    struct nm_so_header *control_header = NULL;
    err = nm_so_header_alloc_control_header(so_sched,
                                            proto_id,
                                            &control_header);
    nm_so_control_error("nm_so_header_alloc_control_header",err);

    /* Ajout de l'entête */
    int size = 0;
    err = nm_so_header_sizeof_header(&size);
    nm_so_control_error("nm_so_header_sizeof_header", err);
    err = nm_so_pw_add_buf(p_core, p_so_pw, control_header,size);
    nm_so_control_error("nm_so_pw_add_buf", err);

    /* Ajout du rdv */
    err = nm_so_pw_add_buf(p_core, p_so_pw, control_data, len);
    nm_so_control_error("nm_so_pw_add_buf", err);

    err = NM_ESUCCESS;

    return err;
}

int
nm_so_pw_add_rdv(struct nm_core *p_core,
                 struct nm_so_pkt_wrap *p_so_pw,
                 void * control_data, int len){
    int err;

    err = nm_so_pw_add_control(p_core, p_so_pw,
                               nm_pi_rdv_req, control_data, len);
    return err;
}



int
nm_so_pw_add_ack(struct nm_core *p_core,
                 struct nm_so_pkt_wrap *p_so_pw,
                 void * control_data, int len){
    int err;

    err = nm_so_pw_add_control(p_core, p_so_pw,
                               nm_pi_rdv_ack, control_data, len);
    return err;
}


// pour n'envoyer que la global header en éclaireur (stream)!
int
nm_so_pw_add_length_ask(struct nm_core *p_core,
                        struct nm_so_pkt_wrap *p_so_pw){

    int err;
    err = NM_ESUCCESS;

    //iovec déjà formater pour envoyer la gh, donc rien à faire...

    return err;
}








/* Accesseurs */
// ----------------- les GET --------------------------
int
nm_so_pw_get_drv (struct nm_so_pkt_wrap *p_so_pw, struct nm_drv **pp_drv){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *pp_drv = p_pw->p_drv;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_trk (struct nm_so_pkt_wrap *p_so_pw, struct nm_trk **pp_trk){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *pp_trk = p_pw->p_trk;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_gate(struct nm_so_pkt_wrap *p_so_pw, struct nm_gate **pp_gate){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *pp_gate = p_pw->p_gate;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_proto_id(struct nm_so_pkt_wrap *p_so_pw, uint8_t *p_proto_id){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *p_proto_id = p_pw->proto_id;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_seq     (struct nm_so_pkt_wrap *p_so_pw, uint8_t *p_seq){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *p_seq = p_pw->seq;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_length  (struct nm_so_pkt_wrap *p_so_pw, uint64_t *p_length){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *p_length = p_pw->length;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_v_nb    (struct nm_so_pkt_wrap *p_so_pw, uint8_t *p_v_nb){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *p_v_nb = p_pw->v_nb;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_v_size  (struct nm_so_pkt_wrap *p_so_pw, int *p_v_size){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    *p_v_size = p_pw->v_size;

    err = NM_ESUCCESS;
    return err;
}

int nm_so_pw_get_pw  (struct nm_so_pkt_wrap *p_so_pw,
                      struct nm_pkt_wrap **pp_pw){
    int err;

    *pp_pw = p_so_pw->p_pw;

    err = NM_ESUCCESS;
    return err;
}

// récupération du nb de seg qd on a reçu la gh en éclaireur
int
nm_so_pw_get_nb_seg(struct nm_so_pkt_wrap *p_so_pw,
                    uint8_t *nb_seg){
    int err;

    *nb_seg = p_so_pw->nb_seg;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_pw_get_aggregated_pws(struct nm_so_pkt_wrap *p_so_pw,
                            struct nm_so_pkt_wrap **aggregated_pws){
    int err;

    aggregated_pws = p_so_pw->aggregated_pws;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_pw_get_nb_aggregated_pws(struct nm_so_pkt_wrap *p_so_pw,
                               int *nb_aggregated_pws){
    int err;

    *nb_aggregated_pws = p_so_pw->nb_aggregated_pws;

    err = NM_ESUCCESS;
    return err;
};


int
nm_so_pw_get_data(struct nm_so_pkt_wrap *p_so_pw,
                  int idx,
                  void **data){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);


    *data = p_pw->v[idx].iov_base;

    err = NM_ESUCCESS;
    return err;
}


int
nm_so_pw_is_small(struct nm_so_pkt_wrap *p_so_pw,
                  tbx_bool_t *is_small){
    struct nm_trk *p_trk = NULL;
    int max_single_request_length = 0;
    uint64_t so_pw_len = 0;
    int err;

    err = nm_so_pw_get_trk(p_so_pw, &p_trk);
    nm_so_control_error("nm_so_pw_get_trk", err);

    max_single_request_length
        = p_trk->cap.max_single_request_length;

    err = nm_so_pw_get_length(p_so_pw, &so_pw_len);
    nm_so_control_error("nm_so_pw_get_length", err);

    if(so_pw_len <= max_single_request_length){
        *is_small = tbx_true;

    } else {
        *is_small = tbx_false;
    }

    err = NM_ESUCCESS;
    return err;
}


int
nm_so_pw_is_large(struct nm_so_pkt_wrap *p_so_pw,
                  tbx_bool_t *is_large){
    tbx_bool_t is_small = tbx_true;
    int err;

    err = nm_so_pw_is_small(p_so_pw, &is_small);

    *is_large = !is_small;

    err = NM_ESUCCESS;
    return err;
}



// ----------------- les SET --------------------------
int nm_so_pw_config (struct nm_so_pkt_wrap *p_so_pw,
                     struct nm_drv *p_drv,
                     struct nm_trk *p_trk,
                     struct nm_gate *p_gate,
                     struct nm_gate_drv *p_gdrv,
                     struct nm_gate_trk *p_gtrk){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    p_pw->p_drv  = p_drv;
    p_pw->p_trk  = p_trk;
    p_pw->p_gate = p_gate;
    p_pw->p_gdrv = p_gdrv;
    p_pw->p_gtrk = p_gtrk;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_update_global_header(struct nm_so_pkt_wrap *p_so_pw,
                           uint8_t nb_seg, uint32_t len){
    struct nm_so_sched_header *gh = NULL;
    void * data = NULL;
    int err;

    err = nm_so_pw_get_data(p_so_pw, 0, &data);
    nm_so_control_error("nm_so_pw_get_data", err);
    gh = data;

    err = nm_so_header_set_v_nb(gh, nb_seg);
    nm_so_control_error("nm_so_header_set_v_nb", err);

    err = nm_so_header_set_total_len (gh, len);
    nm_so_control_error("nm_so_header_set_len", err);

    err = NM_ESUCCESS;
    return err;
}

// stockage de nb de seg qd on a reçu la gh en éclaireur (stream)
int
nm_so_pw_set_nb_agregated_seg(struct nm_so_pkt_wrap *p_so_pw,
                              uint8_t nb_seg){
    int err;

    p_so_pw->nb_seg = nb_seg;

    err = NM_ESUCCESS;
    return err;
}


// paramétrage de la longueur à recevoir pour une entrée donnée de l'iovec
int
nm_so_pw_set_iov_len(struct nm_so_pkt_wrap *p_so_pw,
                     int idx, int len){

    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);
    nm_so_control_error("nm_so_pw_get_pw", err);

    p_pw->v[idx].iov_len = len;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_pw_add_aggregated_pw(struct nm_so_pkt_wrap *p_so_pw,
                           struct nm_so_pkt_wrap *pw){
    struct nm_so_pkt_wrap **aggregated_pws = NULL;
    int nb_aggregated_pws = 0;
    int err;

    aggregated_pws = p_so_pw->aggregated_pws;
    nb_aggregated_pws = p_so_pw->nb_aggregated_pws;

    aggregated_pws[nb_aggregated_pws] = pw;

    p_so_pw->nb_aggregated_pws++;

    err = NM_ESUCCESS;
    return err;
}



/* --------------- Utilitaires --------------------*/
int
nm_so_search_and_extract_pw(p_tbx_slist_t list,
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
        if(!p_pw)
            TBX_FAILURE("nm_so_search_and_extract_pw - p_pw NULL");

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

int
nm_so_pw_print_wrap(struct nm_so_pkt_wrap *p_so_pw){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_so_pw_get_pw(p_so_pw, &p_pw);

    DISP_PTR("p_pw->p_drv          ", p_pw->p_drv);
    DISP_PTR("p_pw->p_trk          ", p_pw->p_trk);
    DISP_PTR("p_pw->p_gate         ", p_pw->p_gate);
    DISP_PTR("p_pw->p_gdrv         ", p_pw->p_gdrv);
    DISP_PTR("p_pw->p_gtrk         ", p_pw->p_gtrk);
    DISP_PTR("p_pw->p_proto        ", p_pw->p_proto);
    DISP_VAL("p_pw->proto_id       ", p_pw->proto_id);
    DISP_VAL("p_pw->seq            ", p_pw->seq);
    DISP_PTR("p_pw->sched_priv     ", p_pw->sched_priv);
    DISP_PTR("p_pw->drv_priv       ", p_pw->drv_priv);
    DISP_PTR("p_pw->gate_priv      ", p_pw->gate_priv);
    DISP_PTR("p_pw->proto_priv     ", p_pw->proto_priv);
    DISP_VAL("p_pw->pkt_priv_flags ", p_pw->pkt_priv_flags);
    DISP_VAL("p_pw->length         ", p_pw->length);
    DISP_VAL("p_pw->iov_flags      ", p_pw->iov_flags);
    DISP_PTR("p_pw->p_pkt_head     ", p_pw->p_pkt_head);
    DISP_PTR("p_pw->data           ", p_pw->data);
    DISP_PTR("p_pw->len_v          ", p_pw->len_v);
    DISP_VAL("p_pw->iov_priv_flags ", p_pw->iov_priv_flags);
    DISP_VAL("p_pw->v_size         ", p_pw->v_size);
    DISP_VAL("p_pw->v_nb           ", p_pw->v_nb);
    DISP_PTR("p_pw->v              ", p_pw->v);
    DISP_PTR("p_pw->nm_v           ", p_pw->nm_v);

    err = NM_ESUCCESS;
    return err;
}









/* -------------- Interface d'avant à revoir -----------*/
int
nm_so_pw_take_aggregation_pw(struct nm_sched *p_sched,
                             struct nm_gate *p_gate,
                             struct nm_so_pkt_wrap **pp_so_pw){

    struct nm_so_sched *so_sched = p_sched->sch_private;
    struct nm_so_pkt_wrap *p_so_pw = NULL;
    int err;

    err = nm_so_pw_alloc(p_sched->p_core,
                         p_gate,
                         0,
                         0,
                         &p_so_pw);
    if(err != NM_ESUCCESS){
        nm_so_control_error("nm_so_pw_alloc", err);
        goto out;
    }

    // Ajout de l'entête globale
    int size = 0;
    err = nm_so_header_sizeof_sched_header(&size);
    nm_so_control_error("nm_so_header_sizeof_header", err);

    struct nm_so_sched_header *global_header = NULL;
    err = nm_so_header_alloc_sched_header(so_sched,
                                          1, size,
                                          &global_header);
    nm_so_control_error("nm_so_header_alloc_sched_header", err);

    err = nm_so_pw_add_buf(p_sched->p_core, p_so_pw,
                           global_header, size);
    nm_so_control_error("nm_so_pw_add_buf", err);


    err = NM_ESUCCESS;
 out:
    return err;
}

int
nm_so_pw_release_aggregation_pw(struct nm_sched *p_sched,
                                struct nm_so_pkt_wrap *p_so_pw){

    int err;

    err = nm_so_pw_free(p_sched->p_core, p_so_pw);
    if(err != NM_ESUCCESS){
        nm_so_control_error("nm_so_pw_free", err);
        goto out;
    }

    err = NM_ESUCCESS;

 out:
    return err;
}


int
nm_so_pw_take_pre_posted_pw(struct nm_sched *p_sched,
                            struct nm_trk *trk,
                            struct nm_so_pkt_wrap **pp_so_pw){
    struct nm_so_pkt_wrap *p_so_pw = NULL;
    void * buffer = NULL;
    uint64_t buffer_size = 0;
    int err;

    err = nm_so_pw_alloc(p_sched->p_core, 
                         NULL,
                         0,
                         0,
                         &p_so_pw);
    if(err != NM_ESUCCESS){
        nm_so_control_error("nm_so_pw_alloc", err);
        goto out;
    }


    buffer_size = trk->cap.max_single_request_length;

    buffer = TBX_MALLOC(buffer_size);
    if(!buffer){
        nm_so_pw_free(p_sched->p_core, p_so_pw);
        err = -NM_ENOMEM;
        goto out;
    }

    err = nm_so_pw_add_buf(p_sched->p_core, p_so_pw,
                           buffer, buffer_size);
    if(err != NM_ESUCCESS){
        nm_so_control_error("nm_so_pw_add_buf", err);
        nm_so_pw_free(p_sched->p_core, p_so_pw);
        goto out;
    }

    err = NM_ESUCCESS;
 out:
    return err;
}

int
nm_so_pw_release_pre_posted_pw(struct nm_sched *p_sched,
                               struct nm_so_pkt_wrap *p_so_pw){
    int err;

    err = nm_so_pw_free(p_sched->p_core, p_so_pw);
    if(err != NM_ESUCCESS){
        nm_so_control_error("nm_so_pw_free", err);
        goto out;
    }

    err = NM_ESUCCESS;

 out:
    return err;
}

