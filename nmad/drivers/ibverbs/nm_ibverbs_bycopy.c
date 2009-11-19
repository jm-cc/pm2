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

/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */

#include "nm_ibverbs.h"

/* *** method: 'bycopy' ************************************ */

#define NM_IBVERBS_BYCOPY_BLOCKSIZE 8192
#define NM_IBVERBS_BYCOPY_RBUF_NUM  8

#define NM_IBVERBS_BYCOPY_BUFSIZE     (NM_IBVERBS_BYCOPY_BLOCKSIZE - sizeof(struct nm_ibverbs_bycopy_header))
#define NM_IBVERBS_BYCOPY_SBUF_NUM    2
#define NM_IBVERBS_BYCOPY_CREDITS_THR ((NM_IBVERBS_BYCOPY_RBUF_NUM / 2) + 1)

#define NM_IBVERBS_BYCOPY_STATUS_EMPTY   0x00  /**< no message in buffer */
#define NM_IBVERBS_BYCOPY_STATUS_DATA    0x01  /**< data in buffer (sent by copy) */
#define NM_IBVERBS_BYCOPY_STATUS_LAST    0x02  /**< last data block */
#define NM_IBVERBS_BYCOPY_STATUS_CREDITS 0x04  /**< message contains credits */

struct nm_ibverbs_bycopy_header 
{
  uint16_t offset;         /**< data offset (packet_size = BUFSIZE - offset) */
  uint8_t  ack;            /**< credits acknowledged */
  volatile uint8_t status; /**< binary mask- describes the content of the message */
}  __attribute__((packed));

/** An "on the wire" packet for 'bycopy' method */
struct nm_ibverbs_bycopy_packet 
{
  char data[NM_IBVERBS_BYCOPY_BUFSIZE];
  struct nm_ibverbs_bycopy_header header;
} __attribute__((packed));

/** Connection state for tracks sending by copy
 */
struct nm_ibverbs_bycopy
{
  struct {
    struct nm_ibverbs_bycopy_packet sbuf[NM_IBVERBS_BYCOPY_SBUF_NUM];
    struct nm_ibverbs_bycopy_packet rbuf[NM_IBVERBS_BYCOPY_RBUF_NUM];
    volatile uint16_t rack, sack; /**< field for credits acknowledgement */
  } buffer;
  
  struct {
    uint32_t next_out;  /**< next sequence number for outgoing packet */
    uint32_t credits;   /**< remaining credits for sending */
    uint32_t next_in;   /**< cell number of next expected packet */
    uint32_t to_ack;    /**< credits not acked yet by the receiver */
  } window;
  
  struct {
    int done;           /**< size of data received so far */
    int msg_size;       /**< size of whole message */
    void*buf_posted;    /**< buffer posted for receive */
    int size_posted;    /**< length of above buffer */
  } recv;
  
  struct {
    const struct iovec*v;
    int n;              /**< size of above iovec */
    int current_packet; /**< current buffer for sending */
    int msg_size;       /**< total size of message to send */
    int done;           /**< total amount of data sent */
    int v_done;         /**< size of current iovec segment already sent */
    int v_current;      /**< current iovec segment beeing sent */
  } send;
  
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct ibv_mr*mr;           /**< global MR (used for 'buffer') */
  struct nm_ibverbs_cnx*cnx;
};

static void nm_ibverbs_bycopy_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv);
static void nm_ibverbs_bycopy_addr_pack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_bycopy_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr);
static void nm_ibverbs_bycopy_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_bycopy_send_poll(void*_status);
static void nm_ibverbs_bycopy_recv_init(void*_status,  struct iovec*v, int n);
static int  nm_ibverbs_bycopy_poll_one(void*_status);
static int  nm_ibverbs_bycopy_poll_any(struct nm_pkt_wrap*p_pw, struct nm_gate**pp_gate);
static int  nm_ibverbs_bycopy_cancel_recv(void*_status);

