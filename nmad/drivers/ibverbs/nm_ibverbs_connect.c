/*
 * NewMadeleine
 * Copyright (C) 2011(see AUTHORS file)
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

#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>

#include "nm_ibverbs.h"

#include <Padico/Module.h>

PADICO_MODULE_HOOK(NewMad_ibverbs);

/* ********************************************************* */

/* large enough to contain our IB url (IP address + port number) */
#define NM_IBVERBS_URL_SIZE 12

/** a full address entry, with host id and associated address.
 */
struct nm_ibverbs_addr_entry_s
{
  nm_trk_id_t trk_id;
  char url[NM_IBVERBS_URL_SIZE];
  struct nm_ibverbs_cnx_addr addr;
  int ack;
};

static int nm_ibverbs_addr_entry_eq(const void*_e1, const void*_e2)
{
  const struct nm_ibverbs_addr_entry_s*e1 = _e1,*e2 = _e2;
  return ((e1->trk_id == e2->trk_id) && (memcmp(e1->url, e2->url, NM_IBVERBS_URL_SIZE) == 0));
}
static uint32_t nm_ibverbs_addr_entry_hash(const void*_e)
{
  const struct nm_ibverbs_addr_entry_s*e = _e;
  return e->trk_id ^ puk_hash_oneatatime((const void*)e->url, NM_IBVERBS_URL_SIZE);
}

PUK_VECT_TYPE(nm_ibverbs_addr_entry, struct nm_ibverbs_addr_entry_s);

/** IB connection manager, one for each nm_ibverbs_drv.
 */
struct nm_ibverbs_connect_s
{
  int sock; /**< UDP socket used to exchange addresses */
  puk_hashtable_t addrs; /**< already received addresses, hashed by (trk_id, url) */
};

/* ********************************************************* */
/* ** state transitions for QP finite-state automaton */

/** create QP and both CQs */
void nm_ibverbs_cnx_qp_create(struct nm_ibverbs_cnx*p_ibverbs_cnx, struct nm_ibverbs_drv*p_ibverbs_drv)
{
  /* init inbound CQ */
  p_ibverbs_cnx->if_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_RX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->if_cq == NULL) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot create in CQ\n");
      abort();
    }
  /* init outbound CQ */
  p_ibverbs_cnx->of_cq = ibv_create_cq(p_ibverbs_drv->context, NM_IBVERBS_TX_DEPTH, NULL, NULL, 0);
  if(p_ibverbs_cnx->of_cq == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot create out CQ\n");
      abort();
    }
  /* create QP */
  struct ibv_qp_init_attr qp_init_attr = {
    .send_cq = p_ibverbs_cnx->of_cq,
    .recv_cq = p_ibverbs_cnx->if_cq,
    .cap     = {
      .max_send_wr     = NM_IBVERBS_TX_DEPTH,
      .max_recv_wr     = NM_IBVERBS_RX_DEPTH,
      .max_send_sge    = NM_IBVERBS_MAX_SG_SQ,
      .max_recv_sge    = NM_IBVERBS_MAX_SG_RQ,
      .max_inline_data = NM_IBVERBS_MAX_INLINE
    },
    .qp_type = IBV_QPT_RC
  };
  p_ibverbs_cnx->qp = ibv_create_qp(p_ibverbs_drv->pd, &qp_init_attr);
  if(p_ibverbs_cnx->qp == NULL)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: couldn't create QP\n");
      abort();
    }
  p_ibverbs_cnx->max_inline = qp_init_attr.cap.max_inline_data;
}

/** modify QP to state RESET */
void nm_ibverbs_cnx_qp_reset(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  /* modify QP- step: RTS */
  struct ibv_qp_attr attr =
    {
      .qp_state = IBV_QPS_RESET
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr, IBV_QP_STATE);
  if(rc != 0)
    {
      fprintf(stderr,"nmad: FATAL- ibverbs: failed to modify QP to RESET.\n");
      abort();
    }
}

/** modify QP to state INIT */
void nm_ibverbs_cnx_qp_init(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state        = IBV_QPS_INIT,
      .pkey_index      = 0,
      .port_num        = NM_IBVERBS_PORT,
      .qp_access_flags = IBV_ACCESS_REMOTE_WRITE
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_PKEY_INDEX         |
			 IBV_QP_PORT               |
			 IBV_QP_ACCESS_FLAGS);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: failed to modify QP to INIT.\n");
      abort();
    }
}

