/*
 * NewMadeleine
 * Copyright (C) 2011-2013 (see AUTHORS file)
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

#include "nm_connector.h"

#include <nm_private.h>

#include <Padico/Module.h>

PADICO_MODULE_DECLARE(NewMad_Connector);

/* ********************************************************* */

/* large enough to contain our IB url (IP address + port number) */
#define NM_CONNECTOR_URL_SIZE 12

/** a full address entry, with host id and associated address.
 */
struct nm_connector_entry_s
{
  char url[NM_CONNECTOR_URL_SIZE];
  int ack;
  char _addr; /**< placeholder for variable-length field */
};

static int nm_connector_entry_eq(const void*_e1, const void*_e2)
{
  const struct nm_connector_entry_s*e1 = _e1,*e2 = _e2;
  return (memcmp(e1->url, e2->url, NM_CONNECTOR_URL_SIZE) == 0);
}
static uint32_t nm_connector_entry_hash(const void*_e)
{
  const struct nm_connector_entry_s*e = _e;
  return puk_hash_oneatatime((const void*)e->url, NM_CONNECTOR_URL_SIZE);
}

PUK_VECT_TYPE(nm_connector_entry, struct nm_connector_entry_s);

/** connection manager, exchange per-connection url using per node url
 */
struct nm_connector_s
{
  int sock; /**< UDP socket used to exchange addresses */
  puk_hashtable_t addrs; /**< already received addresses, hashed by node url */
  char url[16]; /**< url for the connector itself */
  int addr_len; /**< length of addresses */
};

static struct
{
  puk_hashtable_t connectors; /** connectors, hashed by local url */
} nm_connector = { .connectors = NULL };


/* ********************************************************* */
/* ** connector */

struct nm_connector_s*nm_connector_create(int addr_len, const char**url)
{
  /* allocate connector */
  struct nm_connector_s*c = TBX_MALLOC(sizeof(struct nm_connector_s));
  c->sock = -1;
  c->addrs = puk_hashtable_new(&nm_connector_entry_hash, &nm_connector_entry_eq);
  c->addr_len = addr_len;

  /* open socket */
  int fd = NM_SYS(socket)(AF_INET, SOCK_DGRAM, 0);
  assert(fd > -1);
  struct sockaddr_in addr;
  unsigned inaddr_len = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(0);
  addr.sin_addr.s_addr = INADDR_ANY;
  int rc = NM_SYS(bind)(fd, (struct sockaddr*)&addr, inaddr_len);
  if(rc) 
    {
      fprintf(stderr, "nmad: FATAL- connector: socket bind error (%s)\n", strerror(errno));
      abort();
    }
  rc = NM_SYS(getsockname)(fd, (struct sockaddr*)&addr, &inaddr_len);
  if(rc)
    {
      fprintf(stderr, "nmad: FATAL- connector: cannot get socket name (%s)\n", strerror(errno));
      abort();
    }
  int rcvbuf = 64 * 1024;
  rc = NM_SYS(setsockopt)(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
  c->sock = fd;

  /* encode url */
  struct in_addr inaddr = puk_inet_getaddr();
  snprintf(c->url, 16, "%08x%04x", htonl(inaddr.s_addr), addr.sin_port);
  *url = c->url;
  if(nm_connector.connectors == NULL)
    nm_connector.connectors = puk_hashtable_new_string();
  puk_hashtable_insert(nm_connector.connectors, c->url, c);
  return c;
}


static void nm_connector_send(struct nm_connector_s*p_connector, const char*remote_url,
			      const void*local_addr, int ack)
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
  struct nm_connector_entry_s*local_entry = TBX_MALLOC(sizeof(struct nm_connector_entry_s) + p_connector->addr_len);
  assert(strlen(p_connector->url) == NM_CONNECTOR_URL_SIZE);
  memcpy(local_entry->url, p_connector->url, NM_CONNECTOR_URL_SIZE);
  memcpy(&local_entry->_addr, local_addr, p_connector->addr_len);
  local_entry->ack = ack;
  int rc = -1;
 retry_send:
  rc = NM_SYS(sendto)(p_connector->sock, 
		      local_entry, sizeof(struct nm_connector_entry_s) + p_connector->addr_len, 0,
		      (struct sockaddr*)&inaddr, sizeof(inaddr));
  if(rc == -1)
    {
      if(errno == EINTR)
	goto retry_send;
      else if(errno == EPERM)
	{
	  /* Linux returns undocumented EPERM if UDP packet is droped because of throttle */
	  usleep(1000);
	  goto retry_send;
	}
      fprintf(stderr, "nmad: FATAL- connector: error while sending address to %s (%s)\n", remote_url, strerror(errno));
      abort();
    }
  free(local_entry);
}

