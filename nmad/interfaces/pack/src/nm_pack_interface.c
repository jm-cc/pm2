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

#include <Padico/Puk.h>
#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include <nm_pack_interface.h>

static inline void nm_pack_cnx_init(nm_session_t p_session, nm_gate_t gate, nm_tag_t tag, nm_pack_cnx_t*cnx)
{
  cnx->p_session  = p_session;
  cnx->gate       = gate;
  cnx->tag        = tag;
  cnx->nb_packets = 0;
}

int nm_begin_packing(nm_session_t p_session, nm_gate_t gate, nm_tag_t tag, nm_pack_cnx_t*cnx)
{
  nm_pack_cnx_init(p_session, gate, tag, cnx);
  return NM_ESUCCESS;
}

int nm_pack(nm_pack_cnx_t *cnx, const void *data, nm_len_t len)
{
  if(tbx_unlikely(cnx->nb_packets >= NM_PACK_MAX_PENDING - 1))
    nm_flush_packs(cnx);
  return nm_sr_isend(cnx->p_session, cnx->gate, cnx->tag, data, len, &cnx->reqs[cnx->nb_packets++]);
}

int nm_end_packing(nm_pack_cnx_t *cnx)
{
  int err = nm_flush_packs(cnx);
  return err;
}

int nm_cancel_packing(nm_pack_cnx_t *cnx)
{
  return NM_ENOTIMPL;
}

int nm_begin_unpacking(nm_session_t p_session, nm_gate_t gate, nm_tag_t tag, nm_pack_cnx_t *cnx)
{
  nm_pack_cnx_init(p_session, gate, tag, cnx);
  return NM_ESUCCESS;
}

int nm_unpack(nm_pack_cnx_t *cnx, void *data, nm_len_t len)
{
  if(cnx->gate != NM_ANY_GATE)
    {
      if(tbx_unlikely(cnx->nb_packets >= NM_PACK_MAX_PENDING - 1))
	nm_flush_unpacks(cnx);
      nm_sr_irecv(cnx->p_session, cnx->gate, cnx->tag, data, len, &cnx->reqs[cnx->nb_packets++]);
      return NM_ESUCCESS;
    }
  else 
    {
      /* Here, we know that 1) begin_unpacking has been called with
	 ANY_SRC as the gate parameter and 2) this is the first
	 unpack. So we have to perform a synchronous recv in order to wait
	 for the real gate to be known before next unpacks... */
      
      nm_sr_irecv(cnx->p_session, NM_ANY_GATE, cnx->tag, data, len, &cnx->reqs[0]);
      
      int err = nm_sr_rwait(cnx->p_session, &cnx->reqs[0]);
      if(err != NM_ESUCCESS)
	return err;
      
      err = nm_sr_recv_source(cnx->p_session, &cnx->reqs[0], &cnx->gate);
      
      return err;
    }
}

int nm_end_unpacking(nm_pack_cnx_t *cnx)
{
  int err = nm_flush_unpacks(cnx);
  return err;
}

int nm_cancel_unpacking(nm_pack_cnx_t *cnx)
{
  return NM_ENOTIMPL;
}

int nm_flush_packs(nm_pack_cnx_t *cnx)
{
  int i;
  for(i = 0; i < cnx->nb_packets; i++)
    {
      int err = nm_sr_swait(cnx->p_session, &cnx->reqs[i]);
      if(err != NM_ESUCCESS)
	return err;
    }
  cnx->nb_packets = 0;
  return NM_ESUCCESS;
}

int nm_flush_unpacks(nm_pack_cnx_t *cnx)
{
  int i;
  for(i = 0; i < cnx->nb_packets; i++)
    {
      int err = nm_sr_rwait(cnx->p_session, &cnx->reqs[i]);
      if(err != NM_ESUCCESS)
	return err;
    }
  cnx->nb_packets = 0;
  return NM_ESUCCESS;
}

int nm_test_end_packing(nm_pack_cnx_t *cnx)
{
  int i;
  for(i = 0; i < cnx->nb_packets; i++)
    {
      int err = nm_sr_stest(cnx->p_session, &cnx->reqs[i]);
      if(err != NM_ESUCCESS)
	return err;
    }
  return NM_ESUCCESS;
}

int nm_test_end_unpacking(nm_pack_cnx_t *cnx)
{
  int i;
  for(i = 0; i < cnx->nb_packets; i++)
    {
      int err = nm_sr_rtest(cnx->p_session, &cnx->reqs[i]);
      if(err != NM_ESUCCESS)
	return err;
    }
  return NM_ESUCCESS;
}
