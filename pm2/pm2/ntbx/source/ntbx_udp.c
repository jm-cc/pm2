/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/*
 * Ntbx_udp.c
 * ---------
 */


#include "tbx.h"
#include "ntbx.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <errno.h>

/*
 * Macros and constants
 * ========================================================================
 */

#ifdef SOLARIS_SYS
#define socklen_t int
#endif // SOLARIS_SYS

/* Address size */
#define NTBX_UDP_ADDRESS_SIZE sizeof(ntbx_udp_address_t)

/*
 * Functions
 * ========================================================================
 */


/*
 * Setup functions
 * ---------------
 */


void
ntbx_udp_socket_setup(ntbx_udp_socket_t socket,
		      int               snd_size,
		      int               rcv_size)
{
  int           snd, rcv;
  socklen_t     lg = sizeof(int);
  
  LOG_IN();
  snd = snd_size;
  rcv = rcv_size;
  SYSCALL(setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&snd, lg));
  SYSCALL(setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&rcv, lg));
  LOG_OUT();
}

#if 0
void
ntbx_udp_nb_socket_setup(ntbx_udp_socket_t desc)
{
  int           val    = 1;
  int           packet = 0x8000;
  struct linger ling   = { 1, 50 };
  socklen_t     len    = sizeof(int);
  
  LOG_IN(); 
  SYSCALL(fcntl(desc, F_SETFL, O_NONBLOCK));
  SYSCALL(setsockopt(desc, IPPROTO_UDP, UDP_NODELAY, (char *)&val, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_LINGER, (char *)&ling,
		     sizeof(struct linger)));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_SNDBUF, (char *)&packet, len));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_RCVBUF, (char *)&packet, len));
  LOG_OUT();
}
#endif // 0

ntbx_udp_socket_t 
ntbx_udp_socket_create(p_ntbx_udp_address_t address)
{
  socklen_t          lg  = sizeof(ntbx_udp_address_t);
  ntbx_udp_address_t temp;
  int                desc;

  LOG_IN();
  SYSCALL(desc = socket(AF_INET, SOCK_DGRAM, 0));
  
  temp.sin_family      = AF_INET;
  temp.sin_addr.s_addr = htonl(INADDR_ANY);
  temp.sin_port        = 0;
  
  SYSCALL(bind(desc, (struct sockaddr *)&temp, lg));

  if (address)
    SYSCALL(getsockname(desc, (struct sockaddr *)address, &lg));

  LOG_OUT();
  return (ntbx_udp_socket_t)desc;
}

/*
 * Select
 */

int
ntbx_udp_select(ntbx_udp_socket_t socket, int usec)
{
  struct timeval tv;
  fd_set fds;
  int    result;
  
  LOG_IN();
  tv.tv_sec  = 0;
  tv.tv_usec = usec;
  FD_ZERO(&fds);
  FD_SET(socket, &fds);
  
  SYSTEST(result = select(socket + 1, &fds, NULL, NULL, &tv));
  LOG_OUT();
  return result;
}

/*
 * Send: caller must first check
 *       0 < lg <= NTBX_UDP_MAX_DGRAM_SIZE
 */

int
ntbx_udp_sendto(ntbx_udp_socket_t    socket,
		const void          *ptr,
		const int            lg,
		p_ntbx_udp_address_t p_addr)
{
  int result;

  LOG_IN();
  
  do {
    
    SYSTEST(result = sendto(socket, ptr, lg, 0,
			    (struct sockaddr *)p_addr,
			    NTBX_UDP_ADDRESS_SIZE));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	ERROR("UDP sendto");
      }
    } else if (result == 0) {
      FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
      
  LOG_OUT();
  return 0;
}


/*
 * Recv: caller must first check
 *       0 < lg <= NTBX_UDP_MAX_DGRAM_SIZE
 */

int
ntbx_udp_recvfrom(ntbx_udp_socket_t    socket,
		  void                *ptr,
		  int                  lg,
		  p_ntbx_udp_address_t p_addr)
{
  int result;
  int addr_lg = NTBX_UDP_ADDRESS_SIZE;

  LOG_IN();
  
  do {
    
    SYSTEST(result = recvfrom(socket, ptr, lg, 0,
			      (struct sockaddr *)p_addr, &addr_lg));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	ERROR("UDP recvfrom");
      }
    } else if (result == 0) {
      FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
  
  LOG_OUT();
  return 0;
}


/*
 * Send: caller must first check that total length of all fragments
 *       does not exceed NTBX_UDP_MAX_DGRAM_SIZE
 */

int
ntbx_udp_sendmsg(ntbx_udp_socket_t    socket,
		 p_ntbx_udp_iovec_t   iov,
		 int                  iovlen,
		 p_ntbx_udp_address_t p_addr)
{
  int           result;
  struct msghdr msghdr;
  
  LOG_IN();
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_name    = (caddr_t) p_addr;
  msghdr.msg_namelen = NTBX_UDP_ADDRESS_SIZE;
  msghdr.msg_iov     = (struct iovec *)iov;
  msghdr.msg_iovlen  = iovlen;
  
  do {
    
    SYSTEST(result = sendmsg(socket, &msghdr, 0));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	ERROR("UDP sendto");
      }
    } else if (result == 0) {
      FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
      
  LOG_OUT();
  return 0;
}
		 

/*
 * Recv: caller must first check that total length of all fragments
 *       does not exceed NTBX_UDP_MAX_DGRAM_SIZE
 */

int
ntbx_udp_recvmsg(ntbx_udp_socket_t    socket,
		 p_ntbx_udp_iovec_t   iov,
		 int                  iovlen,
		 p_ntbx_udp_address_t p_addr)
{
  int           result;
  struct msghdr msghdr;
  
  LOG_IN();
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_name    = (caddr_t) p_addr;
  msghdr.msg_namelen = NTBX_UDP_ADDRESS_SIZE;
  msghdr.msg_iov     = (struct iovec *)iov;
  msghdr.msg_iovlen  = iovlen;
  
  do {
    
    SYSTEST(result = recvmsg(socket, &msghdr, 0));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	ERROR("UDP recvfrom");
      }
    } else if (result == 0) {
      FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
  
  LOG_OUT();
  return 0;
}