/* poll network for peer address; return 0 for success; -1 for timeout */
static int nm_connector_poll(struct nm_connector_s*p_connector)
{
  struct pollfd fds = { .fd = p_connector->sock, .events = POLLIN };
  int rc = -1;
 retry_poll:
  rc = NM_SYS(poll)(&fds, 1, NM_CONNECTOR_TIMEOUT_ACK);
  if(rc == -1)
    {
      const int err = errno;
      if(err == EINTR)
	goto retry_poll;
      else
	{
	  fprintf(stderr, "nmad: FATAL- connector: timeout while receiving address.\n");
	  abort();
	}
    }
  else if(rc == 0)
    {
      return -1;
    }
  struct nm_connector_entry_s*remote_entry = TBX_MALLOC(sizeof(struct nm_connector_entry_s) + p_connector->addr_len);
 retry_recv:
  rc = NM_SYS(recv)(p_connector->sock, remote_entry, sizeof(struct nm_connector_entry_s) + p_connector->addr_len, 0);
  if(rc == -1)
    {
      const int err = errno;
      if(err == EINTR)
	goto retry_recv;
      else
	{
	  fprintf(stderr, "nmad: FATAL- connector: error while receiving address\n");
	  abort();
	}
    }
  struct nm_connector_entry_s*prev = puk_hashtable_lookup(p_connector->addrs, remote_entry);
  if(prev)
    {
      puk_hashtable_remove(p_connector->addrs, prev);
      TBX_FREE(prev);
    }
  puk_hashtable_insert(p_connector->addrs, remote_entry, remote_entry);
  return 0;
}

static int nm_connector_recv(struct nm_connector_s*p_connector, const char*remote_url, void*remote_addr)
{
  struct nm_connector_entry_s key;
  memcpy(key.url, remote_url, NM_CONNECTOR_URL_SIZE);
  for(;;)
    {
      /* lookup in already received address */
      struct nm_connector_entry_s*remote_entry = puk_hashtable_lookup(p_connector->addrs, &key);
      if(remote_entry != NULL)
	{
	  memcpy(remote_addr, &remote_entry->_addr, p_connector->addr_len);
	  return 0;
	}
      int rc = nm_connector_poll(p_connector);
      if(rc != 0)
	{
	  fprintf(stderr, "nmad: WARNING- connector: timeout while receiving peer address.\n");
	  /* Timeout- we didn't receive peer address in time. We don't know
	   * whether our packet was lost too, and we cannot try to establish
	   * the connection to check. In doubt, return error. Caller will send again.
	   */
	  return -1;
	}
    }
  return 0;
}

static int nm_connector_wait_ack(struct nm_connector_s*p_connector, const char*remote_url)
{
  struct nm_connector_entry_s key;
  memcpy(key.url, remote_url, NM_CONNECTOR_URL_SIZE);
  for(;;)
    {
      struct nm_connector_entry_s*remote_entry = puk_hashtable_lookup(p_connector->addrs, &key);
      if(remote_entry != NULL && remote_entry->ack != 0)
	{
	  return 0;
	}
      int rc = nm_connector_poll(p_connector);
      if(rc != 0)
	{
	  fprintf(stderr, "nmad: WARNING- connector: timeout while waiting for ACK.\n");
	  return -1;
	}
    }
  return 0;
}


int nm_connector_exchange(const char*local_url, const char*remote_url,
			  const void*local_addr, void*remote_addr)
{
  struct nm_connector_s*p_connector = puk_hashtable_lookup(nm_connector.connectors, local_url);
  assert(p_connector != NULL);
  int rc = -1;
  do
    {
      nm_connector_send(p_connector, remote_url, local_addr, 0);
      rc = nm_connector_recv(p_connector, remote_url, remote_addr);
    }
  while(rc != 0);
  do
    {
      nm_connector_send(p_connector, remote_url, local_addr, 1);
      rc = nm_connector_wait_ack(p_connector, remote_url);
    }
  while(rc != 0);
  return rc;
}