static const struct nm_ibverbs_method_iface_s nm_ibverbs_bycopy_method =
  {
    .cnx_create  = &nm_ibverbs_bycopy_cnx_create,
    .addr_pack   = &nm_ibverbs_bycopy_addr_pack,
    .addr_unpack = &nm_ibverbs_bycopy_addr_unpack,
    .send_post   = &nm_ibverbs_bycopy_send_post,
    .send_poll   = &nm_ibverbs_bycopy_send_poll,
    .recv_init   = &nm_ibverbs_bycopy_recv_init,
    .poll_one    = &nm_ibverbs_bycopy_poll_one,
    .poll_any    = &nm_ibverbs_bycopy_poll_any,
    .cancel_recv = &nm_ibverbs_bycopy_cancel_recv
  };

static void*nm_ibverbs_bycopy_instanciate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_bycopy_destroy(void*);

static const struct puk_adapter_driver_s nm_ibverbs_bycopy_adapter =
  {
    .instanciate = &nm_ibverbs_bycopy_instanciate,
    .destroy = &nm_ibverbs_bycopy_destroy
  };

static int nm_ibverbs_bycopy_load(void)
{
  puk_component_declare("NewMad_ibverbs_bycopy",
			puk_component_provides("PadicoAdapter", "adapter", &nm_ibverbs_bycopy_adapter),
			puk_component_provides("NewMad_ibverbs_method", "method", &nm_ibverbs_bycopy_method));
  return 0;
}
PADICO_MODULE_BUILTIN(NewMad_ibverbs_bycopy, &nm_ibverbs_bycopy_load, NULL, NULL);

static void* nm_ibverbs_bycopy_instanciate(puk_instance_t instance, puk_context_t context)
{
  /* check parameters consistency */
  assert(sizeof(struct nm_ibverbs_bycopy_packet) % 1024 == 0);
  assert(NM_IBVERBS_BYCOPY_CREDITS_THR > NM_IBVERBS_BYCOPY_RBUF_NUM / 2);
  /* init */
  struct nm_ibverbs_bycopy*bycopy = TBX_MALLOC(sizeof(struct nm_ibverbs_bycopy));
  memset(&bycopy->buffer, 0, sizeof(bycopy->buffer));
  bycopy->window.next_out = 1;
  bycopy->window.next_in  = 1;
  bycopy->window.credits  = NM_IBVERBS_BYCOPY_RBUF_NUM;
  bycopy->window.to_ack   = 0;
  bycopy->mr              = NULL;
  return bycopy;
}

static void nm_ibverbs_bycopy_destroy(void*_status)
{
  /* TODO */
}


static void nm_ibverbs_bycopy_cnx_create(void*_status, struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  /* register Memory Region */
  bycopy->mr = ibv_reg_mr(p_ibverbs_drv->pd, &bycopy->buffer,
			  sizeof(bycopy->buffer),
			  IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(bycopy->mr == NULL)
    {
      TBX_FAILURE("Infiniband: bycopy cannot register MR.\n");
    }
  bycopy->cnx = p_ibverbs_cnx;
}

static void nm_ibverbs_bycopy_addr_pack(void*_status, struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  struct nm_ibverbs_segment*seg = &addr->segments[addr->n];
  seg->kind  = NM_IBVERBS_CNX_BYCOPY;
  seg->raddr = (uintptr_t)&bycopy->buffer;
  seg->rkey  = bycopy->mr->rkey;
  addr->n++;
}

static void nm_ibverbs_bycopy_addr_unpack(void*_status, struct nm_ibverbs_cnx_addr*addr)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  int i;
  for(i = 0; addr->segments[i].raddr; i++)
    {
      struct nm_ibverbs_segment*seg = &addr->segments[i];
      if(seg->kind == NM_IBVERBS_CNX_BYCOPY)
	{
	  bycopy->seg = *seg;
	  break;
	}
    }
}


/* ** bycopy I/O ******************************************* */

static void nm_ibverbs_bycopy_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  bycopy->send.v              = v;
  bycopy->send.n              = n;
  bycopy->send.current_packet = 0;
  bycopy->send.msg_size       = 0;
  bycopy->send.done           = 0;
  bycopy->send.v_done         = 0;
  bycopy->send.v_current      = 0;
  int i;
  for(i = 0; i < bycopy->send.n; i++)
    {
      bycopy->send.msg_size += bycopy->send.v[i].iov_len;
    }
}

