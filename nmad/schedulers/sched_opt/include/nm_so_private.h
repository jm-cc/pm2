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

#include "nm_protected.h"
#include "nm_so_heap.h"

#define AGREGATED_PW_MAX_SIZE 32768
#define NB_ENTRIES_IN_AGREGATED_PW 200
#define SMALL_THRESHOLD 32000

#define NB_CREATED_PRE_POSTED 10
#define NB_CREATED_SCHED_HEADER 10
#define NB_CREATED_HEADER 10
#define NB_AGREGATION_PW 10

/*********** Unexpecteds *************/
struct nm_so_unexpected{
    uint8_t  proto_id;
    uint8_t  seq;
    uint32_t len;
    void *data;
};



struct nm_so_gate {
    // next p_pw to schedule - one in advance
    struct nm_pkt_wrap * next_pw;
};

struct nm_so_trk{
    /* pour les stream -> premier envoi */
    //struct nm_pkt_wrap *intro;
    tbx_bool_t pending_stream_intro;
};


struct nm_so_sched {
    struct nm_so_trk so_trks[255];
    p_tbx_slist_t trks_to_update;

    // prêts à être pré-postés en réception
    int nb_ready_pre_posted_wraps;
    nm_so_heap_t ready_pre_posted_wraps;

    // prêts à être utiliser en émission
    int nb_ready_wraps;
    nm_so_heap_t ready_wraps;

    // headers
    p_tbx_memory_t sched_header_key;
    p_tbx_memory_t header_key;

    // protocole de rdv
    struct nm_proto *p_proto_rdv;

    // unexpecteds
    p_tbx_slist_t unexpected_list;

    // ack wrapper en cours de construction
    struct nm_pkt_wrap *acks;
};


struct nm_so_pkt_wrap{
    struct nm_pkt_wrap **agregated_pws;
    int nb_agregated_pws;

    uint8_t  nb_seg;
    //uint32_t len;
};

/*********** Headers *************/
typedef struct nm_so_sched_header{
    uint8_t  proto_id;
    uint8_t  nb_seg;
    uint32_t len;
}nm_so_sched_header_t, *p_nm_so_sched_header_t;

typedef struct nm_so_header{
    uint8_t  proto_id;
    uint8_t  seq;
    uint32_t len;
}nm_so_header_t, *p_nm_so_header_t;


/************ Methods ************/
int
nm_so_out_schedule_gate(struct nm_gate *p_gate);

int
nm_so_out_process_success_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw);

int
nm_so_out_process_failed_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 _err);

int
nm_so_in_schedule(struct nm_sched *p_sched);

int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw);

int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		 _err);



/******* Utilitaires **********/
struct nm_pkt_wrap *
nm_so_take_agregation_pw(struct nm_sched *p_sched);
void
nm_so_release_agregation_pw(struct nm_sched *p_sched,
                            struct nm_pkt_wrap *p_pw);

struct nm_pkt_wrap *
nm_so_take_pre_posted_pw(struct nm_sched *p_sched);
void
nm_so_release_pre_posted_pw(struct nm_sched *p_sched,
                            struct nm_pkt_wrap *p_pw);

void
nm_so_update_global_header(struct nm_pkt_wrap *p_pw,
                           uint8_t nb_seg,
                           uint32_t len);

void
nm_so_add_data(struct nm_core *p_core,
               struct nm_pkt_wrap *p_pw,
               int proto_id, int len, int seq, void * data);

struct nm_pkt_wrap *
nm_so_search_and_extract_pw(p_tbx_slist_t list,
                            uint8_t proto_id, uint8_t seq);