/** modify QP to state RTR */
void nm_ibverbs_cnx_qp_rtr(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state           = IBV_QPS_RTR,
      .path_mtu           = NM_IBVERBS_MTU,
      .dest_qp_num        = p_ibverbs_cnx->remote_addr.qpn,
      .rq_psn             = p_ibverbs_cnx->remote_addr.psn,
      .max_dest_rd_atomic = NM_IBVERBS_RDMA_DEPTH,
      .min_rnr_timer      = 12, /* 12 */
      .ah_attr            = {
	.is_global        = 0,
	.dlid             = p_ibverbs_cnx->remote_addr.lid,
	.sl               = 0,
	.src_path_bits    = 0,
	.port_num         = NM_IBVERBS_PORT
      }
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_AV                 |
			 IBV_QP_PATH_MTU           |
			 IBV_QP_DEST_QPN           |
			 IBV_QP_RQ_PSN             |
			 IBV_QP_MAX_DEST_RD_ATOMIC |
			 IBV_QP_MIN_RNR_TIMER);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: failed to modify QP to RTR\n");
      abort();
    }
}

/** modify QP to state RTS */
void nm_ibverbs_cnx_qp_rts(struct nm_ibverbs_cnx*p_ibverbs_cnx)
{
  struct ibv_qp_attr attr =
    {
      .qp_state      = IBV_QPS_RTS,
      .timeout       = 2, /* 14 */
      .retry_cnt     = 7,  /* 7 */
      .rnr_retry     = 7,  /* 7 = unlimited */
      .sq_psn        = p_ibverbs_cnx->local_addr.psn,
      .max_rd_atomic = NM_IBVERBS_RDMA_DEPTH /* 1 */
    };
  int rc = ibv_modify_qp(p_ibverbs_cnx->qp, &attr,
			 IBV_QP_STATE              |
			 IBV_QP_TIMEOUT            |
			 IBV_QP_RETRY_CNT          |
			 IBV_QP_RNR_RETRY          |
			 IBV_QP_SQ_PSN             |
			 IBV_QP_MAX_QP_RD_ATOMIC);
  if(rc != 0)
    {
      fprintf(stderr,"nmad: FATAL-  ibverbs: failed to modify QP to RTS\n");
      abort();
    }
}

/* ********************************************************* */
/* ** connector */

void nm_ibverbs_connect_create(struct nm_ibverbs_drv*p_ibverbs_drv)
{
  assert(p_ibverbs_drv->connector == NULL);
  /* allocate connector */
  struct nm_ibverbs_connect_s*c = TBX_MALLOC(sizeof(struct nm_ibverbs_connect_s));
  c->sock = -1;
  c->addrs = puk_hashtable_new(&nm_ibverbs_addr_entry_hash, &nm_ibverbs_addr_entry_eq);
  p_ibverbs_drv->connector = c;

  /* open socket */
  int fd = NM_SYS(socket)(AF_INET, SOCK_DGRAM, 0);
  assert(fd > -1);
  struct sockaddr_in addr;
  unsigned addr_len = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(0);
  addr.sin_addr.s_addr = INADDR_ANY;
  int rc = NM_SYS(bind)(fd, (struct sockaddr*)&addr, addr_len);
  if(rc) 
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: socket bind error (%s)\n", strerror(errno));
      abort();
    }
  rc = NM_SYS(getsockname)(fd, (struct sockaddr*)&addr, &addr_len);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot get socket name (%s)\n", strerror(errno));
      abort();
    }
  int rcvbuf = 64 * 1024;
  rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
  p_ibverbs_drv->connector->sock = fd;

  /* encode url */
  struct ifaddrs*ifa_list = NULL;
  rc = getifaddrs(&ifa_list);
  if(rc != 0)
    {
      fprintf(stderr, "nmad: FATAL- ibverbs: cannot get local address\n");
      abort();
    }
  struct ifaddrs*i;
  for(i = ifa_list; i != NULL; i = i->ifa_next)
    {
      if (i->ifa_addr && i->ifa_addr->sa_family == AF_INET)
	{
	  struct sockaddr_in*inaddr = (struct sockaddr_in*)i->ifa_addr;
	  if(!(i->ifa_flags & IFF_LOOPBACK))
	    {
	      char s_url[16];
	      snprintf(s_url, 16, "%08x%04x", htonl(inaddr->sin_addr.s_addr), addr.sin_port);
	      p_ibverbs_drv->url = tbx_strdup(s_url);
	      break;
	    }
	}
    }
}