static int nm_ibverbs_bycopy_send_poll(void*_status)
{
  struct nm_ibverbs_bycopy*__restrict__ bycopy = _status;
  while(bycopy->send.done < bycopy->send.msg_size)
    { 
      /* 1- check credits */
      const int rack = bycopy->buffer.rack;
      if(rack) {
	bycopy->window.credits += rack;
	bycopy->buffer.rack = 0;
      }
      if(bycopy->window.credits <= 1)
	{
	  int j;
	  for(j = 0; j < NM_IBVERBS_BYCOPY_RBUF_NUM; j++)
	    {
	      if(bycopy->buffer.rbuf[j].header.status)
		{
		  bycopy->window.credits += bycopy->buffer.rbuf[j].header.ack;
		  bycopy->buffer.rbuf[j].header.ack = 0;
		  bycopy->buffer.rbuf[j].header.status &= ~NM_IBVERBS_BYCOPY_STATUS_CREDITS;
		}
	    }
	}
      if(bycopy->window.credits <= 1) 
	{
	  goto wouldblock;
	}
      
      /* 2- check window availability */
      nm_ibverbs_rdma_poll(bycopy->cnx);
      if(bycopy->cnx->pending.wrids[NM_IBVERBS_WRID_RDMA] >= NM_IBVERBS_BYCOPY_SBUF_NUM)
	{
	  goto wouldblock;
	}
      
      /* 3- prepare and send packet */
      struct nm_ibverbs_bycopy_packet*__restrict__ packet = &bycopy->buffer.sbuf[bycopy->send.current_packet];
      const int remaining = bycopy->send.msg_size - bycopy->send.done;
      const int offset = (remaining > NM_IBVERBS_BYCOPY_BUFSIZE) ? 0 : (NM_IBVERBS_BYCOPY_BUFSIZE - remaining);
      int available   = NM_IBVERBS_BYCOPY_BUFSIZE - offset;
      int packet_size = 0;
      while((packet_size < available) &&
	    (bycopy->send.v_current < bycopy->send.n))
	{
	  const int v_remaining = bycopy->send.v[bycopy->send.v_current].iov_len - bycopy->send.v_done;
	  const int fragment_size = (v_remaining > available) ? available : v_remaining;
	  memcpy(&packet->data[offset + packet_size],
		 bycopy->send.v[bycopy->send.v_current].iov_base + bycopy->send.v_done, fragment_size);
	  packet_size += fragment_size;
	  available   -= fragment_size;
	  bycopy->send.v_done += fragment_size;
	  if(bycopy->send.v_done >= bycopy->send.v[bycopy->send.v_current].iov_len) 
	    {
	      bycopy->send.v_current++;
	      bycopy->send.v_done = 0;
	    }
	}
      assert(NM_IBVERBS_BYCOPY_BUFSIZE - offset == packet_size);
      
      packet->header.offset = offset;
      packet->header.ack    = bycopy->window.to_ack;
      packet->header.status = NM_IBVERBS_BYCOPY_STATUS_DATA;
      bycopy->window.to_ack = 0;
      bycopy->send.done += packet_size;
      if(bycopy->send.done >= bycopy->send.msg_size)
	packet->header.status |= NM_IBVERBS_BYCOPY_STATUS_LAST;
      
      nm_ibverbs_rdma_send(bycopy->cnx,
			   sizeof(struct nm_ibverbs_bycopy_packet) - offset,
			   &packet->data[offset],
			   &bycopy->buffer.rbuf[bycopy->window.next_out].data[offset],
			   &bycopy->buffer,
			   &bycopy->seg,
			   bycopy->mr,
			   NM_IBVERBS_WRID_RDMA);
      bycopy->send.current_packet = (bycopy->send.current_packet + 1) % NM_IBVERBS_BYCOPY_SBUF_NUM;
      bycopy->window.next_out     = (bycopy->window.next_out     + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      bycopy->window.credits--;
    }
  nm_ibverbs_rdma_poll(bycopy->cnx);
  if(bycopy->cnx->pending.wrids[NM_IBVERBS_WRID_RDMA]) 
    {
      goto wouldblock;
    }
  return NM_ESUCCESS;
 wouldblock:
  return -NM_EAGAIN;
}

static void nm_ibverbs_bycopy_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_bycopy*bycopy = _status;
  assert(bycopy->recv.buf_posted == NULL);
  bycopy->recv.done        = 0;
  bycopy->recv.msg_size    = -1;
  bycopy->recv.buf_posted  = v->iov_base;
  bycopy->recv.size_posted = v->iov_len;
}

