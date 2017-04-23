/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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

#include "nm_private_config.h"
#include <Padico/Puk.h>
#include <tbx.h>
#ifdef PIOMAN
#include <pioman.h>
#endif

#if defined(PUKABI)
#include <Padico/Puk-ABI.h>
#define NM_SYS(SYMBOL) PUK_ABI_FSYS_WRAP(SYMBOL)
#else  /* PUKABI */
#define NM_SYS(SYMBOL) SYMBOL
#endif /* PUKABI */

#ifndef NMAD
#error "NMAD flag not set while including nm_private.h."
#endif /* NMAD */

#include <nm_public.h>
#include <nm_core_interface.h>
#include <nm_log.h>

#include <nm_parameters.h>

#include <Padico/Module.h>

#ifdef PM2_TOPOLOGY
#include <tbx_topology.h>
#endif

/* ** Tracks ************************************************/

typedef int8_t nm_trk_id_t;

#define NM_TRK_SMALL ((nm_trk_id_t)0)
#define NM_TRK_LARGE ((nm_trk_id_t)1)
#define NM_TRK_NONE  ((nm_trk_id_t)-1)

#define NM_SO_MAX_TRACKS   2

/* ** Sequence numbers ************************************* */

/** Reserved sequence number used before an unpack has been matched. */
#define NM_SEQ_NONE ((nm_seq_t)0)

/** First sequence number */
#define NM_SEQ_FIRST ((nm_seq_t)1)

/** largest sequence number */
#define NM_SEQ_MAX ((nm_seq_t)-1)

/** Compute next sequence number */
static inline nm_seq_t nm_seq_next(nm_seq_t seq)
{
  assert(seq != NM_SEQ_NONE);
  seq++;
  if(seq == NM_SEQ_NONE)
    seq++;
  return seq;
}

/** compare to seq numbers, assuming they are in the future
 * returns -1 if seq1 is before seq2, 0 if equal, 1 if seq1 after seq2 */
static inline int nm_seq_compare(nm_seq_t current, nm_seq_t seq1, nm_seq_t seq2)
{
  assert(seq1 != current);
  assert(seq2 != current);
  const nm_seq_t seq1_abs = (seq1 > current) ? (seq1 - current) : (seq1 - current - 1);
  const nm_seq_t seq2_abs = (seq2 > current) ? (seq2 - current) : (seq2 - current - 1);
  if(seq1_abs < seq2_abs)
    return -1;
  else if(seq1_abs > seq2_abs)
    return 1;
  else
    return 0;
}

/* ** Profiling ******************************************** */

#ifdef NMAD_PROFILE
#define nm_profile_inc(COUNTER) do { __sync_fetch_and_add(&COUNTER, 1); } while(0)
#else
#define nm_profile_inc(COUNTER) do {} while(0)
#endif

/* ** Drivers ********************************************** */

typedef uint16_t nm_drv_id_t;

/* ** Events *********************************************** */

PUK_VECT_TYPE(nm_core_monitor, struct nm_core_monitor_s*);

/** a pending event, not dispatched immediately because it was received out of order */
PUK_LIST_TYPE(nm_core_pending_event,
	      struct nm_core_event_s event;             /**< the event to dispatch */
	      struct nm_core_monitor_s*p_core_monitor;  /**< the monitor to fire (event matching already checked) */
	      );

/** an event ready for dispatch (matching already done) */
struct nm_core_dispatching_event_s
{
  struct nm_core_event_s event;
  struct nm_monitor_s*p_monitor;
};
PUK_LFQUEUE_TYPE(nm_core_dispatching_event, struct nm_core_dispatching_event_s*, NULL, 1024);
PUK_ALLOCATOR_TYPE(nm_core_dispatching_event, struct nm_core_dispatching_event_s);


#include "nm_trk_cap.h"
#include "nm_tags.h"
#include "nm_pkt_wrap.h"
#include "nm_minidriver.h"
#include "nm_drv.h"
#include "nm_log.h"
#ifdef PIOMAN
#include "nm_piom_ltasks.h"
#endif

#include "nm_headers.h"
#include "nm_strategy.h"
#include "nm_gate.h"
#include "nm_core.h"

#include "nm_lock.h"
#include "nm_core_inline.h"
#include "nm_tactics.h"

#include "nm_sampling.h"

__PUK_SYM_INTERNAL
void nm_core_driver_flush(struct nm_core *p_core);

__PUK_SYM_INTERNAL
void nm_core_driver_exit(struct nm_core *p_core);

__PUK_SYM_INTERNAL
void nm_core_status_event(nm_core_t p_core, const struct nm_core_event_s*const p_event, struct nm_req_s*p_req);

__PUK_SYM_INTERNAL
void nm_core_events_dispatch(struct nm_core*p_core);

__PUK_SYM_INTERNAL
void nm_unexpected_clean(struct nm_core*p_core);

__PUK_SYM_INTERNAL
void nm_core_progress(struct nm_core*p_core);

__PUK_SYM_INTERNAL
void nm_drv_refill_recv(nm_drv_t p_drv, nm_gate_t p_gate);

__PUK_SYM_INTERNAL
void nm_pw_post_send(struct nm_pkt_wrap_s*p_pw);

__PUK_SYM_INTERNAL
void nm_pw_poll_send(struct nm_pkt_wrap_s *p_pw);

__PUK_SYM_INTERNAL
int nm_pw_post_recv(struct nm_pkt_wrap_s*p_pw);

__PUK_SYM_INTERNAL
int nm_pw_poll_recv(struct nm_pkt_wrap_s*p_pw);

__PUK_SYM_INTERNAL
void nm_out_prefetch(struct nm_core*p_core);

/** Process a complete successful outgoing request.
 */
__PUK_SYM_INTERNAL
void nm_pw_process_complete_send(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw);

/** Process complete incoming request.
 */
__PUK_SYM_INTERNAL
void nm_pw_process_complete_recv(struct nm_core*p_core, struct nm_pkt_wrap_s*p_pw);

__PUK_SYM_INTERNAL
void nm_data_pkt_pack(struct nm_pkt_wrap_s*p_pw, nm_core_tag_t tag, nm_seq_t seq,
		      const struct nm_data_s*p_data, nm_len_t chunk_offset, nm_len_t chunk_len, uint8_t flags);

__PUK_SYM_INTERNAL
void nm_data_pkt_unpack(const struct nm_data_s*p_data, const struct nm_header_pkt_data_s*h, const struct nm_pkt_wrap_s*p_pw,
			nm_len_t chunk_offset, nm_len_t chunk_len);

#endif /* NM_PRIVATE_H */
