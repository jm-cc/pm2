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

#ifndef _NM_SO_TRACKS_
#define _NM_SO_TRACKS_

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include <nm_public.h>
#include "nm_gate.h"
#include "nm_sched.h"
#include "nm_so_private.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_parameters.h"
#include "nm_so_debug.h"

static __inline__
void
_nm_so_post_recv(struct nm_gate *p_gate, struct nm_so_pkt_wrap *p_so_pw,
		 int track_id, int drv_id)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_pw->pw.p_gate = p_gate;

  /* Packet is assigned to given track */
  p_so_pw->pw.p_drv = (p_so_pw->pw.p_gdrv =
		       p_gate->p_gate_drv_array[drv_id])->p_drv;
  p_so_pw->pw.p_trk = (p_so_pw->pw.p_gtrk
		       = p_so_pw->pw.p_gdrv->p_gate_trk_array[track_id])->p_trk;

  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->p_sched->post_aux_recv_req, &p_so_pw->pw);

  p_so_gate->active_recv[drv_id][track_id] = 1;
}

static __inline__
int
nm_so_post_regular_recv(struct nm_gate *p_gate, int drv_id)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER |
		       NM_SO_DATA_PREPARE_RECV,
		       &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_recv(p_gate, p_so_pw, TRK_SMALL, drv_id);

  err = NM_ESUCCESS;
 out:
  return err;
}

static __inline__
int
nm_so_refill_regular_recv(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int err = NM_ESUCCESS;

#ifdef CONFIG_MULTI_RAIL
  {
    int nb_drivers = p_gate->p_sched->p_core->nb_drivers;
    int drv;

    for(drv = 0; drv < nb_drivers; drv++)
      if(!p_so_gate->active_recv[drv][TRK_SMALL])
	err = nm_so_post_regular_recv(p_gate, drv);
  }
#else
  if(p_so_gate->active_recv[NM_SO_DEFAULT_NET][TRK_SMALL] == 0)
    err = nm_so_post_regular_recv(p_gate, NM_SO_DEFAULT_NET);
#endif

  return err;
}


static __inline__
int
nm_so_post_large_recv(struct nm_gate *p_gate, int drv_id,
		      uint8_t tag, uint8_t seq,
		      void *data, uint32_t len)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw = NULL;

  err = nm_so_pw_alloc_and_fill_with_data(tag, seq,
                                          data, len,
                                          0, 0, NM_SO_DATA_DONT_USE_HEADER,
                                          &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_recv(p_gate, p_so_pw, TRK_LARGE, drv_id);

  err = NM_ESUCCESS;
 out:
  return err;
}

static __inline__
int
nm_so_direct_post_large_recv(struct nm_gate *p_gate, int drv_id,
                             struct nm_so_pkt_wrap *p_so_pw)
{
  int err;

  _nm_so_post_recv(p_gate, p_so_pw, TRK_LARGE, drv_id);

  err = NM_ESUCCESS;
  return err;
}



static __inline__
void
_nm_so_post_send(struct nm_gate *p_gate,
		 struct nm_so_pkt_wrap *p_so_pw,
		 int track_id, int drv_id)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_pw->pw.p_gate = p_gate;

  NM_SO_TRACE_LEVEL(3, "Packet posted on track %d\n", track_id);

  /* Packet is assigned to given track */
  p_so_pw->pw.p_drv = (p_so_pw->pw.p_gdrv =
		       p_gate->p_gate_drv_array[drv_id])->p_drv;
  p_so_pw->pw.p_trk = (p_so_pw->pw.p_gtrk
		       = p_so_pw->pw.p_gdrv->p_gate_trk_array[track_id])->p_trk;

  /* append pkt to scheduler post list */
  //#warning vérifier le nb max de requetes concurrentes autorisées!!!!(dans nm_trk_cap.h -> max_pending_send_request)

  tbx_slist_append(p_gate->post_sched_out_list, &p_so_pw->pw);

  p_so_gate->active_send[drv_id][track_id]++;
}

#endif
