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
#include "nm_so_interfaces.h"
#include "nm_so_sendrecv_interface.h"

#undef NMAD_SO_DEBUG
#if defined(NMAD_SO_DEBUG)
#  define NMAD_SO_TRACE(...) { fprintf(stderr, __VA_ARGS__) ; }
#else
#  define NMAD_SO_TRACE(...) { }
#endif /* NMAD_SO_DEBUG */

/* Status flags contents */
#define NM_SO_STATUS_PACKET_HERE     ((uint8_t)1)
#define NM_SO_STATUS_UNPACK_HERE     ((uint8_t)2)
#define NM_SO_STATUS_RDV_HERE        ((uint8_t)4)
#define NM_SO_STATUS_RDV_IN_PROGRESS ((uint8_t)8)


#define NM_SO_STATUS_SEND_COMPLETED  ((uint8_t)1)
#define NM_SO_STATUS_RECV_COMPLETED  ((uint8_t)2)


struct nm_so_sr_gate {
  /* WARNING: better replace the following array by two separate
     bitmaps, to save space and avoid false sharing between send and
     recv operations. status[tag_id][seq] */
	nm_so_request status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];
//  volatile uint8_t status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];
};

struct nm_so_sched {
  nm_so_strategy *current_strategy;
  struct nm_so_interface_ops *current_interface;

  /* For any source messages */
  struct {
    uint8_t  status;
    void    *data;
    uint32_t len;
  } any_src[NM_SO_MAX_TAGS];

  uint8_t next_gate_id;

  unsigned pending_any_src_unpacks;
};

struct nm_so_gate {

  struct nm_so_sched *p_so_sched;

  /* Actually counts the number of expected small messages, including
     RdV requests for large messages */
  unsigned pending_unpacks;

  unsigned active_recv[NM_SO_MAX_NETS][NM_SO_MAX_TRACKS];
  unsigned active_send[NM_SO_MAX_NETS][NM_SO_MAX_TRACKS];

  /* WARNING: better replace the following array by two separate
     bitmaps, to save space and avoid false sharing between send and
     recv operations. status[tag_id][seq] */
  uint8_t status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];

  uint8_t send_seq_number[NM_SO_MAX_TAGS], recv_seq_number[NM_SO_MAX_TAGS];

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

  /* For large messages waiting for ACKs */
  struct list_head pending_large_send[NM_SO_MAX_TAGS];

  /* For large messages waiting for Track 1 (or 2) to be free */
  struct list_head pending_large_recv;

  void *strat_priv;
  void *interface_private;
};


int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len);

int
__nm_so_unpack_any_src(struct nm_core *p_core,
                       uint8_t tag,
                       void *data, uint32_t len);
int
nm_so_out_process_success_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw);

int
nm_so_out_process_failed_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		         _err);

int
nm_so_out_schedule_gate(struct nm_gate *p_gate);

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
