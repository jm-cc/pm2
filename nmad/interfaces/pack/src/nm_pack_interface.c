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

#include <nm_private.h>
#include <nm_sendrecv_interface.h>

#include "nm_pack_interface.h"

#define NM_PACK_MAX_PENDING 256
#define NM_PACK_REQS_PREALLOC 16

static p_tbx_memory_t nm_pack_mem = NULL;

static inline void nm_pack_cnx_init(nm_core_t p_core, nm_gate_t gate, nm_tag_t tag, nm_pack_cnx_t*cnx)
{
  if(tbx_unlikely(nm_pack_mem == NULL))
    {
      tbx_malloc_init(&nm_pack_mem,  NM_PACK_MAX_PENDING * sizeof(nm_sr_request_t), NM_PACK_REQS_PREALLOC, "nmad/pack/requests"); 
    }
  cnx->reqs = tbx_malloc(nm_pack_mem);
  cnx->p_core     = p_core;
  cnx->gate       = gate;
  cnx->tag        = tag;
  cnx->nb_packets = 0;
}

int nm_begin_packing(nm_core_t p_core,
		     nm_gate_t gate, nm_tag_t tag,
		     nm_pack_cnx_t *cnx)
{
  nm_pack_cnx_init(p_core, gate, tag, cnx);
  return NM_ESUCCESS;
}

int nm_pack(nm_pack_cnx_t *cnx, const void *data, uint32_t len)
{
  if(tbx_unlikely(cnx->nb_packets >= NM_PACK_MAX_PENDING - 1))
    nm_flush_packs(cnx);
  return nm_sr_isend(cnx->p_core, cnx->gate, cnx->tag, data, len, &cnx->reqs[cnx->nb_packets++]);
}

int nm_end_packing(nm_pack_cnx_t *cnx)
{
  int err = nm_flush_packs(cnx);
  tbx_free(nm_pack_mem, cnx->reqs);
  return err;
}

int nm_cancel_packing(nm_pack_cnx_t *cnx)
{
  return NM_ENOTIMPL;
}

int nm_begin_unpacking(nm_core_t p_core,
		       nm_gate_t gate, nm_tag_t tag,
		       nm_pack_cnx_t *cnx)
{
  nm_pack_cnx_init(p_core, gate, tag, cnx);
  return NM_ESUCCESS;
}

int nm_unpack(nm_pack_cnx_t *cnx, void *data, uint32_t len)
{
  if(cnx->gate != NM_ANY_GATE)
    {
      if(tbx_unlikely(cnx->nb_packets >= NM_PACK_MAX_PENDING - 1))
	nm_flush_unpacks(cnx);
      return nm_sr_irecv(cnx->p_core, cnx->gate, cnx->tag, data, len, &cnx->reqs[cnx->nb_packets++]);
    }
  else 
    {
      /* Here, we know that 1) begin_unpacking has been called with
	 ANY_SRC as the gate parameter and 2) this is the first
	 unpack. So we have to perform a synchronous recv in order to wait
	 for the real gate to be known before next unpacks... */
      
      int err = nm_sr_irecv(cnx->p_core, NM_ANY_GATE, cnx->tag, data, len, &cnx->reqs[0]);
      if(err != NM_ESUCCESS)
	return err;
      
      err = nm_sr_rwait(cnx->p_core, &cnx->reqs[0]);
      if(err != NM_ESUCCESS)
	return err;
      
      err = nm_sr_recv_source(cnx->p_core, &cnx->reqs[0], &cnx->gate);
      
      return err;
    }
}

int nm_end_unpacking(nm_pack_cnx_t *cnx)
{
  int err = nm_flush_unpacks(cnx);
  tbx_free(nm_pack_mem, cnx->reqs);
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
      int err = nm_sr_swait(cnx->p_core, &cnx->reqs[i]);
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
      int err = nm_sr_rwait(cnx->p_core, &cnx->reqs[i]);
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
      int err = nm_sr_stest(cnx->p_core, &cnx->reqs[i]);
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
      int err = nm_sr_rtest(cnx->p_core, &cnx->reqs[i]);
      if(err != NM_ESUCCESS)
	return err;
    }
  return NM_ESUCCESS;
}
