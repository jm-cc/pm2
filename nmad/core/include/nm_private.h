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

#ifdef PIOMAN
#include "pioman.h"
#endif
#include <pm2_common.h>

#include <nm_public.h>
#include <nm_log.h>

#include <Padico/Module.h>

#ifdef PM2_NUIOA
#include <numa.h>
#endif

#if(!defined(PIOMAN) || defined(PIOM_POLLING_DISABLED))
/* use nmad progression */
#define NMAD_POLL 1
#else
/* use pioman as a progression engine */
#define PIOMAN_POLL 1
#endif
/** Gate */
typedef int nm_gate_id_t;

#define NUMBER_OF_GATES          255


/* Tracks */
typedef int8_t nm_trk_id_t;

#define NM_TRK_SMALL ((nm_trk_id_t)0)
#define NM_TRK_LARGE ((nm_trk_id_t)1)
#define NM_TRK_NONE  ((nm_trk_id_t)-1)

#define NM_SO_MAX_TRACKS   2

#include "nm_so_parameters.h"

#include "nm_drv_cap.h"
#include "nm_trk_cap.h"
#include "nm_tags.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_errno.h"
#include "nm_log.h"
#ifdef PIOMAN
#include "nm_piom.h"
#include "nm_piom_ltasks.h"
#endif

#include "nm_so_strategies.h"
#include "nm_so_private.h"
#include "nm_so_headers.h"
#include "nm_gate.h"
#include "nm_core.h"
#include "nm_event.h"

#include "nm_so_pkt_wrap.h"
#include "nm_lock.h"
#include "nm_core_inline.h"
#include "nm_lock_inline.h"

#include "nm_sampling.h"

int nm_sched_out(struct nm_core *p_core);

int nm_sched_in(struct nm_core *p_core);

int nm_poll_send(struct nm_pkt_wrap *p_pw);

int nm_poll_recv(struct nm_pkt_wrap*p_pw);

/* ** SchedOpt internal functions */

int nm_so_rdv_success(struct nm_core*p_core, struct nm_unpack_s*unpack,
                      uint32_t len, uint32_t chunk_offset);

int nm_so_process_large_pending_recv(struct nm_gate*p_gate);


/* ** TEMPORARY: former Sched ops */

/** Initialize the scheduler module.
 */
int nm_so_schedule_init			(struct nm_core *p_core);

/** Shutdown the scheduler module.
 */
int nm_so_schedule_exit			(struct nm_core *p_core);

/** Process complete incoming request.
 */
int nm_so_process_complete_recv(struct nm_core	*p_core,
				struct nm_pkt_wrap *p_pw);


int nm_post_send(struct nm_pkt_wrap*p_pw);

int nm_core_disable_progression(struct nm_core*p_core);

int nm_core_enable_progression(struct nm_core*p_core);

#endif /* NM_PRIVATE_H */
