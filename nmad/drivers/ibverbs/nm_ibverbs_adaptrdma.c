/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
#include <nm_connector.h>

#include <Padico/Module.h>

/* *** method: 'adaptrdma' ********************************* */

static const int nm_ibverbs_adaptrdma_block_granularity = 4096;
static const int nm_ibverbs_adaptrdma_step_base         = 16  * 1024;
static const int nm_ibverbs_adaptrdma_step_overrun      = 16  * 1024;

#define NM_IBVERBS_ADAPTRDMA_BUFSIZE (16 * 1024 * 1024)
#define NM_IBVERBS_ADAPTRDMA_HDRSIZE (sizeof(struct nm_ibverbs_adaptrdma_header_s))

#define NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR      0x01
#define NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE    0x02

/** on the wire header of minidriver 'adaptrdma' */
struct nm_ibverbs_adaptrdma_header_s
{
  uint32_t checksum;
  volatile uint32_t offset;
  volatile uint32_t busy; /* 'busy' has to be the last field in the struct */
} __attribute__((packed));

/** Connection state for tracks sending with adaptive RDMA super-pipeline */
struct nm_ibverbs_adaptrdma_s
{
  struct ibv_mr*mr;
  struct nm_ibverbs_segment_s seg; /**< remote segment */
  struct nm_ibverbs_cnx_s*p_cnx;
  puk_context_t context;
  struct
  {
    char sbuf[NM_IBVERBS_ADAPTRDMA_BUFSIZE];
    char rbuf[NM_IBVERBS_ADAPTRDMA_BUFSIZE];
  } buffer;

  struct
  {
    const char*message;
    int todo;
    int done;
    void*rbuf;
    void*sbuf;
    int size_guard;
  } send;

  struct
  {
    char*message;
    int size;
    void*rbuf;
    int done;
    int block_size;
  } recv;
};

static void nm_ibverbs_adaptrdma_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props);
static void nm_ibverbs_adaptrdma_init(puk_context_t context, const void**p_url, size_t*p_url_size);
static void nm_ibverbs_adaptrdma_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_adaptrdma_send_iov_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_adaptrdma_send_poll(void*_status);
static void nm_ibverbs_adaptrdma_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_adaptrdma_poll_one(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_adaptrdma_minidriver =
  {
    .getprops       = &nm_ibverbs_adaptrdma_getprops,
    .init           = &nm_ibverbs_adaptrdma_init,
    .connect        = &nm_ibverbs_adaptrdma_connect,
    .send_iov_post      = &nm_ibverbs_adaptrdma_send_iov_post,
    .send_poll      = &nm_ibverbs_adaptrdma_send_poll,
    .recv_iov_post  = &nm_ibverbs_adaptrdma_recv_init,
    .recv_poll_one  = &nm_ibverbs_adaptrdma_poll_one,
    .recv_cancel    = NULL
  };

static void*nm_ibverbs_adaptrdma_instantiate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_adaptrdma_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_adaptrdma_component =
  {
    .instantiate    = &nm_ibverbs_adaptrdma_instantiate,
    .destroy        = &nm_ibverbs_adaptrdma_destroy
  };


/* ********************************************************* */

PADICO_MODULE_COMPONENT(NewMad_ibverbs_adaptrdma,
  puk_component_declare("NewMad_ibverbs_adaptrdma",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_adaptrdma_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_adaptrdma_minidriver),
			puk_component_attr("ibv_device", "auto"),
			puk_component_attr("ibv_port", "auto")));


/* ********************************************************* */

static void* nm_ibverbs_adaptrdma_instantiate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = malloc(sizeof(struct nm_ibverbs_adaptrdma_s));
  memset(&p_adaptrdma->buffer, 0, sizeof(p_adaptrdma->buffer));
  p_adaptrdma->mr = NULL;
  p_adaptrdma->context = context;
  return p_adaptrdma;
}

static void nm_ibverbs_adaptrdma_destroy(void*_status)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = _status;
  free(p_adaptrdma);
}

/* ********************************************************* */

static void nm_ibverbs_adaptrdma_getprops(puk_context_t context, struct nm_minidriver_properties_s*p_props)
{
  struct nm_ibverbs_hca_s*p_hca = nm_ibverbs_hca_from_context(context);
  nm_ibverbs_hca_get_profile(p_hca, &p_props->profile);
}

