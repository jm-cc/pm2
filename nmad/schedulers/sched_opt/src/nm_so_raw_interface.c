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
#include "nm_so_raw_interface_private.h"

#define NM_SO_STATUS_SEND_COMPLETED  ((uint8_t)1)
#define NM_SO_STATUS_RECV_COMPLETED  ((uint8_t)2)

struct nm_so_ri_gate {
  /* WARNING: better replace the following array by two separate
     bitmaps, to save space and avoid false sharing between send and
     recv operations. status[tag_id][seq] */
  volatile uint8_t status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];

};

static int nm_so_ri_init_gate(struct nm_gate *p_gate);
static int nm_so_ri_pack_success(struct nm_gate *p_gate,
                                 uint8_t tag, uint8_t seq);
static int nm_so_ri_unpack_success(struct nm_gate *p_gate,
                                   uint8_t tag, uint8_t seq);

struct nm_so_interface_ops ri_ops = {
  .init_gate = nm_so_ri_init_gate,
  .pack_success = nm_so_ri_pack_success,
  .unpack_success = nm_so_ri_unpack_success,
  .unexpected = NULL,
};

/* User interface */

int
nm_so_ri_init(struct nm_core *p_core,
	      struct nm_so_interface **p_so_interface)
{
  struct nm_so_interface *p_so_int = NULL;

#ifndef CONFIG_SCHED_OPT
  return -NM_EINVAL;
#endif

  p_so_int = TBX_MALLOC(sizeof(struct nm_so_interface));
  if(p_so_interface == NULL)
    return -NM_ENOMEM;

  /* Initialize shorcuts pointers */
  p_so_int->p_core = p_core;
  p_so_int->p_so_sched = p_core->p_sched->sch_private;

  /* Fill-in scheduler callbacks */
  p_so_int->p_so_sched->current_interface = &ri_ops;

  *p_so_interface = p_so_int;

  return NM_ESUCCESS;
}

/* Send operations */

int
nm_so_ri_isend(struct nm_so_interface *p_so_interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate = p_so_gate->interface_private;
  uint8_t seq;
  volatile uint8_t *p_req;

  seq = p_so_gate->send_seq_number[tag]++;

  p_req = &p_ri_gate->status[tag][seq];

  *p_req &= ~NM_SO_STATUS_SEND_COMPLETED;

  if(p_request)
    *p_request = (intptr_t)p_req;

  return p_so_interface->p_so_sched->current_strategy->pack(p_gate,
							    tag, seq,
							    data, len);
}

int
nm_so_ri_stest(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  volatile uint8_t *p_request = (uint8_t *)request;

  if(*p_request & NM_SO_STATUS_SEND_COMPLETED)
    return NM_ESUCCESS;

  nm_schedule(p_core);

  return (*p_request & NM_SO_STATUS_SEND_COMPLETED) ?
    NM_ESUCCESS : -NM_EAGAIN;
}

int
nm_so_ri_swait(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  uint8_t *p_request = (uint8_t *)request;

  while(!(*p_request & NM_SO_STATUS_SEND_COMPLETED))
    nm_schedule(p_core);

  return NM_ESUCCESS;
}

int
nm_so_ri_swait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long seq_sup)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf, limit = seq_sup;

  do {

    nm_so_ri_swait(p_so_interface,
		   (nm_so_request)&p_ri_gate->status[tag][seq]);

  } while(seq++ != limit);

  return NM_ESUCCESS;
}

/* Receive operations */

int
nm_so_ri_irecv(struct nm_so_interface *p_so_interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate = p_so_gate->interface_private;
  uint8_t seq;
  volatile uint8_t *p_req;

  seq = p_so_gate->recv_seq_number[tag]++;

  p_req = &p_ri_gate->status[tag][seq];

  *p_req &= ~NM_SO_STATUS_RECV_COMPLETED;

  if(p_request)
    *p_request = (intptr_t)p_req;

  return __nm_so_unpack(p_gate, tag, seq, data, len);
}

int
nm_so_ri_rtest(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  uint8_t *p_request = (uint8_t *)request;

  if(*p_request & NM_SO_STATUS_RECV_COMPLETED)
    return NM_ESUCCESS;

  nm_schedule(p_core);

  return (*p_request & NM_SO_STATUS_RECV_COMPLETED) ?
    NM_ESUCCESS : -NM_EAGAIN;
}

int
nm_so_ri_rwait(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  uint8_t *p_request = (uint8_t *)request;

  while(!(*p_request & NM_SO_STATUS_RECV_COMPLETED))
    nm_schedule(p_core);

  return NM_ESUCCESS;
}

int
nm_so_ri_rwait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long seq_sup)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf, limit = seq_sup;

  do {

    nm_so_ri_rwait(p_so_interface,
		   (nm_so_request)&p_ri_gate->status[tag][seq]);

  } while(seq++ != limit);

  return NM_ESUCCESS;
}


unsigned long
nm_so_ri_get_current_send_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  return p_so_gate->send_seq_number[tag];
}

unsigned long
nm_so_ri_get_current_recv_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  return p_so_gate->recv_seq_number[tag];
}


/* Callback functions */
static
int nm_so_ri_init_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate;

  p_ri_gate = TBX_MALLOC(sizeof(struct nm_so_ri_gate));
  if(p_ri_gate == NULL)
    return -NM_ENOMEM;

  p_so_gate->interface_private = p_ri_gate;

  return NM_ESUCCESS;
}

static
int nm_so_ri_pack_success(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate = p_so_gate->interface_private;

  p_ri_gate->status[tag][seq] |= NM_SO_STATUS_SEND_COMPLETED;

  return NM_ESUCCESS;
}

static
int nm_so_ri_unpack_success(struct nm_gate *p_gate,
                            uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_ri_gate *p_ri_gate = p_so_gate->interface_private;

  p_ri_gate->status[tag][seq] |= NM_SO_STATUS_RECV_COMPLETED;

  return NM_ESUCCESS;
}