static int nm_ibverbs_bycopy_poll_one(void*_status)
{
  int err = -NM_EUNKNOWN;
  int complete = 0;
  struct nm_ibverbs_bycopy*__restrict__ bycopy = _status;
  struct nm_ibverbs_bycopy_packet*__restrict__ packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
  while( ( (bycopy->recv.msg_size == -1) || (!complete) ) &&
	 (packet->header.status != 0) ) 
    {
      if(bycopy->recv.msg_size == -1) 
	{
	  assert((packet->header.status & NM_IBVERBS_BYCOPY_STATUS_DATA) != 0);
	  bycopy->recv.msg_size = 0;
	}
      complete = (packet->header.status & NM_IBVERBS_BYCOPY_STATUS_LAST);
      const int offset = packet->header.offset;
      const int packet_size = NM_IBVERBS_BYCOPY_BUFSIZE - offset;
      memcpy(bycopy->recv.buf_posted + bycopy->recv.done, &packet->data[offset], packet_size);
      bycopy->recv.msg_size += packet_size;
      bycopy->recv.done += packet_size;
      bycopy->window.credits += packet->header.ack;
      packet->header.ack = 0;
      packet->header.status = 0;
      bycopy->window.to_ack++;
      if(bycopy->window.to_ack > NM_IBVERBS_BYCOPY_CREDITS_THR) 
	{
	  bycopy->buffer.sack = bycopy->window.to_ack;
	  nm_ibverbs_rdma_send(bycopy->cnx,
			       sizeof(uint16_t),
			       (void*)&bycopy->buffer.sack,
			       (void*)&bycopy->buffer.rack,
			       &bycopy->buffer,
			       &bycopy->seg,
			       bycopy->mr,
			       NM_IBVERBS_WRID_ACK);
	  bycopy->window.to_ack = 0;
	}
      nm_ibverbs_rdma_poll(bycopy->cnx);
      bycopy->window.next_in = (bycopy->window.next_in + 1) % NM_IBVERBS_BYCOPY_RBUF_NUM;
      packet = &bycopy->buffer.rbuf[bycopy->window.next_in];
    }
  nm_ibverbs_rdma_poll(bycopy->cnx);
  if((bycopy->recv.msg_size > 0) && complete)
    {
      bycopy->recv.buf_posted = NULL;
      nm_ibverbs_send_flush(bycopy->cnx, NM_IBVERBS_WRID_ACK);
      err = NM_ESUCCESS;
    }
  else 
    {
      err = -NM_EAGAIN;
    }
  return err;
}

static int nm_ibverbs_bycopy_poll_any(struct nm_pkt_wrap*p_pw, struct nm_gate**pp_gate)
{
#warning TODO- poll_any
#if 0
  const struct nm_core*p_core = p_pw->p_drv->p_core;
  struct nm_gate*p_gate = NULL;
  NM_FOR_EACH_GATE(p_gate, p_core)
    {
      if(p_gate->status == NM_GATE_STATUS_CONNECTED)
	{
	  struct nm_ibverbs_cnx*__restrict__ p_ibverbs_cnx = nm_ibverbs_get_cnx(_status, p_pw->trk_id);
	  if(p_ibverbs_cnx->bycopy.buffer.rbuf[p_ibverbs_cnx->bycopy.window.next_in].header.status)
	    {
	      p_pw->p_gate = (struct nm_gate*)p_gate;
	      *pp_gate = p_gate;
	      return NM_ESUCCESS;
	    }
	}
    }
#endif
  return -NM_EAGAIN;
}

static int nm_ibverbs_bycopy_cancel_recv(void*_status)
{
  int err = -NM_EAGAIN;
  struct nm_ibverbs_bycopy*__restrict__ bycopy = _status;
  if(bycopy->recv.done == 0)
    {
      bycopy->recv.buf_posted = NULL;
      bycopy->recv.size_posted = 0;
      err = NM_ESUCCESS;
    }
  return err;
}
