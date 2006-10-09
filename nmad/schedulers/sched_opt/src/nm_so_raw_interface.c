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

#include <tbx.h>

#include <nm_public.h>
#include "nm_so_private.h"
#include "nm_so_raw_interface.h"

extern int
nm_sched_out_gate(struct nm_gate *);


int
nm_so_stest(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  if(p_so_gate->status[tag][seq] & NM_SO_STATUS_SEND_COMPLETED)
    return NM_ESUCCESS;

  if(p_so_gate->active_recv[0][0] || p_so_gate->active_recv[1][0])
    /* We also need to schedule in new packets (typically ACKs) */
    nm_schedule(p_core);
  else
    /* We just need to schedule out data on this gate */
    nm_sched_out_gate(p_gate);

  return (p_so_gate->status[tag][seq] & NM_SO_STATUS_SEND_COMPLETED) ?
    NM_ESUCCESS : -NM_EAGAIN;
}

int
nm_so_swait(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  while(!(p_so_gate->status[tag][seq] & NM_SO_STATUS_SEND_COMPLETED))

    if(p_so_gate->active_recv[0][0] || p_so_gate->active_recv[1][0])
      /* We also need to schedule in new packets (typically ACKs) */
      nm_schedule(p_core);
    else
      /* We just need to schedule out data on this gate */
      nm_sched_out_gate(p_gate);

  return NM_ESUCCESS;
}

int
nm_so_rtest(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  if(p_so_gate->status[tag][seq] & NM_SO_STATUS_RECV_COMPLETED)
    return NM_ESUCCESS;

  nm_schedule(p_core);

  return (p_so_gate->status[tag][seq] & NM_SO_STATUS_RECV_COMPLETED) ?
    NM_ESUCCESS : -NM_EAGAIN;
}

int
nm_so_rwait(struct nm_core *p_core,
	    struct nm_gate *p_gate,
	    uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  while(!(p_so_gate->status[tag][seq] & NM_SO_STATUS_RECV_COMPLETED))
    nm_schedule(p_core);

  return NM_ESUCCESS;
}
