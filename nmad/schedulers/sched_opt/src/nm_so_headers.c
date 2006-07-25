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

#define NB_CREATED_SCHED_HEADER 10
#define NB_CREATED_HEADER 10

/*********** Headers *************/
struct nm_so_sched_header{
    uint8_t  proto_id;
    uint8_t  nb_seg;
    uint32_t len;
};

struct nm_so_header{
    uint8_t  proto_id;
    uint8_t  seq;
    uint32_t len;
};

/*********************************/

int
nm_so_header_init(struct nm_core *p_core){
    struct nm_so_sched *so_sched = p_core->p_sched->sch_private;
    int err;

    // allocateur rapide des entetes globales et de données
    tbx_malloc_init(&so_sched->sched_header_key,
                    sizeof(struct nm_so_sched_header),
                    NB_CREATED_SCHED_HEADER, "so_sched_header");

    tbx_malloc_init(&so_sched->header_key,
                    sizeof(struct nm_so_header),
                    NB_CREATED_HEADER, "so_header");

    err = NM_ESUCCESS;
    return err;
}




int
nm_so_header_alloc_sched_header(struct nm_so_sched *so_sched,
                                uint8_t nb_seg, uint32_t len,
                                struct nm_so_sched_header **h){
    struct nm_so_sched_header *global_header = NULL;
    int err;

    global_header = tbx_malloc(so_sched->sched_header_key);

    if (!global_header) {
        err = -NM_ENOMEM;
        goto out;
    }

    global_header->proto_id = nm_pi_sched;
    global_header->nb_seg = 1;

    *h = global_header;
    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_header_free_sched_header(struct nm_so_sched *so_sched,
                               struct nm_so_sched_header *header){
    int err;

    tbx_free(so_sched->sched_header_key, header);

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_header_alloc_data_header(struct nm_so_sched *so_sched,
                               uint8_t proto_id,
                               uint8_t seq, uint32_t len,
                               struct nm_so_header **header){
    struct nm_so_header * data_header = NULL;
    int err;


    data_header = tbx_malloc(so_sched->header_key);
    if(!data_header){
        err = -NM_ENOMEM;
        goto out;
    }

    data_header->proto_id = proto_id;
    data_header->seq = seq;
    data_header->len = len;

    *header = data_header;
    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_header_alloc_control_header(struct nm_so_sched *so_sched,
                                  uint8_t proto_id,
                                  struct nm_so_header **header){
    struct nm_so_header *control = NULL;
    int err;

    control = tbx_malloc(so_sched->header_key);
    if(!control){
        err = -NM_ENOMEM;
        goto out;
    }

    control->proto_id = proto_id;

    *header= control;
    err = NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_header_free_header(struct nm_so_sched *so_sched,
                         struct nm_so_header **header){
    int err;

    tbx_free(so_sched->header_key, header);

    err = NM_ESUCCESS;
    return err;
}


int
nm_so_header_get_sched_proto_id(struct nm_so_sched_header *header,
                                uint8_t *proto_id){
    int err;

    *proto_id = header->proto_id;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_header_get_v_nb(struct nm_so_sched_header *header,
                      uint8_t *v_nb){
    int err;

    *v_nb = header->nb_seg;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_header_get_total_len(struct nm_so_sched_header *header,
                           uint32_t *len){
    int err;

    *len = header->len;

    err = NM_ESUCCESS;
    return err;
}

/*********************************/
int
nm_so_header_get_proto_id(struct nm_so_header *header,
                          uint8_t *proto_id){
    int err;

    *proto_id = header->proto_id;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_header_get_seq(struct nm_so_header *header,
                     uint8_t *seq){
    int err;

    *seq = header->seq;

    err = NM_ESUCCESS;
    return err;
}

int
nm_so_header_get_len(struct nm_so_header *header,
                      uint32_t *len){
    int err;

    *len = header->len;

    err = NM_ESUCCESS;
    return err;
}

