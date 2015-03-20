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
#include <nm_connector.h>

#include <Padico/Module.h>

/* *** method: 'adaptrdma' ********************************* */

static const int nm_ibverbs_adaptrdma_block_granularity = 4096;
static const int nm_ibverbs_adaptrdma_step_base         = 16  * 1024;
static const int nm_ibverbs_adaptrdma_step_overrun      = 16  * 1024;

#define NM_IBVERBS_ADAPTRDMA_BUFSIZE (16 * 1024 * 1024)
#define NM_IBVERBS_ADAPTRDMA_HDRSIZE (sizeof(struct nm_ibverbs_adaptrdma_header))

#define NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR      0x01
#define NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE    0x02

/** on the wire header of minidriver 'adaptrdma' */
struct nm_ibverbs_adaptrdma_header 
{
  uint32_t checksum;
  volatile uint32_t offset;
  volatile uint32_t busy; /* 'busy' has to be the last field in the struct */
} __attribute__((packed));

/** Connection state for tracks sending with adaptive RDMA super-pipeline */
struct nm_ibverbs_adaptrdma 
{
  struct ibv_mr*mr;
  struct nm_ibverbs_segment seg; /**< remote segment */
  struct nm_ibverbs_cnx*cnx;
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

static void nm_ibverbs_adaptrdma_getprops(int index, struct nm_minidriver_properties_s*props);
static void nm_ibverbs_adaptrdma_init(puk_context_t context, const void**drv_url, size_t*url_size);
static void nm_ibverbs_adaptrdma_connect(void*_status, const void*remote_url, size_t url_size);
static void nm_ibverbs_adaptrdma_send_post(void*_status, const struct iovec*v, int n);
static int  nm_ibverbs_adaptrdma_send_poll(void*_status);
static void nm_ibverbs_adaptrdma_recv_init(void*_status, struct iovec*v, int n);
static int  nm_ibverbs_adaptrdma_poll_one(void*_status);

static const struct nm_minidriver_iface_s nm_ibverbs_adaptrdma_minidriver =
  {
    .getprops    = &nm_ibverbs_adaptrdma_getprops,
    .init        = &nm_ibverbs_adaptrdma_init,
    .connect     = &nm_ibverbs_adaptrdma_connect,
    .send_post   = &nm_ibverbs_adaptrdma_send_post,
    .send_poll   = &nm_ibverbs_adaptrdma_send_poll,
    .recv_init   = &nm_ibverbs_adaptrdma_recv_init,
    .poll_one    = &nm_ibverbs_adaptrdma_poll_one,
    .cancel_recv = NULL
  };

static void*nm_ibverbs_adaptrdma_instanciate(puk_instance_t instance, puk_context_t context);
static void nm_ibverbs_adaptrdma_destroy(void*);

static const struct puk_component_driver_s nm_ibverbs_adaptrdma_component =
  {
    .instanciate = &nm_ibverbs_adaptrdma_instanciate,
    .destroy = &nm_ibverbs_adaptrdma_destroy
  };


/* ********************************************************* */

PADICO_MODULE_COMPONENT(NewMad_ibverbs_adaptrdma,
  puk_component_declare("NewMad_ibverbs_adaptrdma",
			puk_component_provides("PadicoComponent", "component", &nm_ibverbs_adaptrdma_component),
			puk_component_provides("NewMad_minidriver", "minidriver", &nm_ibverbs_adaptrdma_minidriver)));


/* ********************************************************* */

static void* nm_ibverbs_adaptrdma_instanciate(puk_instance_t instance, puk_context_t context)
{
  struct nm_ibverbs_adaptrdma*adaptrdma = TBX_MALLOC(sizeof(struct nm_ibverbs_adaptrdma));
  memset(&adaptrdma->buffer, 0, sizeof(adaptrdma->buffer));
  adaptrdma->mr = NULL;
  adaptrdma->context = context;
  return adaptrdma;
}

static void nm_ibverbs_adaptrdma_destroy(void*_status)
{
  /* TODO */
}

/* ********************************************************* */

static void nm_ibverbs_adaptrdma_getprops(int index, struct nm_minidriver_properties_s*props)
{
  nm_ibverbs_hca_get_profile(index, &props->profile);
}

static void nm_ibverbs_adaptrdma_init(puk_context_t context, const void**drv_url, size_t*url_size)
{
  const char*url = NULL;
  nm_connector_create(sizeof(struct nm_ibverbs_cnx_addr), &url);
  puk_context_putattr(context, "local_url", url);
  *drv_url = url;
  *url_size = strlen(url);
}

static void nm_ibverbs_adaptrdma_connect(void*_status, const void*remote_url, size_t url_size)
{
  struct nm_ibverbs_adaptrdma*adaptrdma = _status;
  const char*s_index = puk_context_getattr(adaptrdma->context, "index");
  const int index= atoi(s_index);
  struct nm_ibverbs_hca_s*p_hca = nm_ibverbs_hca_resolve(index);
  struct nm_ibverbs_cnx*p_ibverbs_cnx = nm_ibverbs_cnx_new(p_hca);
  adaptrdma->cnx = p_ibverbs_cnx;
  /* register Memory Region */
  adaptrdma->mr = ibv_reg_mr(p_hca->pd, &adaptrdma->buffer, sizeof(adaptrdma->buffer),
			     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  if(adaptrdma->mr == NULL)
    {
      TBX_FAILURE("Infiniband: adaptrdma cannot register MR.\n");
    }
  struct nm_ibverbs_segment*seg = &p_ibverbs_cnx->local_addr.segment;
  seg->raddr = (uintptr_t)&adaptrdma->buffer;
  seg->rkey  = adaptrdma->mr->rkey;
  /* ** exchange addresses */
  const char*local_url = puk_context_getattr(adaptrdma->context, "local_url");
  int rc = nm_connector_exchange(local_url, remote_url,
				 &p_ibverbs_cnx->local_addr, &p_ibverbs_cnx->remote_addr);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: timeout in address exchange.\n");
    }
  adaptrdma->seg =  p_ibverbs_cnx->remote_addr.segment;
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
static inline struct nm_ibverbs_adaptrdma_header*nm_ibverbs_adaptrdma_get_header(void*buf, int packet_size)
{
  return buf + packet_size - sizeof(struct nm_ibverbs_adaptrdma_header);
}


static void nm_ibverbs_adaptrdma_send_post(void*_status, const struct iovec*v, int n)
{
  struct nm_ibverbs_adaptrdma*adaptrdma = _status;
  assert(n == 1);
  adaptrdma->send.message = v[0].iov_base;
  adaptrdma->send.todo    = v[0].iov_len;
  adaptrdma->send.done    = 0;
  adaptrdma->send.rbuf    = adaptrdma->buffer.rbuf;
  adaptrdma->send.sbuf    = adaptrdma->buffer.sbuf;
  adaptrdma->send.size_guard = nm_ibverbs_adaptrdma_step_base;
}

static int nm_ibverbs_adaptrdma_send_poll(void*_status)
{
  struct nm_ibverbs_adaptrdma*adaptrdma = _status;
  int packet_size = 0;
  const int block_size = nm_ibverbs_adaptrdma_block_size(adaptrdma->send.done);
  const int available  = block_size - NM_IBVERBS_ADAPTRDMA_HDRSIZE;
  while((adaptrdma->send.todo > 0) && 
	((adaptrdma->cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0) ||
	 (packet_size < adaptrdma->send.size_guard) ||
	 (adaptrdma->send.todo <= nm_ibverbs_adaptrdma_step_overrun)))
    {
      const int frag_size = (adaptrdma->send.todo > available) ? available : adaptrdma->send.todo;
      struct nm_ibverbs_adaptrdma_header*const h = 
	nm_ibverbs_adaptrdma_get_header(adaptrdma->send.sbuf, packet_size + block_size);
      memcpy(adaptrdma->send.sbuf + packet_size, 
	     &adaptrdma->send.message[adaptrdma->send.done], frag_size);
      h->checksum = nm_ibverbs_checksum(&adaptrdma->send.message[adaptrdma->send.done], frag_size);
      h->busy   = NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR;
      h->offset = frag_size;
      adaptrdma->send.todo -= frag_size;
      adaptrdma->send.done += frag_size;
      packet_size += block_size;
      if((adaptrdma->cnx->pending.wrids[NM_IBVERBS_WRID_PACKET] > 0) &&
	 (adaptrdma->send.done > nm_ibverbs_adaptrdma_step_base))
	{
	  nm_ibverbs_rdma_poll(adaptrdma->cnx);
	}
    }
  struct nm_ibverbs_adaptrdma_header*const h_last =
    nm_ibverbs_adaptrdma_get_header(adaptrdma->send.sbuf, packet_size);
  h_last->busy = NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE;
  nm_ibverbs_rdma_send(adaptrdma->cnx, packet_size, adaptrdma->send.sbuf, adaptrdma->send.rbuf, 
		       &adaptrdma->buffer,
		       &adaptrdma->seg,
		       adaptrdma->mr,
		       NM_IBVERBS_WRID_PACKET);
  adaptrdma->send.size_guard *= (adaptrdma->send.size_guard <= 8192) ? 2 : 1.5;
  nm_ibverbs_rdma_poll(adaptrdma->cnx);
  adaptrdma->send.sbuf += packet_size;
  adaptrdma->send.rbuf += packet_size;
  if(adaptrdma->send.todo <= 0)
    {
      nm_ibverbs_send_flush(adaptrdma->cnx, NM_IBVERBS_WRID_PACKET);
      return NM_ESUCCESS;
    }
  else
    return -NM_EAGAIN;

}

static void nm_ibverbs_adaptrdma_recv_init(void*_status, struct iovec*v, int n)
{
  struct nm_ibverbs_adaptrdma*adaptrdma = _status;
  adaptrdma->recv.done       = 0;
  adaptrdma->recv.block_size = nm_ibverbs_adaptrdma_block_size(0);
  adaptrdma->recv.message    = v->iov_base;
  adaptrdma->recv.size       = v->iov_len;
  adaptrdma->recv.rbuf       = adaptrdma->buffer.rbuf;
}


static int nm_ibverbs_adaptrdma_poll_one(void*_status)
{
  struct nm_ibverbs_adaptrdma*adaptrdma = _status;
  do
    {
      struct nm_ibverbs_adaptrdma_header*h = nm_ibverbs_adaptrdma_get_header(adaptrdma->recv.rbuf,
									     adaptrdma->recv.block_size);
      if(!h->busy)
	goto wouldblock;
      const int frag_size = h->offset;
      const int flag = h->busy;
      memcpy(&adaptrdma->recv.message[adaptrdma->recv.done], adaptrdma->recv.rbuf, frag_size);
      /* checksum */
      if(nm_ibverbs_checksum(&adaptrdma->recv.message[adaptrdma->recv.done], frag_size) != h->checksum)
	{
	  fprintf(stderr, "nmad: FATAL- ibverbs: checksum failed.\n");
	  abort();
	}
      /* clear blocks */
      void*_rbuf = adaptrdma->recv.rbuf;
      adaptrdma->recv.done += frag_size;
      adaptrdma->recv.rbuf += adaptrdma->recv.block_size;
      while(_rbuf < adaptrdma->recv.rbuf)
	{
	  h = nm_ibverbs_adaptrdma_get_header(_rbuf, nm_ibverbs_adaptrdma_block_granularity);
	  h->offset = 0;
	  h->busy = 0;
	  _rbuf += nm_ibverbs_adaptrdma_block_granularity;
	}
      switch(flag)
	{
	case NM_IBVERBS_ADAPTRDMA_FLAG_REGULAR:
	  break;
	  
	case NM_IBVERBS_ADAPTRDMA_FLAG_BLOCKSIZE:
	  adaptrdma->recv.block_size = nm_ibverbs_adaptrdma_block_size(adaptrdma->recv.done);
	  break;
	  
	default:
	  fprintf(stderr, "nmad: FATAL- ibverbs: unexpected flag 0x%x in adaptrdma_recv()\n", flag);
	  abort();
	  break;
	}
    }
  while(adaptrdma->recv.done < adaptrdma->recv.size);
  
  adaptrdma->recv.message = NULL;
  return NM_ESUCCESS;
  
 wouldblock:
  return -NM_EAGAIN;
}

