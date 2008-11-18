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


#ifndef NM_PRIVATE_H
#define NM_PRIVATE_H

#include <pm2_common.h>
#include <pm2_list.h>

#include "nm_public.h"

#include <Padico/Module.h>

#ifdef PM2_NUIOA
#include <numa.h>
#endif

/* Tracks 
 */

typedef int8_t nm_trk_id_t;

#define NM_TRK_SMALL ((nm_trk_id_t)0)
#define NM_TRK_LARGE ((nm_trk_id_t)1)
#define NM_TRK_NONE  ((nm_trk_id_t)-1)

#define NM_SO_MAX_TRACKS   2


#include "nm_so_parameters.h"
#include "nm_so_headers.h"

#include "nm_drv_cap.h"
#include "nm_trk_cap.h"
#include "nm_drv.h"
#include "nm_gate.h"
#include "nm_tags.h"
#include "nm_pkt_wrap.h"
#include "nm_errno.h"
#include "nm_log.h"
#include "nm_cnx_rq.h"
#ifdef PIOMAN
#include "nm_piom.h"
#endif

#include "nm_so_strategies.h"
#include "nm_so_private.h"
#include "nm_core.h"
#include "nm_event.h"

#include "nm_so_pkt_wrap.h"
#include "nm_core_inline.h"

#ifdef SAMPLING
#include "nm_parser.h"
#endif


int nm_sched_out(struct nm_core *p_core);

int nm_sched_in(struct nm_core *p_core);

int nm_poll_send(struct nm_pkt_wrap *p_pw);

int nm_poll_recv(struct nm_pkt_wrap*p_pw);

/* ** SchedOpt internal functions */

int nm_so_process_unexpected(tbx_bool_t is_any_src, struct nm_gate *p_gate,
			     nm_tag_t tag, uint8_t seq, uint32_t len, void *data);


int nm_so_build_multi_ack(struct nm_gate *p_gate,
                          nm_tag_t tag, uint8_t seq, uint32_t chunk_offset,
                          int nb_drv, nm_drv_id_t *drv_ids, uint32_t *chunk_lens);

int nm_so_rdv_success(tbx_bool_t is_any_src,
                      struct nm_gate *p_gate,
                      nm_tag_t tag, uint8_t seq,
                      uint32_t len,
                      uint32_t chunk_offset,
                      uint8_t is_last_chunk);

int nm_so_init_large_datatype_recv_with_multi_ack(struct nm_pkt_wrap *p_so_pw);


/* ** TEMPORARY: former Sched ops */

/** Initialize the scheduler module.
 */
int nm_so_schedule_init			(struct nm_core *p_core);

/** Shutdown the scheduler module.
 */
int nm_so_schedule_exit			(struct nm_core *p_core);


/** Initialize the scheduler structures for a new gate.
 */
int nm_so_init_gate		(struct nm_core	 *p_core,
				 struct nm_gate	 *p_gate);


/** Clean-up the scheduler structures for a gate.
 */
int nm_so_close_gate		(struct nm_core	 *p_core,
				 struct nm_gate	 *p_gate);


/** Schedule and post new outgoing buffers */
int nm_so_out_schedule_gate	(struct nm_gate *p_gate);


/** Process complete outgoing request */
int nm_so_process_complete_send(struct nm_core	*p_core,
				struct nm_pkt_wrap *p_pw,
				int _err);


/** Process complete incoming request.
 */
int nm_so_process_complete_recv(struct nm_core	*p_core,
				struct nm_pkt_wrap *p_pw,
				int _err);



#endif /* NM_PRIVATE_H */
