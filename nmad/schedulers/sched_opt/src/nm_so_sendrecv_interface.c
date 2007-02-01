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
#include "nm_so_sendrecv_interface.h"
#include "nm_so_sendrecv_interface_private.h"

#define NM_SO_STATUS_SEND_COMPLETED  ((uint8_t)1)
#define NM_SO_STATUS_RECV_COMPLETED  ((uint8_t)2)

struct nm_so_sr_gate {
  /* WARNING: better replace the following array by two separate
     bitmaps, to save space and avoid false sharing between send and
     recv operations. status[tag_id][seq] */
  volatile uint8_t status[NM_SO_MAX_TAGS][NM_SO_PENDING_PACKS_WINDOW];
};

struct any_src_status {
  uint8_t status;
  long gate_id;
  int is_first_request;
};

#define outer_any_src_struct(p_status) \
	((struct any_src_status *)((uint8_t *)(p_status) - \
         (unsigned long)(&((struct any_src_status *)0)->status)))


static struct any_src_status any_src[NM_SO_MAX_TAGS];

static int nm_so_sr_init_gate(struct nm_gate *p_gate);
static int nm_so_sr_exit_gate(struct nm_gate *p_gate);
static int nm_so_sr_pack_success(struct nm_gate *p_gate,
                                 uint8_t tag, uint8_t seq);
static int nm_so_sr_unpack_success(struct nm_gate *p_gate,
                                   uint8_t tag, uint8_t seq,
                                   tbx_bool_t any_src);

/** Send/recv API.
 */
struct nm_so_interface_ops sr_ops = {
  .init_gate = nm_so_sr_init_gate,
  .exit_gate = nm_so_sr_exit_gate,
  .pack_success = nm_so_sr_pack_success,
  .unpack_success = nm_so_sr_unpack_success,
};

/* User interface */

/** Initialize the send/receive interface.
 */
int
nm_so_sr_init(struct nm_core *p_core,
	      struct nm_so_interface **p_so_interface)
{
  struct nm_so_interface *p_so_int = NULL;
  int i;

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
  p_so_int->p_so_sched->current_interface = &sr_ops;

  *p_so_interface = p_so_int;

  for(i=0 ; i<NM_SO_MAX_TAGS ; i++) {
    any_src[i].is_first_request = 1;
  }

  return NM_ESUCCESS;
}

/** Shutdown the send/receive interface.
 */
int
nm_so_sr_exit(struct nm_so_interface *p_so_interface)
{
  TBX_FREE(p_so_interface);
  p_so_interface = NULL;
  return NM_ESUCCESS;
}


/* Send operations */

int
nm_so_sr_isend(struct nm_so_interface *p_so_interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq;
  volatile uint8_t *p_req;

  seq = p_so_gate->send_seq_number[tag]++;

  p_req = &p_sr_gate->status[tag][seq];

  *p_req &= ~NM_SO_STATUS_SEND_COMPLETED;

  if(p_request)
    *p_request = (intptr_t)p_req;

  return p_so_interface->p_so_sched->current_strategy->pack(p_gate,
							    tag, seq,
							    data, len);
}

int
nm_so_sr_stest(struct nm_so_interface *p_so_interface,
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

extern int
nm_so_sr_stest_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb) {
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;
  int ret = NM_EAGAIN;

  while(nb--) {

    ret = nm_so_sr_stest(p_so_interface,
                         (nm_so_request)&p_sr_gate->status[tag][seq]);
    if (ret == -NM_EAGAIN) {
      return ret;
    }
    seq++;
  }

  return ret;
}

int
nm_so_sr_swait(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  uint8_t *p_request = (uint8_t *)request;

  while(!(*p_request & NM_SO_STATUS_SEND_COMPLETED))
    nm_schedule(p_core);

  return NM_ESUCCESS;
}

int
nm_so_sr_swait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;

  while(nb--) {

    nm_so_sr_swait(p_so_interface,
		   (nm_so_request)&p_sr_gate->status[tag][seq]);

    seq++;
  }

  return NM_ESUCCESS;
}

/* Receive operations */

int
nm_so_sr_irecv(struct nm_so_interface *p_so_interface,
	       long gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  volatile uint8_t *p_req = NULL;

  if(gate_id == -1) {

    if (any_src[tag].is_first_request == 0 && !(any_src[tag].status & NM_SO_STATUS_RECV_COMPLETED)) {
      NMAD_SO_TRACE("Irecv not completed for ANY_SRC tag=%d\n", tag);
      nm_so_sr_rwait(p_so_interface, (intptr_t) &any_src[tag].status);
    }

    any_src[tag].is_first_request = 0;
    any_src[tag].status &= ~NM_SO_STATUS_RECV_COMPLETED;

    if(p_request) {
      p_req = &any_src[tag].status;
      *p_request = (intptr_t)p_req;
    }

    NMAD_SO_TRACE("calling __nm_so_unpack_any_src request = %p\n", *p_request);
    return __nm_so_unpack_any_src(p_core, tag, data, len);
  } else {
    struct nm_gate *p_gate = p_core->gate_array + gate_id;
    struct nm_so_gate *p_so_gate = p_gate->sch_private;
    struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
    uint8_t seq;

    seq = p_so_gate->recv_seq_number[tag]++;

    p_req = &p_sr_gate->status[tag][seq];

    *p_req &= ~NM_SO_STATUS_RECV_COMPLETED;

    if(p_request)
      *p_request = (intptr_t)p_req;

    NMAD_SO_TRACE("IRECV: seq = %d, request = %p\n", seq, p_request);
    return __nm_so_unpack(p_gate, tag, seq, data, len);
  }
}

