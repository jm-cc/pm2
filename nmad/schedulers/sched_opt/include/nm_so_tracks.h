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
#include "nm_so_private.h"
#include "nm_so_pkt_wrap.h"

static __inline__
void
_nm_so_post_recv(struct nm_gate *p_gate, struct nm_so_pkt_wrap *p_so_pw,
		 int track_id)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_pw->pw.p_gate = p_gate;

  /* Packet is assigned to given track */
  p_so_pw->pw.p_drv = (p_so_pw->pw.p_gdrv =
		       p_gate->p_gate_drv_array[0])->p_drv;
  p_so_pw->pw.p_trk = (p_so_pw->pw.p_gtrk
		       = p_so_pw->pw.p_gdrv->p_gate_trk_array[track_id])->p_trk;

  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->p_sched->post_aux_recv_req, &p_so_pw->pw);

  p_so_gate->active_recv[track_id] = 1;
}

static __inline__
int
nm_so_post_regular_recv(struct nm_gate *p_gate)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER |
		       NM_SO_DATA_PREPARE_RECV,
		       &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_recv(p_gate, p_so_pw, 0);

  err = NM_ESUCCESS;
 out:
  return err;
}

#ifdef NM_SO_OPTIMISTIC_RECV
static __inline__
int
nm_so_post_optimistic_recv(struct nm_gate *p_gate,
			   uint8_t tag, uint8_t seq,
			   void *data, uint32_t len)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc_optimistic(tag, seq, data, len, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_recv(p_gate, p_so_pw, 0);

  err = NM_ESUCCESS;
 out:
  return err;
}
#endif


static __inline__
int
nm_so_post_large_recv(struct nm_gate *p_gate,
		      uint8_t tag, uint8_t seq,
		      void *data, uint32_t len)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw = NULL;

  err = nm_so_pw_alloc_and_fill_with_data(tag, seq,
                                          data, len,
                                          NM_SO_DATA_DONT_USE_HEADER,
                                          &p_so_pw);

  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_recv(p_gate, p_so_pw, 1);

  err = NM_ESUCCESS;
 out:
  return err;
}


static __inline__
int
nm_so_direct_post_large_recv(struct nm_so_pkt_wrap *p_so_pw)
{
  int err;
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;

  _nm_so_post_recv(p_gate, p_so_pw, 1);

  err = NM_ESUCCESS;
  return err;
}



static __inline__
void
_nm_so_post_send(struct nm_gate *p_gate,
		 struct nm_so_pkt_wrap *p_so_pw,
		 uint8_t track_id)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_pw->pw.p_gate = p_gate;

  /* Packet is assigned to given track */
  p_so_pw->pw.p_drv = (p_so_pw->pw.p_gdrv =
		       p_gate->p_gate_drv_array[0])->p_drv;
  p_so_pw->pw.p_trk = (p_so_pw->pw.p_gtrk
		       = p_so_pw->pw.p_gdrv->p_gate_trk_array[track_id])->p_trk;

  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->post_sched_out_list, &p_so_pw->pw);

  p_so_gate->active_send[track_id]++;
}

#endif