static void nm_ibverbs_adaptrdma_init(puk_context_t context, const void**p_url, size_t*p_url_size)
{
  const char*url = NULL;
  nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr_s), &url);
  puk_context_putattr(context, "local_url", url);
  *p_url = url;
  *p_url_size = strlen(url);
}

static void nm_ibverbs_adaptrdma_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = _status;
  struct nm_ibverbs_hca_s*p_hca = nm_ibverbs_hca_from_context(p_adaptrdma->context);
  struct nm_ibverbs_cnx_s*p_ibverbs_cnx = nm_ibverbs_cnx_new(p_hca);
  p_adaptrdma->p_cnx = p_ibverbs_cnx;
  /* register Memory Region */
  p_adaptrdma->mr = ibv_reg_mr(p_hca->pd, &p_adaptrdma->buffer, sizeof(p_adaptrdma->buffer),
                               IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(p_adaptrdma->mr == NULL)
    {
      NM_FATAL("Infiniband: adaptrdma cannot register MR.\n");
    }
  struct nm_ibverbs_segment_s*p_seg = &p_ibverbs_cnx->local_addr.segment;
  p_seg->raddr = (uintptr_t)&p_adaptrdma->buffer;
  p_seg->rkey  = p_adaptrdma->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(p_adaptrdma->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_ibverbs_cnx->local_addr, &p_ibverbs_cnx->remote_addr);
  if(rc)
    {
      NM_FATAL("ibverbs: timeout in address exchange.\n");
    }
  p_adaptrdma->seg = p_ibverbs_cnx->remote_addr.segment;
  nm_ibverbs_cnx_connect(p_ibverbs_cnx);
}


/* ** adaptrdma I/O **************************************** */

static inline int nm_ibverbs_adaptrdma_block_size(int done)
{
  if(done < 16 * 1024)
    return 4096;
  else if(done < 128 * 1024)
    return  4096;
  else if(done < 512 * 1024)
    return  8192;
  else if(done < 1024 * 1024)
    return  16384;
  else
    return 16384;
}

/** calculates the position of the packet header, given the beginning of the packet and its size
 */
static inline struct nm_ibverbs_adaptrdma_header_s*nm_ibverbs_adaptrdma_get_header(void*buf, int packet_size)
{
  return buf + packet_size - sizeof(struct nm_ibverbs_adaptrdma_header_s);
}

static void nm_ibverbs_adaptrdma_send_iov_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = _status;
  assert(n == 1);
  p_adaptrdma->send.message = v[0].iov_base;
  p_adaptrdma->send.todo    = v[0].iov_len;
  p_adaptrdma->send.done    = 0;
  p_adaptrdma->send.rbuf    = p_adaptrdma->buffer.rbuf;
  p_adaptrdma->send.sbuf    = p_adaptrdma->buffer.sbuf;
  p_adaptrdma->send.size_guard = nm_ibverbs_adaptrdma_step_base;
}