int
nm_so_sr_rtest(struct nm_so_interface *p_so_interface,
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

extern int
nm_so_sr_rtest_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb) {
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;
  int ret = NM_EAGAIN;

  while(nb--) {

    ret = nm_so_sr_rtest(p_so_interface,
                         (nm_so_request)&p_sr_gate->status[tag][seq]);
    if (ret == -NM_EAGAIN) {
      return ret;
    }
    seq++;
  }

  return ret;
}

int
nm_so_sr_rwait(struct nm_so_interface *p_so_interface,
	       nm_so_request request)
{
  struct nm_core *p_core = p_so_interface->p_core;
  volatile uint8_t *p_request = (uint8_t *)request;

#ifdef NMAD_SO_DEBUG
  if (*p_request & NM_SO_STATUS_RECV_COMPLETED) {
    NMAD_SO_TRACE("request %p completed\n", p_request);
  }
  else {
    NMAD_SO_TRACE("request %p not completed\n", p_request);
  }
#endif

  while(!(*p_request & NM_SO_STATUS_RECV_COMPLETED))
    nm_schedule(p_core);

  return NM_ESUCCESS;
}

int
nm_so_sr_recv_source(struct nm_so_interface *p_so_interface,
                     nm_so_request request, long *gate_id)
{
  struct any_src_status *p_status = outer_any_src_struct(request);

  if(gate_id)
    *gate_id = p_status->gate_id;

  return NM_ESUCCESS;
}

int
nm_so_sr_probe(struct nm_so_interface *p_so_interface,
               long gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  uint8_t seq = p_so_gate->recv_seq_number[tag];

  volatile uint8_t *status = &(p_so_gate->status[tag][seq]);

  return ((*status & NM_SO_STATUS_PACKET_HERE) || (*status & NM_SO_STATUS_RDV_HERE)) ?
    NM_ESUCCESS : -NM_EAGAIN;
}

int
nm_so_sr_rwait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;
  uint8_t seq = seq_inf;

  while(nb--) {

    nm_so_sr_rwait(p_so_interface,
		   (nm_so_request)&p_sr_gate->status[tag][seq]);

    seq++;
  }

  return NM_ESUCCESS;
}


unsigned long
nm_so_sr_get_current_send_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  return p_so_gate->send_seq_number[tag];
}

unsigned long
nm_so_sr_get_current_recv_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag)
{
  struct nm_core *p_core = p_so_interface->p_core;
  struct nm_gate *p_gate = p_core->gate_array + gate_id;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  return p_so_gate->recv_seq_number[tag];
}


/* Callback functions */
static
int nm_so_sr_init_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate;

  p_sr_gate = TBX_MALLOC(sizeof(struct nm_so_sr_gate));
  if(p_sr_gate == NULL)
    return -NM_ENOMEM;

  p_so_gate->interface_private = p_sr_gate;

  return NM_ESUCCESS;
}

static
int nm_so_sr_exit_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;

  TBX_FREE(p_sr_gate);
  p_so_gate->interface_private = NULL;
  return NM_ESUCCESS;
}

static
int nm_so_sr_pack_success(struct nm_gate *p_gate,
                          uint8_t tag, uint8_t seq)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;

  p_sr_gate->status[tag][seq] |= NM_SO_STATUS_SEND_COMPLETED;

  return NM_ESUCCESS;
}

static
int nm_so_sr_unpack_success(struct nm_gate *p_gate,
                            uint8_t tag, uint8_t seq,
                            tbx_bool_t is_any_src)
{
  if(!is_any_src) {
    struct nm_so_gate *p_so_gate = p_gate->sch_private;
    struct nm_so_sr_gate *p_sr_gate = p_so_gate->interface_private;

    p_sr_gate->status[tag][seq] |= NM_SO_STATUS_RECV_COMPLETED;

    NMAD_SO_TRACE("data received for request = %p\n", &p_sr_gate->status[tag][seq]);

  } else {

    any_src[tag].gate_id = p_gate->id;
    any_src[tag].status |= NM_SO_STATUS_RECV_COMPLETED;

    NMAD_SO_TRACE("data received for ANY_SRC request = %p\n", &any_src[tag].status);

  }

  return NM_ESUCCESS;
}