void nm_ibverbs_connect_send(struct nm_ibverbs_drv*p_ibverbs_drv, const char*remote_url, nm_trk_id_t trk_id,
			     const struct nm_ibverbs_cnx_addr*local_addr, int ack)
{
  /* parse peer address */
  in_addr_t peer_addr;
  int peer_port;
  sscanf(remote_url, "%08x%04x", &peer_addr, &peer_port);
  struct sockaddr_in inaddr =
    {
      .sin_family = AF_INET,
      .sin_port   = peer_port,
      .sin_addr   = (struct in_addr){ .s_addr = ntohl(peer_addr) }
    };
  /* send address */
  struct nm_ibverbs_addr_entry_s local_entry;
  local_entry.trk_id = trk_id;
  assert(strlen(p_ibverbs_drv->url) == NM_IBVERBS_URL_SIZE);
  memcpy(local_entry.url, p_ibverbs_drv->url, NM_IBVERBS_URL_SIZE);
  memcpy(&local_entry.addr, local_addr, sizeof(struct nm_ibverbs_cnx_addr));
  local_entry.ack = ack;
  int rc = -1;
 retry_send:
  rc = NM_SYS(sendto)(p_ibverbs_drv->connector->sock, 
		      &local_entry, sizeof(struct nm_ibverbs_addr_entry_s), 0, &inaddr, sizeof(inaddr));
  if(rc == -1)
    {
      if(errno == EINTR)
	goto retry_send;
      fprintf(stderr, "nmad: FATAL- ibverbs: error while sending address to %s (%s)\n", remote_url, strerror(errno));
      abort();
    }
}

/* poll network for peer address; return 0 for success; -1 for timeout */
static int nm_ibverbs_connect_poll(struct nm_ibverbs_drv*p_ibverbs_drv)
{
  struct pollfd fds = { .fd = p_ibverbs_drv->connector->sock, .events = POLLIN };
  int rc = -1;
 retry_poll:
  rc = NM_SYS(poll)(&fds, 1, 600);
  if(rc == -1)
    {
      const int err = errno;
      if(err == EINTR)
	goto retry_poll;
      else
	{
	  fprintf(stderr, "nmad: FATAL- ibverbs: error while receiving address.\n");
	  abort();
	}
    }
  else if(rc == 0)
    {
      return -1;
    }
  struct nm_ibverbs_addr_entry_s*remote_entry = TBX_MALLOC(sizeof(struct nm_ibverbs_addr_entry_s));
 retry_recv:
  rc = NM_SYS(recv)(p_ibverbs_drv->connector->sock, remote_entry, sizeof(struct nm_ibverbs_addr_entry_s), 0);
  if(rc == -1)
    {
      const int err = errno;
      if(err == EINTR)
	goto retry_recv;
      else
	{
	  fprintf(stderr, "nmad: FATAL- ibverbs: error while receiving address\n");
	  abort();
	}
    }
  struct nm_ibverbs_addr_entry_s*prev = puk_hashtable_lookup(p_ibverbs_drv->connector->addrs, remote_entry);
  if(prev)
    {
      puk_hashtable_remove(p_ibverbs_drv->connector->addrs, prev);
      TBX_FREE(prev);
    }
  puk_hashtable_insert(p_ibverbs_drv->connector->addrs, remote_entry, remote_entry);
  return 0;
}

int nm_ibverbs_connect_recv(struct nm_ibverbs_drv*p_ibverbs_drv, const char*remote_url, nm_trk_id_t trk_id,
			    struct nm_ibverbs_cnx_addr*remote_addr)
{
  struct nm_ibverbs_addr_entry_s key;
  key.trk_id = trk_id;
  memcpy(key.url, remote_url, NM_IBVERBS_URL_SIZE);
  for(;;)
    {
      /* lookup in already received address */
      struct nm_ibverbs_addr_entry_s*remote_entry = puk_hashtable_lookup(p_ibverbs_drv->connector->addrs, &key);
      if(remote_entry != NULL)
	{
	  memcpy(remote_addr, &remote_entry->addr, sizeof(struct nm_ibverbs_cnx_addr));
	  return 0;
	}
      int rc = nm_ibverbs_connect_poll(p_ibverbs_drv);
      if(rc != 0)
	{
	  fprintf(stderr, "nmad: WARNING- ibverbs: timeout while receiving peer address.\n");
	  /* Timeout- we didn't receive peer address in time. We don't know
	   * whether our packet was lost too, and we cannot try to establish
	   * the connection to check. In doubt, return error. Caller will send again.
	   */
	  return -1;
	}
    }
  return 0;
}

int nm_ibverbs_connect_wait_ack(struct nm_ibverbs_drv*p_ibverbs_drv, const char*remote_url, nm_trk_id_t trk_id)
{
  struct nm_ibverbs_addr_entry_s key;
  key.trk_id = trk_id;
  memcpy(key.url, remote_url, NM_IBVERBS_URL_SIZE);
  for(;;)
    {
      struct nm_ibverbs_addr_entry_s*remote_entry = puk_hashtable_lookup(p_ibverbs_drv->connector->addrs, &key);
      if(remote_entry != NULL && remote_entry->ack != 0)
	{
	  return 0;
	}
      int rc = nm_ibverbs_connect_poll(p_ibverbs_drv);
      if(rc != 0)
	{
	  fprintf(stderr, "nmad: WARNING- ibverbs: timeout while waiting for ACK.\n");
	  return -1;
	}
    }
  return 0;
}
