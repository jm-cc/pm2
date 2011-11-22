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

#include <Padico/Module.h>

#ifdef PM2_NUIOA
#include <numa.h>
#endif

/* ** Tracks */
typedef int8_t nm_trk_id_t;

#define NM_TRK_SMALL ((nm_trk_id_t)0)
#define NM_TRK_LARGE ((nm_trk_id_t)1)
#define NM_TRK_NONE  ((nm_trk_id_t)-1)

#define NM_SO_MAX_TRACKS   2

/* ** Sequence numbers */

/** Reserved sequence number used before an unpack has been matched. */
#define NM_SEQ_NONE ((nm_seq_t)0)

/** First sequence number */
#define NM_SEQ_FIRST ((nm_seq_t)1)

/** Compute next sequence number */
static inline nm_seq_t nm_seq_next(nm_seq_t seq)
{
  if((seq + 1) == NM_SEQ_NONE)
    return NM_SEQ_FIRST;
  else
    return seq + 1;
}

/** Compute previous sequence number */
static inline nm_seq_t nm_seq_prev(nm_seq_t seq)
{
  if((seq - 1) == NM_SEQ_NONE)
    return seq - 2;
  else
    return seq - 1;
}


/* ** Drivers */

typedef uint16_t nm_drv_id_t;


#include "nm_so_parameters.h"

#include "nm_trk_cap.h"
#include "nm_tags.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_errno.h"
#include "nm_log.h"
#ifdef PIOMAN
#include "nm_piom_ltasks.h"
#endif

#include "nm_so_strategies.h"
#include "nm_so_headers.h"
#include "nm_gate.h"
#include "nm_core.h"

#include "nm_lock.h"
#include "nm_core_inline.h"
#include "nm_lock_inline.h"
#include "nm_tactics.h"

#include "nm_sampling.h"

TBX_INTERNAL int nm_core_driver_exit(struct nm_core *p_core);

TBX_INTERNAL void nm_unexpected_clean(struct nm_core*p_core);

TBX_INTERNAL void nm_try_and_commit(struct nm_core *p_core);
TBX_INTERNAL void nm_drv_post_send(struct nm_drv *p_drv);
TBX_INTERNAL void nm_drv_poll_send(struct nm_drv *p_drv);

TBX_INTERNAL void nm_drv_refill_recv(struct nm_drv* p_drv);
TBX_INTERNAL void nm_drv_post_recv(struct nm_drv*p_drv);
TBX_INTERNAL void nm_drv_poll_recv(struct nm_drv *p_drv);

TBX_INTERNAL void nm_pw_post_send(struct nm_pkt_wrap*p_pw);
TBX_INTERNAL int  nm_pw_poll_send(struct nm_pkt_wrap *p_pw);
TBX_INTERNAL int  nm_pw_poll_recv(struct nm_pkt_wrap*p_pw);

TBX_INTERNAL void nm_out_prefetch(struct nm_core*p_core);

TBX_INTERNAL void nm_drv_post_all(struct nm_drv*p_drv);


/* ** SchedOpt internal functions */

TBX_INTERNAL int nm_so_rdv_success(struct nm_core*p_core, struct nm_unpack_s*unpack,
				   uint32_t len, uint32_t chunk_offset);

TBX_INTERNAL int nm_so_process_large_pending_recv(struct nm_gate*p_gate);


/** Process complete incoming request.
 */
TBX_INTERNAL int nm_so_process_complete_recv(struct nm_core	*p_core,
					     struct nm_pkt_wrap *p_pw);


/** Compute the cumulated size of an iovec.
 */
static inline int nm_so_iov_len(const struct iovec *iov, int nb_entries)
{
  uint32_t len = 0;
  int i;
  for(i = 0; i < nb_entries; i++)
    {
      len += iov[i].iov_len;
    }
  return len;
}

#endif /* NM_PRIVATE_H */
