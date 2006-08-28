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


#ifndef _NM_SO_PRIVATE_H_
#define _NM_SO_PRIVATE_H_

#include <stdint.h>

#include <pm2_list.h>

#include <nm_protected.h>
#include "nm_so_parameters.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_strategies.h"

extern nm_so_strategy *active_strategy;


/* Status flags contents */
#define NM_SO_STATUS_SEND_COMPLETED  ((uint8_t)1)
#define NM_SO_STATUS_RECV_COMPLETED  ((uint8_t)2)
#define NM_SO_STATUS_PACKET_HERE     ((uint8_t)4)
#define NM_SO_STATUS_UNPACK_HERE     ((uint8_t)8)


#define NM_SO_MAX_TRACKS   3

struct nm_so_gate {
  /* list of raw outgoing packets */
  struct list_head out_list;

  /* Actually counts the number of expected small messages, including
     RdV requests for large messages */
  unsigned pending_unpacks;

  unsigned active_recv[NM_SO_MAX_TRACKS];

  /* WARNING: better replace the following array by two separate
     bitmaps, to save space and avoid false sharing between send and
     recv operations. status[tag_id][seq] */
  volatile uint8_t status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];

  union recv_info {
    struct {
      void *data;
      struct nm_so_pkt_wrap *p_so_pw;
    } pkt_here;
    struct {
      void *data;
      uint32_t len;
    } unpack_here;
  } recv[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];

};

struct nm_so_sched {
  int dummy;
};


int
nm_so_out_schedule_gate(struct nm_gate *p_gate);

int
nm_so_out_process_success_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw);

int
nm_so_out_process_failed_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		         _err);

int
nm_so_in_schedule(struct nm_sched *p_sched);

int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw);

int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		         _err);


#endif
