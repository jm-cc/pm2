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

#ifndef NM_SO_PKT_WRAP_H
#define NM_SO_PKT_WRAP_H

struct nm_so_pkt_wrap;


/* -------------- Allocation/Libération ---------------*/
int
nm_so_pw_alloc(struct nm_core *p_core,
               struct nm_gate *p_gate,
               uint8_t tag_id,
               uint8_t seq,
               struct nm_so_pkt_wrap **pp_so_pw);

int
nm_so_pw_free(struct nm_core *p_core,
              struct nm_so_pkt_wrap *p_so_pw);


/* -------------- Ajout d'entrée dans l'iovec ---------------*/
int
nm_so_pw_add_buf(struct nm_core *p_core,
                 struct nm_so_pkt_wrap *p_so_pw,
                 void * data, int len);

int
nm_so_pw_add_data(struct nm_core *p_core,
                  struct nm_so_pkt_wrap *p_so_pw,
                  int proto_id, int len, int seq, void *data);

int
nm_so_pw_add_rdv(struct nm_core *p_core,
                 struct nm_so_pkt_wrap *p_so_pw,
                 void * control_data, int len);

int
nm_so_pw_add_ack(struct nm_core *p_core,
                 struct nm_so_pkt_wrap *p_so_pw,
                 void * control_data, int len);

// pour n'envoyer que la global header en éclaireur (stream)!
int
nm_so_pw_add_length_ask(struct nm_core *p_core,
                        struct nm_so_pkt_wrap *p_so_pw);






/* Accesseurs */
// ----------------- les GET --------------------------
int nm_so_pw_get_drv (struct nm_so_pkt_wrap *, struct nm_drv **);
int nm_so_pw_get_trk (struct nm_so_pkt_wrap *, struct nm_trk **);
int nm_so_pw_get_gate(struct nm_so_pkt_wrap *, struct nm_gate **);

int nm_so_pw_get_proto_id(struct nm_so_pkt_wrap *, uint8_t *);
int nm_so_pw_get_seq     (struct nm_so_pkt_wrap *, uint8_t *);
int nm_so_pw_get_length  (struct nm_so_pkt_wrap *, uint64_t *);
int nm_so_pw_get_v_nb    (struct nm_so_pkt_wrap *, uint8_t *);
int nm_so_pw_get_v_size  (struct nm_so_pkt_wrap *, int *);

int nm_so_pw_get_pw  (struct nm_so_pkt_wrap *,
                      struct nm_pkt_wrap **);

// récupération du nb de seg qd on a reçu la gh en éclaireur
int
nm_so_pw_get_nb_seg(struct nm_so_pkt_wrap *p_so_pw,

                              uint8_t *nb_seg);
int
nm_so_pw_get_aggregated_pws(struct nm_so_pkt_wrap *p_so_pw,
                            struct nm_so_pkt_wrap **aggregated_pws);
int
nm_so_pw_get_nb_aggregated_pws(struct nm_so_pkt_wrap *p_so_pw,
                               int *nb_aggregated_pws);


int
nm_so_pw_get_data(struct nm_so_pkt_wrap *p_so_pw,
                  int idx,
                  void **data); //p_pw->v[idx].iov_base;


int
nm_so_pw_is_small(struct nm_so_pkt_wrap *p_so_pw, tbx_bool_t *);

int
nm_so_pw_is_large(struct nm_so_pkt_wrap *p_so_pw, tbx_bool_t *);





// ----------------- les SET --------------------------
int nm_so_pw_config (struct nm_so_pkt_wrap *,
                     struct nm_drv *,
                     struct nm_trk *,
                     struct nm_gate *,
                     struct nm_gate_drv *,
                     struct nm_gate_trk *);

int
nm_so_pw_update_global_header(struct nm_so_pkt_wrap *p_so_pw,
                              uint8_t nb_seg, uint32_t len);


// stockage de nb de seg qd on a reçu la gh en éclaireur (stream)
int
nm_so_pw_set_nb_agregated_seg(struct nm_so_pkt_wrap *p_so_pw,
                              uint8_t nb_seg);


// paramétrage de la longueur à recevoir pour une entrée donnée de l'iovec
int
nm_so_pw_set_iov_len(struct nm_so_pkt_wrap *p_so_pw,
                     int idx, int len);

int
nm_so_pw_add_aggregated_pw(struct nm_so_pkt_wrap *p_so_pw,
                           struct nm_so_pkt_wrap *pw);






/* --------------- Utilitaires --------------------*/
int
nm_so_pw_search_and_extract_pw(p_tbx_slist_t list,
                               uint8_t proto_id, uint8_t seq,
                               struct nm_so_pkt_wrap **pp_so_pw);
int
nm_so_pw_print_wrap(struct nm_so_pkt_wrap *p_so_pw);



/* -------------- Interface d'avant à revoir -----------*/
#warning Interface_d_avant_à_revoir

int
nm_so_pw_take_aggregation_pw(struct nm_sched *p_sched,
                             struct nm_gate *p_gate,
                             struct nm_so_pkt_wrap **pp_so_pw);
int
nm_so_pw_release_aggregation_pw(struct nm_sched *p_sched,
                                struct nm_so_pkt_wrap *p_so_pw);
int
nm_so_pw_take_pre_posted_pw(struct nm_sched *p_sched,
                            struct nm_trk *trk,
                            struct nm_so_pkt_wrap **pp_so_pw);
int
nm_so_pw_release_pre_posted_pw(struct nm_sched *p_sched,
                               struct nm_so_pkt_wrap *p_so_pw);

#endif