static int nm_ibverbs_adaptrdma_send_poll(void*_status)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = _status;
  int packet_size = 0;
  const int block_size = nm_ibverbs_adaptrdma_block_size(p_adaptrdma->send.done);
  const int available  = block_size - NM_IBVERBS_ADAPTRDMA_HDRSIZE;
  while((p_adaptrdma->send.todo > 0) && 
	((p_adaptrdma->p_cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0) ||
	 (packet_size < p_adaptrdma->send.size_guard) ||
	 (p_adaptrdma->send.todo <= nm_ibverbs_adaptrdma_step_overrun)))
    {
      const int frag_size = (p_adaptrdma->send.todo > available) ? available : p_adaptrdma->send.todo;
      struct nm_ibverbs_adaptrdma_header_s*const p_header = 
	nm_ibverbs_adaptrdma_get_header(p_adaptrdma->send.sbuf, packet_size + block_size);
      memcpy(p_adaptrdma->send.sbuf + packet_size, 
	     &p_adaptrdma->send.message[p_adaptrdma->send.done], frag_size);
      p_header->checksum = nm_ibverbs_checksum(&p_adaptrdma->send.message[p_adaptrdma->send.done], frag_size);
      p_header->busy   = NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR;
      p_header->offset = frag_size;
      p_adaptrdma->send.todo -= frag_size;
      p_adaptrdma->send.done += frag_size;
      packet_size += block_size;
      if((p_adaptrdma->p_cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0) &&
	 (p_adaptrdma->send.done > nm_ibverbs_adaptrdma_step_base))
	{
	  nm_ibverbs_rdma_poll(p_adaptrdma->p_cnx);
	}
    }
  struct nm_ibverbs_adaptrdma_header_s*const p_h_last =
    nm_ibverbs_adaptrdma_get_header(p_adaptrdma->send.sbuf, packet_size);
  p_h_last->busy = NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE;
  nm_ibverbs_rdma_send(p_adaptrdma->p_cnx, packet_size, p_adaptrdma->send.sbuf, p_adaptrdma->send.rbuf, 
		       &p_adaptrdma->buffer,
		       &p_adaptrdma->seg,
		       p_adaptrdma->mr,
		       NM_IBVERBS_WRID_PACKET);
  p_adaptrdma->send.size_guard *= (p_adaptrdma->send.size_guard <= 8192) ? 2 : 1.5;
  nm_ibverbs_rdma_poll(p_adaptrdma->p_cnx);
  p_adaptrdma->send.sbuf += packet_size;
  p_adaptrdma->send.rbuf += packet_size;
  if(p_adaptrdma->send.todo <= 0)
    {
      nm_ibverbs_send_flush(p_adaptrdma->p_cnx, NM_IBVERBS_WRID_PACKET);
      return NM_ESUCCESS;
    }
  else
    {
      return -NM_EAGAIN;
    }

}

static void nm_ibverbs_adaptrdma_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = _status;
  p_adaptrdma->recv.done       = 0;
  p_adaptrdma->recv.block_size = nm_ibverbs_adaptrdma_block_size(0);
  p_adaptrdma->recv.message    = v->iov_base;
  p_adaptrdma->recv.size       = v->iov_len;
  p_adaptrdma->recv.rbuf       = p_adaptrdma->buffer.rbuf;
}


static int nm_ibverbs_adaptrdma_poll_one(void*_status)
{
  struct nm_ibverbs_adaptrdma_s*p_adaptrdma = _status;
  do
    {
      struct nm_ibverbs_adaptrdma_header_s*p_header =
        nm_ibverbs_adaptrdma_get_header(p_adaptrdma->recv.rbuf, p_adaptrdma->recv.block_size);
      if(!p_header->busy)
	goto wouldblock;
      const int frag_size = p_header->offset;
      const int flag = p_header->busy;
      memcpy(&p_adaptrdma->recv.message[p_adaptrdma->recv.done], p_adaptrdma->recv.rbuf, frag_size);
      /* checksum */
      if(nm_ibverbs_checksum(&p_adaptrdma->recv.message[p_adaptrdma->recv.done], frag_size) != p_header->checksum)
	{
	  NM_FATAL("ibverbs: checksum failed.\n");
	}
      /* clear blocks */
      void*_rbuf = p_adaptrdma->recv.rbuf;
      p_adaptrdma->recv.done += frag_size;
      p_adaptrdma->recv.rbuf += p_adaptrdma->recv.block_size;
      while(_rbuf < p_adaptrdma->recv.rbuf)
	{
	  p_header = nm_ibverbs_adaptrdma_get_header(_rbuf, nm_ibverbs_adaptrdma_block_granularity);
	  p_header->offset = 0;
	  p_header->busy = 0;
	  _rbuf += nm_ibverbs_adaptrdma_block_granularity;
	}
      switch(flag)
	{
	case NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR:
	  break;
	  
	case NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE:
	  p_adaptrdma->recv.block_size = nm_ibverbs_adaptrdma_block_size(p_adaptrdma->recv.done);
	  break;
	  
	default:
	  NM_FATAL("ibverbs: unexpected flag 0x%x in adaptrdma_recv()\n", flag);
	  break;
	}
    }
  while(p_adaptrdma->recv.done < p_adaptrdma->recv.size);
  
  p_adaptrdma->recv.message = NULL;
  return NM_ESUCCESS;
  
 wouldblock:
  return -NM_EAGAIN;
}

