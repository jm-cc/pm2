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
 * Mad_udp.c
 * =========
 */


#include "madeleine.h"

#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


/*********************************************************************
 *                        Macros and constants
 *                          ----------------
 *********************************************************************/

DEBUG_DECLARE(udp)

#undef  DEBUG_NAME
#define DEBUG_NAME udp

#ifdef PM2DEBUG
#define MAD_UDP_STATS
#endif // PM2DEBUG

#ifdef MARCEL
#warning "MARCEL support not finished for UDP driver"
#endif // MARCEL

/* VTHD Tests */
//#define VTHD_TEST_ICLUSTER
//#define VTHD_TEST_SCI

#ifdef PM2DEBUG
#define CHECK(cd,msg) \
        if (!(cd)) TBX_FAILURE((msg))
#else  // PM2DEBUG
#define CHECK(cd,msg)
#endif // PM2DEBUG

#ifdef MARCEL
#define TBX_LOCK_SHARED_IF_MARCEL(arg)   TBX_LOCK_SHARED((arg))
#define TBX_UNLOCK_SHARED_IF_MARCEL(arg) TBX_UNLOCK_SHARED((arg))
#else  // MARCEL
#define TBX_LOCK_SHARED_IF_MARCEL(arg)
#define TBX_UNLOCK_SHARED_IF_MARCEL(arg)
#endif // MARCEL



/*
 * Communication protocol
 */

/* Three predefined sizes for UDP socket buffers */
#define MAD_UDP_SO_BUF_LARGE   0x40000
#define MAD_UDP_SO_BUF_MID     0x4000
#define MAD_UDP_SO_BUF_SMALL   0x400

/* Max size in bytes of a UDP datagram:
   65536 does not work under Linux (too big), and too close values involves
   difficulties when several processes are on the same node. */
#define MAD_UDP_MAX_DGRAM_SIZE (0x10000 - 0x400)
#if (defined(VTHD_TEST_ICLUSTER) || defined(VTHD_TEST_SCI))
#define MAD_UDP_SMALL_MAX_DGRAM_SIZE (0x600)
#endif

/* Each node must tell the others what are its request and data ports. This
   two ports are dumped in a string, separated by MAD_UDP_PORT_SEPARATOR. */
#define MAD_UDP_PORT_SEPARATOR '-'
/* Length UDP ports decimal representation */
#define MAD_UDP_PORT_MAX_LENGTH 6
#define MAD_UDP_PORTS_BUF_MAX_LENGTH ((MAD_UDP_PORT_MAX_LENGTH<<2) + 4)

/* Definition of various packet types formats */

/* REQ format: "REQ-%-11d"<data>
   .                <msgid>     */
#define MAD_UDP_REQ_SIZE                 256
#define MAD_UDP_REQ_HD_SIZE              16
#define MAD_UDP_REQ_MAXNO                10000000000 // 10 G
#define MAD_UDP_REQ_SET_MSGID(req,msgid) sprintf((req), "REQ-%-11d", (msgid))
#define MAD_UDP_REQ_MSGID(req)           atoi((req) + 4)
#define MAD_UDP_IS_REQ(req)    (!(strncmp((req), "REQ-", 4) && strncmp((req), "REQ*", 4)))


/* ACK format: "ACK-%-11d:%-15d"
   .               <msgid><bufid> */
#define MAD_UDP_ACK_SIZE                 32
#define MAD_UDP_ACK_SET_MSGID(ack,msgid) sprintf((ack),"ACK-%-11d",(msgid)); *((ack)+15)=':'
#define MAD_UDP_ACK_SET_BUFID(ack,bufid) sprintf((ack)+16,"%-15d",(bufid))
#define MAD_UDP_IS_ACK(ack)    (!(strncmp((ack),"ACK-",4) && strncmp((ack),"ACK*",4)))

/* RRM format: "RRM-%-11d:%-15d"<boolean array>
   .               <msgid><bufid>              */
#define MAD_UDP_RRM_HD_SIZE              32
#define MAD_UDP_RRM_SET_MSGID(rrm,msgid) sprintf((rrm),"RRM-%-11d",(msgid)); *((rrm)+15)=':'
#define MAD_UDP_RRM_SET_BUFID(rrm,bufid) sprintf((rrm)+16,"%-15d",(bufid))
#define MAD_UDP_IS_RRM(rrm)    (!(strncmp((rrm),"RRM-",4) && strncmp((rrm),"RRM*",4)))

/* DGR (datagram) format: "DGR-%-11d:%-15d:%-15d"<data>
   .                          <msgid><bufid><dgrid>    */
#define MAD_UDP_DGRAM_HD_SIZE            48
#define MAD_UDP_DGRAM_BODY_SIZE          (MAD_UDP_MAX_DGRAM_SIZE - MAD_UDP_DGRAM_HD_SIZE)
#if (defined(VTHD_TEST_ICLUSTER) || defined(VTHD_TEST_SCI))
#define MAD_UDP_BIG_DGRAM_BODY_SIZE      (MAD_UDP_MAX_DGRAM_SIZE - MAD_UDP_DGRAM_HD_SIZE)
#define MAD_UDP_SMALL_DGRAM_BODY_SIZE    (MAD_UDP_SMALL_MAX_DGRAM_SIZE - MAD_UDP_DGRAM_HD_SIZE)
#endif
#define MAD_UDP_DGRAM_SET_MSGID(dgr,msgid) sprintf((dgr),"DGR-%-11d",(msgid)); *((dgr)+15)=':'
#define MAD_UDP_DGRAM_SET_BUFID(dgr,bufid) sprintf((dgr)+16,"%-15d",(bufid)); *((dgr)+31)=':'
#define MAD_UDP_DGRAM_SET_DGRID(dgr,dgrid) sprintf((dgr)+32,"%-15d",(dgrid))
#define MAD_UDP_IS_DGRAM_HD(dgr)           (!(strncmp((dgr),"DGR-",4) && strncmp((dgr),"DGR*",4)))

/* Piggy-backing - ACK for ACK with data format:
   "ACK-%-11d\0DGR-%-11d:%-15d:%-15d"<data>"
   . <acked_bufid> <msgid><bufid><dgrid>        */
#define MAD_UDP_ACKDG_HD_SIZE            (MAD_UDP_ACK_SIZE + MAD_UDP_DGRAM_HD_SIZE)
#define MAD_UDP_ACKDG_BODY_SIZE          1968 //+80 = 2048
#if (defined(VTHD_TEST_ICLUSTER) || defined(VTHD_TEST_SCI))
#define MAD_UDP_BIG_ACKDG_BODY_SIZE      1968
#define MAD_UDP_SMALL_ACKDG_BODY_SIZE    944
#endif
#define MAD_UDP_ACKDG_DGR_SIZE           (MAD_UDP_DGRAM_HD_SIZE + MAD_UDP_ACKDG_BODY_SIZE)
#if (defined(VTHD_TEST_ICLUSTER) || defined(VTHD_TEST_SCI))
#define MAD_UDP_BIG_ACKDG_DGR_SIZE       (MAD_UDP_DGRAM_HD_SIZE + MAD_UDP_ACKDG_BODY_SIZE)
#define MAD_UDP_SMALL_ACKDG_DGR_SIZE     (MAD_UDP_DGRAM_HD_SIZE + MAD_UDP_SMALL_ACKDG_BODY_SIZE)
#endif

/* Genaral ... */

#define MAD_UDP_MARK_AS_REEMITTED(buf)   buf[3] = '*'
#define MAD_UDP_UNMARK(buf)              buf[3] = '-'
#define MAD_UDP_IS_REEMITTED(buf)        (buf[3] == '*')


/*********************************************************************
 *                          Local structures
 *                           --------------
 *********************************************************************/


#ifdef SOLARIS_SYS
#define socklen_t int
#endif // SOLARIS_SYS

typedef int                mad_udp_id_t;
typedef struct iovec       mad_udp_iovec_t,   *p_mad_udp_iovec_t;
typedef unsigned int       mad_udp_port_t,    *p_mad_udp_port_t;
typedef struct in_addr     mad_udp_in_addr_t, *p_mad_udp_in_addr_t;
typedef struct sockaddr_in mad_udp_address_t, *p_mad_udp_address_t;
typedef fd_set             mad_udp_fdset_t,   *p_mad_udp_fdset_t;
typedef struct s_mad_udp_socket
{
  int                 desc;
  mad_udp_port_t      port;
#ifdef MARCEL
  volatile tbx_bool_t read_free;
  volatile tbx_bool_t write_free;
#endif // MARCEL
} mad_udp_socket_t,  *p_mad_udp_socket_t;


/* Each node create three sockets:
 *  one to send and receive request;
 *  one to send data and receive requests ack, data acks
 *    or request for reemission (needs a big send buffer);
 *  one to receive data and send ack for ack,
 *    or request for reemission (needs a big receive buffer).
 */
typedef enum e_mad_udp_socket_type
{
  REQUEST,
  SND_DATA,
  RCV_DATA,
  NB_SOCKET
} mad_udp_socket_type_t;


#ifdef MARCEL

/* Poll mode. */
typedef enum e_mad_udp_poll_mode
{
  POLL_READ,
  POLL_SELECT_FOR_READ,
  POLL_SELECT_FOR_WRITE,
} mad_udp_poll_mode_t;

/* Only one receiving thread is allowed, ie only one thread can receive
   packets on REQUEST and RCV_DATA sockets.
   Yet, several threads can send messages at the same time. These sending
   threads have to receive acks on SND_DATA socket. We use MARCEL_POLL to
   receive what is arriving on this socket and wake up the destination thread.
   As the basis algorithm includes timeouts on receiving acks, a sending
   thread must first register to marcel_polling thread, with date and
   timeout value. The marcel_polling thread can check regularly timeouts and
   wake up concerned threads with the appropriate status. */
typedef struct s_mad_udp_poll_arg
{
  unsigned long       timeout_date;
#if PM2DEBUG
  unsigned long       timeout_date_bak;
  char                fname[50];
#endif
  mad_udp_poll_mode_t mode;
  tbx_bool_t          poll_on_socket[NB_SOCKET];
  tbx_bool_t          active_socket[NB_SOCKET];
  p_mad_channel_t     channel;
  p_mad_connection_t  connection;
  mad_udp_port_t      rm_port;
  p_mad_udp_address_t p_rm_addr;
} mad_udp_marcel_poll_arg_t, *p_mad_udp_marcel_poll_arg_t;

#endif // MARCEL


/* Each adapter has got a socket to transmit infos
   on local request and data ports */
typedef struct s_mad_udp_adapter_specific
{
  ntbx_tcp_socket_t tcp_socket;
} mad_udp_adapter_specific_t, *p_mad_udp_adapter_specific_t;


/* The three sockets are created at channel level.
   A field is reserved to store all the recvfrom (or recvmsg) calls. */
typedef struct s_mad_udp_channel_specific
{
  mad_udp_socket_t socket[NB_SOCKET];
  char             request_buffer[MAD_UDP_REQ_SIZE];
#ifdef MARCEL
  TBX_SHARED;
  marcel_pollid_t  pollid;
  mad_udp_fdset_t  rfds;
  mad_udp_fdset_t  wfds;
  int              max_fds; 
  char            *ack;
  int              ack_max_size;
  volatile int     nb_snd_threads;
#endif // MARCEL
#ifdef MAD_UDP_STATS
  int              nb_snd_msg;
  int              nb_rcv_msg;
  int              reemitted_requests;
  int              reemitted_dgrams;
  int              reemitted_acks;
  int              reemitted_afacks;
  int              rrm_nb;
#ifdef MARCEL
  int              old_acks;
  int              wrong_acks;
  int              strange_acks;
  int              old_dgrams;
  int              wrong_dgrams;
  int              strange_dgrams;
  int              old_reqs;
  int              wrong_reqs;
  int              strange_reqs;
  int              old_rrms;
  int              wrong_rrms;
  int              strange_rrms;
#endif // MARCEL
#endif // MAD_UDP_STATS
} mad_udp_channel_specific_t, *p_mad_udp_channel_specific_t;

/* A connection registers remote (rm) ports for all sockets */
typedef struct s_mad_udp_connection_specific
{
  mad_udp_in_addr_t rm_in_addr;
  mad_udp_port_t    rm_port[NB_SOCKET];
  mad_udp_id_t      snd_msgid; // msg to send (or being sent)
  mad_udp_id_t      snd_bufid; // buffer to send (or being sent)
  mad_udp_id_t      rcv_msgid; // msg to be received (or being received)
  mad_udp_id_t      rcv_bufid; // buffer to be received (or being received)
  // Store data arrived with ACK for ACK (data for next buffer in msg)
  char              rcv_early_dgr[MAD_UDP_ACKDG_DGR_SIZE];
  // "Stream control"
  int               pause;
  int               pause_min_size;
#ifdef MARCEL
  //marcel_sem_t      sem_rcv_ack;
  char             *rcv_ack;
  int               rcv_ack_max_size;
#endif // MARCEL
} mad_udp_connection_specific_t, *p_mad_udp_connection_specific_t;



/*********************************************************************
 *                          Function headers
 *                           --------------
 *********************************************************************/

/* Communication protocol utilities */
static void
mad_udp_dgr_extract_ids(char *buf, mad_udp_id_t *msgid,
			mad_udp_id_t *bufid, mad_udp_id_t *dgrid);
static void
mad_udp_ack_rrm_extract_ids(char *buf,
			    mad_udp_id_t *msgid, mad_udp_id_t *bufid);
static void
mad_udp_pause(int usec);

/* Network utilities */
static void
mad_udp_socket_setup(p_mad_udp_socket_t socket,
		     int snd_size, int rcv_size);
static void
mad_udp_socket_create(p_mad_udp_address_t address, p_mad_udp_socket_t socket);

static int
mad_udp_recvfrom(p_mad_udp_socket_t socket, void *ptr,
		 const int lg, p_mad_udp_address_t p_addr);
static int
mad_udp_recvmsg(p_mad_udp_socket_t socket, p_mad_udp_iovec_t iov,
		const int iovlen, p_mad_udp_address_t p_addr);
#ifdef MARCEL
static int
mad_udp_nb_select_r(p_mad_udp_marcel_poll_arg_t poll_arg);
static int
mad_udp_nb_select_w(p_mad_udp_marcel_poll_arg_t poll_arg);
static int
mad_udp_nb_sendto(p_mad_udp_marcel_poll_arg_t poll_arg,
		  const void *ptr, const int lg);
static int
mad_udp_recvfrom(p_mad_udp_socket_t socket, void *ptr,
		 const int lg, p_mad_udp_address_t p_addr);
static int
mad_udp_nb_sendmsg(p_mad_udp_marcel_poll_arg_t poll_arg,
		   p_mad_udp_iovec_t iov, const int iovlen);

#else  // MARCEL

static int
mad_udp_select_r(p_mad_udp_socket_t *socket_set, int set_nb, int msec);
static int
mad_udp_sendto(p_mad_udp_socket_t socket, const void *ptr,
	       const int lg, p_mad_udp_address_t p_addr);
static int
mad_udp_sendmsg(p_mad_udp_socket_t socket, p_mad_udp_iovec_t iov,
		const int iovlen, p_mad_udp_address_t p_addr);

#endif // MARCEL

#ifdef MARCEL
/* Marcel Poll interface functions */
static void
mad_udp_marcel_group(marcel_pollid_t id);
static void *
mad_udp_marcel_poll(marcel_pollid_t id,
		    unsigned active, unsigned sleeping, unsigned blocked);
static void *
mad_udp_marcel_fast_poll(marcel_pollid_t id, any_t arg, tbx_bool_t first_call);
#endif // MARCEL


/* Send/receive request */
static void
mad_udp_sendreq(p_mad_connection_t  connection,
		p_mad_buffer_t      first_buffer,
		p_mad_udp_address_t p_remote_address);
static p_mad_connection_t
mad_udp_recvreq(p_mad_channel_t channel);



/*********************************************************************
 *           Communication protocol utilities static functions
 *                           --------------
 *********************************************************************/

static void
mad_udp_dgr_extract_ids(char *buf, mad_udp_id_t *msgid,
			mad_udp_id_t *bufid, mad_udp_id_t *dgrid)
{
  char *pos = buf + 31;

  *pos = '\0';
  mad_udp_ack_rrm_extract_ids(buf, msgid, bufid);
  *dgrid = atoi(pos + 1);
  *pos = ':';
}

static void
mad_udp_ack_rrm_extract_ids(char *buf,
			    mad_udp_id_t *msgid, mad_udp_id_t *bufid)
{
  char *pos = buf + 15;
  
  *pos   = '\0';
  *msgid = atoi(buf + 4);
  *bufid = atoi(pos + 1);
  *pos   = ':';
}

static void
mad_udp_pause(int usec)
{
  struct timeval tv;

  if (usec <= 0)
    return;
  tv.tv_sec  = 0;
  tv.tv_usec = usec;
  select(0, NULL, NULL, NULL, &tv);
}


/*********************************************************************
 *                 Network utilities static functions
 *                           --------------
 *********************************************************************/


/*
 * Socket create and setup
 */

static void
mad_udp_socket_create(p_mad_udp_address_t address, p_mad_udp_socket_t s)
{
  socklen_t         lg  = sizeof(mad_udp_address_t);
  mad_udp_address_t temp;
  int               desc;

  //LOG_IN();
  SYSCALL(desc = socket(AF_INET, SOCK_DGRAM, 0));
  temp.sin_family      = AF_INET;
  temp.sin_addr.s_addr = htonl(INADDR_ANY);
  temp.sin_port        = htons(0);
  SYSCALL(bind(desc, (struct sockaddr *)&temp, lg));
  if (address)
    SYSCALL(getsockname(desc, (struct sockaddr *)address, &lg));
  s->desc = desc;
  //LOG_OUT();
}

static void
mad_udp_socket_setup(p_mad_udp_socket_t socket,
		     int                snd_size,
		     int                rcv_size)
{
  int           desc, snd, rcv;
  socklen_t     lg = sizeof(int);
  
  //LOG_IN();
  desc = socket->desc;
  snd  = snd_size;
  rcv  = rcv_size;
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_SNDBUF, (char *)&snd, lg));
  SYSCALL(setsockopt(desc, SOL_SOCKET, SO_RCVBUF, (char *)&rcv, lg));
  //LOG_OUT();
}


/*
 * Blocking recvfrom, recvmsg (also used with MARCEL)
 */

static int
mad_udp_recvfrom(p_mad_udp_socket_t  socket,
		 void               *ptr,
		 int                 lg,
		 p_mad_udp_address_t p_addr)
{
  int result;
  int addr_lg = sizeof(mad_udp_address_t);

  LOG_IN();
  CHECK(lg <= MAD_UDP_MAX_DGRAM_SIZE, "mad_udp_recvfrom: message too big.");
  do {
    SYSTEST(result = recvfrom(socket->desc, ptr, lg, 0,
			      (struct sockaddr *)p_addr, &addr_lg));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	TBX_ERROR("UDP recvfrom");
      }
    } else if (result == 0) {
      TBX_FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
  LOG_OUT();
  return 0;
}

static int
mad_udp_recvmsg(p_mad_udp_socket_t  socket,
		p_mad_udp_iovec_t   iov,
		int                 iovlen,
		p_mad_udp_address_t p_addr)
{
  int           result;
  struct msghdr msghdr;
  
  LOG_IN();
#ifdef PM2DEBUG
  {
    int i, lg = 0;
    for (i=0; i < iovlen; i++)
      lg += iov[i].iov_len;
    CHECK(lg <= MAD_UDP_MAX_DGRAM_SIZE, "mad_udp_recvmsg: msg too big.");
  }
#endif // PM2DEBUG
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_name    = (caddr_t) p_addr;
  msghdr.msg_namelen = sizeof(mad_udp_address_t);
  msghdr.msg_iov     = (struct iovec *)iov;
  msghdr.msg_iovlen  = iovlen;
  do {
    SYSTEST(result = recvmsg(socket->desc, &msghdr, 0));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	TBX_ERROR("UDP recvfrom");
      }
    } else if (result == 0) {
      TBX_FAILURE("connection closed");
    } else {
      //LOG_OUT();
      return result;
    }
  } while (result < 0);
  LOG_OUT();
  return 0;
}

#ifdef MARCEL

/*
 * Non blocking select for read and for write
 */

static __inline__ int 
_mad_udp_nb_select(p_mad_udp_marcel_poll_arg_t poll_arg)
{
  p_mad_udp_channel_specific_t channel_specific;
  mad_udp_socket_type_t        st;
  struct timeval               tv;
  fd_set                       fds;
  int                          desc, max_fd, result = 0;

  //LOG_IN();
  channel_specific = poll_arg->channel->specific;
  tv.tv_sec  = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  max_fd     = 0;
  for (st = REQUEST; st < NB_SOCKET; st++) {
    if (poll_arg->poll_on_socket[st]) {
      fprintf(stderr, "select - socket %d : desc=%d, read_free=%d, write_free=%d\n",
	      st, (channel_specific->socket[st]).desc,
	      (channel_specific->socket[st]).read_free, (channel_specific->socket[st]).write_free);
      if ( ((poll_arg->mode == POLL_SELECT_FOR_READ)
	    && (channel_specific->socket[st]).read_free)
	   || ((poll_arg->mode == POLL_SELECT_FOR_WRITE)
	       && (channel_specific->socket[st]).write_free)) {
	desc   = (channel_specific->socket[st]).desc;
	max_fd = tbx_max(max_fd, desc);
	FD_SET(desc, &fds);
	//marcel_mutex_lock(&((channel_specific->socket[st]).mutex));
      }
    }
  }
  if (max_fd) {
    if (poll_arg->mode == POLL_SELECT_FOR_READ) {
      SYSTEST(result = select(max_fd + 1, &fds, NULL, NULL, &tv));
    } else if (poll_arg->mode == POLL_SELECT_FOR_WRITE) {
      SYSTEST(result = select(max_fd + 1, NULL, &fds, NULL, &tv));
    } else {
      TBX_FAILURE("_mad_udp_nb_select: wrong mode.");
    }
    if (result > 0) {
      for (st = REQUEST; st < NB_SOCKET; st++) {
	if ((poll_arg->active_socket[st] = FD_ISSET((channel_specific->socket[st]).desc, &fds))) {
	  //marcel_mutex_unlock(&((channel_specific->socket[st]).mutex));
	  if (poll_arg->mode == POLL_SELECT_FOR_READ)
	    (channel_specific->socket[st]).read_free = tbx_false;
	  else
	    (channel_specific->socket[st]).write_free = tbx_false;
	}
      }
      return result;
    }
  }
  
  memset(poll_arg->active_socket, tbx_false, NB_SOCKET * sizeof(tbx_bool_t));
  marcel_poll(channel_specific->pollid, (any_t)poll_arg);
  result = 0;
  if (poll_arg->timeout_date) {
    for (st = REQUEST; st < NB_SOCKET; st++) {
      if (poll_arg->active_socket[st])
	result++;
	//else
      //  marcel_mutex_unlock(&((channel_specific->socket[st]).mutex));
    }
  }
  
  //LOG_OUT();
  return result;
}

static int
mad_udp_nb_select_r(p_mad_udp_marcel_poll_arg_t poll_arg)
{
  int result;
  //LOG_IN();
  poll_arg->mode = POLL_SELECT_FOR_READ;
  result = _mad_udp_nb_select(poll_arg);
  //LOG_OUT();
  return result;
}

static int
mad_udp_nb_select_w(p_mad_udp_marcel_poll_arg_t poll_arg)
{
  int result;
  //LOG_IN();
  poll_arg->mode = POLL_SELECT_FOR_WRITE;
  result = _mad_udp_nb_select(poll_arg);
  //LOG_OUT();
  return result;
}


/*
 * Non blocking sendto, recvfrom, sendmsg, recvmsg
 */

static int
mad_udp_nb_sendto(p_mad_udp_marcel_poll_arg_t poll_arg,
		  const void *ptr,  const int lg)
{
  p_mad_udp_channel_specific_t channel_specific;
  mad_udp_socket_type_t        st;
  int                          desc, result;

  LOG_IN();
  CHECK(lg <= MAD_UDP_MAX_DGRAM_SIZE, "mad_udp_nb_sendto: message too big.");

  channel_specific = poll_arg->channel->specific;
  for (st = REQUEST; (st < NB_SOCKET && !poll_arg->poll_on_socket[st]); st++);
  CHECK(st < NB_SOCKET, "mad_udp_nb_sendto: wrong poll_arg.");
  TRACE_VAL("poll_arg->poll_on_socket[REQUEST]", poll_arg->poll_on_socket[REQUEST]);
  TRACE_VAL("poll_arg->poll_on_socket[SND_DATA]", poll_arg->poll_on_socket[SND_DATA]);
  TRACE_VAL("poll_arg->poll_on_socket[RCV_DATA]", poll_arg->poll_on_socket[RCV_DATA]);
  desc = (channel_specific->socket[st]).desc;

  poll_arg->timeout_date = (unsigned int)0; // No timeout
#ifdef PM2DEBUG
  poll_arg->timeout_date_bak = 0;
#endif
  do {
    while (mad_udp_nb_select_w(poll_arg) <= 0);
    TRACE_VAL("Sendto to port", ntohs(poll_arg->p_rm_addr->sin_port));
    TRACE_VAL("... on socket", desc);
    SYSTEST(result = sendto(desc, ptr, lg, 0,
			    (struct sockaddr *)poll_arg->p_rm_addr,
			    sizeof(mad_udp_address_t)));
    (channel_specific->socket[st]).write_free = tbx_true;
    //marcel_mutex_unlock(&(socket->mutex));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	TBX_ERROR("UDP sendto");
      }
    } else if (result == 0) {
      TBX_FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
  LOG_OUT();
  return 0;
}

static int
mad_udp_nb_sendmsg(p_mad_udp_marcel_poll_arg_t poll_arg,
		   p_mad_udp_iovec_t iov, const int iovlen)
{
  p_mad_udp_channel_specific_t channel_specific;
  mad_udp_socket_type_t        st;
  int                          desc, result;
  struct msghdr                msghdr;

  LOG_IN();
#ifdef PM2DEBUG
  {
    int i, lg = 0;
    for (i=0; i < iovlen; i++)
      lg += iov[i].iov_len;
    CHECK(lg <= MAD_UDP_MAX_DGRAM_SIZE,
	  "mad_udp_nb_sendmsg: message too big.");
  }
#endif // PM2DEBUG
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_name    = (caddr_t)poll_arg->p_rm_addr;
  msghdr.msg_namelen = sizeof(mad_udp_address_t);
  msghdr.msg_iov     = (struct iovec *)iov;
  msghdr.msg_iovlen  = iovlen;

  channel_specific   = poll_arg->channel->specific;
  for (st = REQUEST; st < NB_SOCKET && !poll_arg->poll_on_socket[st]; st++);
  CHECK(st < NB_SOCKET, "mad_udp_nb_sendmsg: wrong poll_arg.");
  desc = (channel_specific->socket[st]).desc;

  poll_arg->timeout_date = (unsigned int)0; // No timeout
#ifdef PM2DEBUG
  poll_arg->timeout_date_bak = 0;
#endif

  do {
    while (mad_udp_nb_select_w(poll_arg) <= 0);
    TRACE_VAL("Sendmsg to port", ntohs(((p_mad_udp_address_t)msghdr.msg_name)->sin_port));
    TRACE_VAL("... on socket", desc);
    SYSTEST(result = sendmsg(desc, &msghdr, 0));
    (channel_specific->socket[st]).write_free = tbx_true;
    //marcel_mutex_unlock(&(socket->mutex));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	TBX_ERROR("UDP sendmsg");
      }
    } else if (result == 0) {
      TBX_FAILURE("connection closed");
    } else {
      LOG_OUT();
      return result;
    }
  } while (result < 0);
  LOG_OUT();
  return 0;

}

#else  // MARCEL

/*
 * Blocking select for read
 */

static int
mad_udp_select_r(p_mad_udp_socket_t *socket_set, int set_nb, int msec)
{
  struct timeval tv;
  fd_set         fds;
  int            max_fd, i, result;

  //LOG_IN();
  tv.tv_sec  = 0;
  // 1000=1024-16-8 => usec = (2^10 - 2^4 - 2^3) * msec
  tv.tv_usec = (msec << 10) - (msec << 4) - (msec << 3);
  FD_ZERO(&fds);
  max_fd = 0;
  for (i = 0; i < set_nb; i++) {
    max_fd = tbx_max(max_fd, (socket_set[i])->desc);
    FD_SET((socket_set[i])->desc, &fds);
  }
  SYSTEST(result = select(max_fd + 1, &fds, NULL, NULL, &tv));
  if (result > 0) {
    for (i = 0; i < set_nb; i++) {
      if (!FD_ISSET((socket_set[i])->desc, &fds))
	socket_set[i] = NULL;
    }
  }
  //LOG_OUT();
  return result;
}

/*
 * Blocking sendto, sendmsg
 */

static int
mad_udp_sendto(p_mad_udp_socket_t socket, const void         *ptr,
	       const int          lg,     p_mad_udp_address_t p_addr)
{
  int result;

  //LOG_IN();
  CHECK(lg <= MAD_UDP_MAX_DGRAM_SIZE, "mad_udp_sendto: message too big.");
  do {
    SYSTEST(result = sendto(socket->desc, ptr, lg, 0,
			    (struct sockaddr *)p_addr,
			    sizeof(mad_udp_address_t)));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	TBX_ERROR("UDP sendto");
      }
    } else if (result == 0) {
      TBX_FAILURE("connection closed");
    } else {
      //LOG_OUT();
      return result;
    }
  } while (result < 0);
  //LOG_OUT();
  return 0;
}

static int
mad_udp_sendmsg(p_mad_udp_socket_t  socket,
		p_mad_udp_iovec_t   iov,
		int                 iovlen,
		p_mad_udp_address_t p_addr)
{
  int           result;
  struct msghdr msghdr;

  //LOG_IN();
#ifdef PM2DEBUG
  {
    int i, lg = 0;
    for (i=0; i < iovlen; i++)
      lg += iov[i].iov_len;
    CHECK(lg <= MAD_UDP_MAX_DGRAM_SIZE, "mad_udp_sendmsg: message too big.");
  }
#endif // PM2DEBUG
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_name    = (caddr_t) p_addr;
  msghdr.msg_namelen = sizeof(mad_udp_address_t);
  msghdr.msg_iov     = (struct iovec *)iov;
  msghdr.msg_iovlen  = iovlen;
  do {
    SYSTEST(result = sendmsg(socket->desc, &msghdr, 0));
    if (result == -1) {
      if ((errno != EAGAIN) && (errno != EINTR)) {
	TBX_ERROR("UDP sendto");
      }
    } else if (result == 0) {
      TBX_FAILURE("connection closed");
    } else {
      //LOG_OUT();
      return result;
    }
  } while (result < 0);
  //LOG_OUT();
  return 0;
}

#endif // MARCEL


#ifdef MARCEL

/*********************************************************************
 *                      Marcel polling functions
 *                           --------------
 *********************************************************************/

static void
mad_udp_marcel_group(marcel_pollid_t id)
{
  p_mad_udp_marcel_poll_arg_t  arg     = NULL;
  p_mad_udp_channel_specific_t ch_sp   = NULL;
  int                          max_fds = 0;
  mad_udp_socket_type_t        st;

  //LOG_IN();
  if ((arg = FIRST_POLL(id))) {

    ch_sp = arg->channel->specific;
    FD_ZERO(&(ch_sp->rfds));
    FD_ZERO(&(ch_sp->wfds));
    FOREACH_POLL(id) { GET_ARG(id, arg);
      for (st = REQUEST; st < NB_SOCKET; st++) {
	if (arg->poll_on_socket[st]) {
	  int desc = (ch_sp->socket[st]).desc;
	  max_fds = tbx_max(max_fds, desc);
	  FD_SET(desc, (arg->mode == POLL_SELECT_FOR_WRITE) ? &(ch_sp->wfds) : &(ch_sp->rfds));
	}
      }
    }
    ch_sp->max_fds = max_fds;
  }
  //LOG_OUT();
}

static void *
mad_udp_marcel_poll(marcel_pollid_t id,
		    unsigned active, unsigned sleeping, unsigned blocked)
{
  p_mad_udp_marcel_poll_arg_t  arg        = NULL;
  p_mad_udp_channel_specific_t ch_sp      = NULL;
  int                          select_result;
  tbx_bool_t                   isset[2][NB_SOCKET];
  
  //LOG_IN();

  if ((arg = FIRST_POLL(id))) {

    ch_sp = arg->channel->specific;
    
    /* Select on polled sockets */
    {
      mad_udp_fdset_t       rfds, wfds;
      struct timeval        tv;
      mad_udp_socket_type_t st;
#ifdef PM2DEBUG
      int                   nb_polled_socket = 0;
#endif //PM2DEBUG
      
      memcpy(&rfds, &(ch_sp->rfds), sizeof(mad_udp_fdset_t));
      memcpy(&wfds, &(ch_sp->wfds), sizeof(mad_udp_fdset_t));
      tv.tv_sec  = 0;
      tv.tv_usec = 0;
      SYSTEST(select_result = select(ch_sp->max_fds + 1, &rfds, &wfds, NULL, &tv));

      if (select_result > 0) {
	LOG_VAL("mad_udp_marcel_poll: select result", select_result);
	// Update isset : 1st col for rfds, and 2nd col for wfds
	isset[0][REQUEST]  = FD_ISSET((ch_sp->socket[REQUEST]).desc,  &rfds);
	isset[0][SND_DATA] = FD_ISSET((ch_sp->socket[SND_DATA]).desc, &rfds);
	isset[0][RCV_DATA] = FD_ISSET((ch_sp->socket[RCV_DATA]).desc, &rfds);
	isset[1][REQUEST]  = FD_ISSET((ch_sp->socket[REQUEST]).desc,  &wfds);
	isset[1][SND_DATA] = FD_ISSET((ch_sp->socket[SND_DATA]).desc, &wfds);
	isset[1][RCV_DATA] = FD_ISSET((ch_sp->socket[RCV_DATA]).desc, &wfds);
#ifdef PM2DEBUG
	for (st=REQUEST; st < NB_SOCKET; st++) {
	  if (isset[0][st])
	    nb_polled_socket++;
	  if (isset[1][st])
	    nb_polled_socket++;
	}
#endif // PM2DEBUG
      }
    }
    
    /* Process select result */
    {
      // receiving thread pollinst
      p_mad_udp_marcel_poll_arg_t rcv_th_arg = NULL;
      // When an ACK or RRM is to read ...
      int               addr_lg = sizeof(mad_udp_address_t);
      int               size    = -1;
      mad_udp_address_t remote_address;
      mad_udp_in_addr_t rm_in_addr;
      mad_udp_port_t    rm_port = 0;
      unsigned long     now     = marcel_clock();

      FOREACH_POLL(id) { GET_ARG(id, arg);
	p_mad_udp_connection_specific_t con_sp;
	p_mad_udp_socket_t              p_socket;

	if (select_result > 0) {

	  if (arg->poll_on_socket[RCV_DATA]) {
	    // This pollinst was registered by THE receiving thread
	    p_socket = &(ch_sp->socket[RCV_DATA]);

	    if (isset[0][RCV_DATA] && (p_socket->read_free)
		&& (arg->mode == POLL_SELECT_FOR_READ)) {
	      isset[0][RCV_DATA]  = tbx_false;
	      p_socket->read_free = tbx_false;
	      arg->active_socket[RCV_DATA] = tbx_true;
	      CANCEL_POLL(id);
	      continue;
	    }

	    if (isset[1][RCV_DATA] && (p_socket->write_free)
		&& (arg->mode == POLL_SELECT_FOR_WRITE)) {
	      isset[1][RCV_DATA]   = tbx_false;
	      p_socket->write_free = tbx_false;
	      arg->active_socket[RCV_DATA] = tbx_true;
	      CANCEL_POLL(id);
	    }
	    continue;
	  }

	  if (arg->poll_on_socket[REQUEST]) {
	    p_socket = &(ch_sp->socket[REQUEST]);

	    if (isset[0][REQUEST] && (p_socket->read_free)
		&& (arg->mode == POLL_SELECT_FOR_READ)) {
	      // This pollinst was registered by THE receiving thread
	      CHECK(arg->poll_on_socket[SND_DATA] &&
		    !arg->poll_on_socket[RCV_DATA], "Wrong poll argument !");
	      isset[0][REQUEST]           = tbx_false;
	      arg->active_socket[REQUEST] = tbx_true;
	      rcv_th_arg = arg; // Cancel is at end of loop
	      // do not "continue" (if isset[0][SND_DATA], we want to know it)
	    }

	    if (isset[1][REQUEST] && (p_socket->write_free)
		&& (arg->mode == POLL_SELECT_FOR_WRITE)) {
	      // This pollinst was registered by a sending thread
	      isset[1][REQUEST]           = tbx_false;
	      p_socket->write_free        = tbx_false;
	      arg->active_socket[REQUEST] = tbx_true;
	      CANCEL_POLL(id);
	      continue;
	    }
	  }

	  if (arg->poll_on_socket[SND_DATA]) {
	    p_socket = &(ch_sp->socket[SND_DATA]);

	    if (isset[0][SND_DATA] && (p_socket->read_free)) {
	      if (arg->mode == POLL_READ) {
		// This pollinst was registered by a sending thread
		// READ mode has priority on SELECT mode:
		if (rcv_th_arg)
		  rcv_th_arg->active_socket[SND_DATA] = tbx_false;

		if (size == -1 && p_socket->read_free) {
		  // recvfrom has not been called yet
		  int desc = (ch_sp->socket[SND_DATA]).desc;

		  SYSTEST(size = recvfrom(desc, ch_sp->ack, ch_sp->ack_max_size,
					  0, (struct sockaddr *)&remote_address,
					  &addr_lg));
		  rm_in_addr = remote_address.sin_addr;
		  rm_port    = remote_address.sin_port;
		}
		// Test if the received ACK or RRM was for the thread of arg
		con_sp = arg->connection->specific;
		if ((rm_port == arg->rm_port)
		    && (rm_in_addr.s_addr == con_sp->rm_in_addr.s_addr)) {
		  isset[0][SND_DATA]  = tbx_false;
		  p_socket->read_free = tbx_false;
		  size = 0;
		  arg->active_socket[SND_DATA] = tbx_true;
		  memcpy(con_sp->rcv_ack, ch_sp->ack, size);
		  memcpy(arg->p_rm_addr, &remote_address, addr_lg);
		  CANCEL_POLL(id);
		  continue;
		}

	      } else if (arg->mode == POLL_SELECT_FOR_READ) {
		// This pollinst was registered by THE receiving thread (in recvreq)
		CHECK(!arg->poll_on_socket[RCV_DATA], "Wrong poll argument !");
		// process the incoming ACK only if no sending thread is alive
		if (!ch_sp->nb_snd_threads) {
		  arg->active_socket[SND_DATA] = tbx_true;
		  rcv_th_arg = arg;
		  // do not update isset[0][SND_DATA] because READ mode have
		  //   priority, update only rcv_th_arg to cancel it at end of
		  //   the loop if no thread has read on SND_DATA socket;
		  continue;
		}
	      }
	    }

	    if (isset[1][SND_DATA] && (p_socket->write_free)
		&& (arg->mode == POLL_SELECT_FOR_WRITE)) {
	      // This pollinst was registered by a sending thread
	      isset[1][SND_DATA]   = tbx_false;
	      p_socket->write_free = tbx_false;
	      arg->active_socket[SND_DATA] = tbx_true;
	      CANCEL_POLL(id);
	      continue;
	    }
	  }
	}

	if ((arg->timeout_date) && (arg->timeout_date <= now)) {
	  arg->timeout_date = (unsigned int) 0;
	  CANCEL_POLL(id);
	}
      }

      if (size > 0) {
	/* This case occurs when none of ID's pollinsts is waiting for the ACK
	   or RRM which has been read on SND_DATA.
	   Give it to the first POLL_READ mode pollinst (which exists, since
	   recvfrom function was called) ... */
	FOREACH_POLL(id) { GET_ARG(id, arg);
	  if (arg->mode == POLL_READ) {
	    p_mad_udp_connection_specific_t con_sp = arg->connection->specific;
	    arg->active_socket[SND_DATA] = tbx_true;
	    (ch_sp->socket[SND_DATA]).read_free = tbx_false;
	    memcpy(con_sp->rcv_ack, ch_sp->ack, size);
	    memcpy(arg->p_rm_addr, &remote_address, addr_lg);
	    CANCEL_POLL(id);
	    break;
	  }
	}
      }
      if (rcv_th_arg) {
	if (rcv_th_arg->active_socket[REQUEST])
	  (ch_sp->socket[REQUEST]).read_free = tbx_false;
	if (rcv_th_arg->active_socket[SND_DATA])
	  (ch_sp->socket[SND_DATA]).read_free = tbx_false;
	// LOG_OUT();
	return MARCEL_POLL_SUCCESS_FOR(rcv_th_arg);
      }
    }
  }

  //LOG_OUT();
  return MARCEL_POLL_FAILED;
}

static void *
mad_udp_marcel_fast_poll(marcel_pollid_t id, any_t arg,
			 tbx_bool_t first_call TBX_UNUSED)
{
  void                        *status   = MARCEL_POLL_FAILED;
  p_mad_udp_marcel_poll_arg_t  poll_arg = NULL;
  p_mad_udp_channel_specific_t ch_sp    = NULL;
  mad_udp_fdset_t              rfds, wfds;
  int                          select_result;

  //LOG_IN();
  if ((poll_arg = (p_mad_udp_marcel_poll_arg_t) arg)) {

    ch_sp = poll_arg->channel->specific;

    /* Select on polled sockets */
    {
      int                   max_fds = 0;
      mad_udp_socket_type_t st;
      struct timeval        tv;
#ifdef PM2DEBUG
      int                   nb_polled_socket = 0;
#endif //PM2DEBUG
      
      FD_ZERO(&rfds);
      FD_ZERO(&wfds);
      for (st = REQUEST; st < NB_SOCKET; st++) {
	if (poll_arg->poll_on_socket[st]) {
	  int desc = (ch_sp->socket[st]).desc;
#ifdef PM2DEBUG
	  nb_polled_socket++;
#endif // PM2DEBUG
	  max_fds = tbx_max(max_fds, desc);
	  FD_SET(desc, (poll_arg->mode == POLL_SELECT_FOR_WRITE) ? &wfds : &rfds);
	}
      }

      tv.tv_sec  = 0;
      tv.tv_usec = 0;
      SYSTEST(select_result = select(max_fds + 1, &rfds, &wfds, NULL, &tv));
    }
    
    /* Process select result */

    if (select_result > 0) {

      DISP_VAL("mad_udp_marcel_fast_poll: select result", select_result);

      if (poll_arg->poll_on_socket[RCV_DATA]) {
	// The pollinst was registered by THE receiving thread
	if ((poll_arg->mode == POLL_SELECT_FOR_READ)
	    && (ch_sp->socket[RCV_DATA]).read_free
	    && FD_ISSET((ch_sp->socket[RCV_DATA]).desc, &rfds)) {
	  (ch_sp->socket[RCV_DATA]).read_free = tbx_false;
	  poll_arg->active_socket[RCV_DATA]   = tbx_true;
	  //LOG_OUT();
	  return MARCEL_POLL_SUCCESS(id);
	}
	if (poll_arg->mode == POLL_SELECT_FOR_WRITE
	    && (ch_sp->socket[RCV_DATA]).write_free
	    && FD_ISSET((ch_sp->socket[RCV_DATA]).desc, &wfds)) {
	  (ch_sp->socket[RCV_DATA]).write_free = tbx_false;
	  poll_arg->active_socket[RCV_DATA]    = tbx_true;
	  //LOG_OUT();
	  return MARCEL_POLL_SUCCESS(id);
	}
	// POLL_READ should not occur (since dgrams might be too big for being read here)
	// POLL_WRITE is useless since only one thread has to write on RCV_DATA
      }

      if (poll_arg->poll_on_socket[REQUEST]) {
	if ((poll_arg->mode == POLL_SELECT_FOR_READ)
	    && (ch_sp->socket[REQUEST]).read_free
	    && (FD_ISSET((ch_sp->socket[REQUEST]).desc, &rfds))) {
	  // The pollinst was registered by THE receiving thread
          CHECK(poll_arg->poll_on_socket[SND_DATA] &&
	        !poll_arg->poll_on_socket[RCV_DATA], "Wrong poll argument !");
	  poll_arg->active_socket[REQUEST] = tbx_true;
	  // do not return: process first SND_DATA socket
	  status = MARCEL_POLL_SUCCESS(id);
        }

	if ((poll_arg->mode == POLL_SELECT_FOR_WRITE)
	    && (ch_sp->socket[REQUEST]).write_free
	    && FD_ISSET((ch_sp->socket[REQUEST]).desc,  &wfds)) {
	  (ch_sp->socket[REQUEST]).write_free = tbx_false;
	  poll_arg->active_socket[REQUEST]    = tbx_true;
	  //LOG_OUT();
	  return MARCEL_POLL_SUCCESS(id);
	}
      }

      if (poll_arg->poll_on_socket[SND_DATA]) {
	if ((ch_sp->socket[SND_DATA]).read_free
	    && FD_ISSET((ch_sp->socket[SND_DATA]).desc, &rfds)) {
	  if (poll_arg->mode == POLL_READ) {
	    // The pollinst was registered by a sending thread
	    int size, desc, addr_lg = sizeof(mad_udp_address_t);
	    p_mad_udp_connection_specific_t con_sp = NULL;

	    desc = (ch_sp->socket[SND_DATA]).desc;
	    SYSTEST(size = recvfrom(desc, ch_sp->ack, ch_sp->ack_max_size,
				    0, (struct sockaddr *)poll_arg->p_rm_addr,
				    &addr_lg));

	    // Test if the received ACK or RRM was for the thread of poll_arg
	    con_sp = poll_arg->connection->specific;
	    poll_arg->active_socket[SND_DATA] = tbx_true;
	    (ch_sp->socket[SND_DATA]).read_free = tbx_false;
	    memcpy(con_sp->rcv_ack, ch_sp->ack, size);
	    //LOG_OUT();
	    return MARCEL_POLL_SUCCESS(id);

	  } else if (poll_arg->mode == POLL_SELECT_FOR_READ) {
	    // The pollinst was registered by THE receiving thread
	    CHECK(!poll_arg->poll_on_socket[RCV_DATA], "Wrong poll argument !");
	    // process the incoming ACK only if no sending thread is alive
	    if (!ch_sp->nb_snd_threads) {
	      poll_arg->active_socket[SND_DATA] = tbx_true;
	      status = MARCEL_POLL_SUCCESS(id);
	      // return at end
	    }
	  }
	}

	if ((poll_arg->mode == POLL_SELECT_FOR_WRITE)
	    && FD_ISSET((ch_sp->socket[SND_DATA]).desc, &wfds)) {
	  (ch_sp->socket[SND_DATA]).write_free = tbx_false;
	  poll_arg->active_socket[SND_DATA]    = tbx_true;
	  //LOG_OUT();
	  return MARCEL_POLL_SUCCESS(id);
	}
      }
    }

    if ((status == MARCEL_POLL_FAILED)
	&& (poll_arg->timeout_date)
	&& (poll_arg->timeout_date <= marcel_clock())) {
      poll_arg->timeout_date = (unsigned int) 0;
      CANCEL_POLL(id);
    }

    if (status != MARCEL_POLL_FAILED) {
      if (poll_arg->active_socket[REQUEST])
	(ch_sp->socket[REQUEST]).read_free  = tbx_false;
      if (poll_arg->active_socket[SND_DATA])
	(ch_sp->socket[SND_DATA]).read_free = tbx_false;
    }
  }
  
  //LOG_OUT();
  return status;
}

#endif // MARCEL



/*********************************************************************
 *                  Request utilities static functions
 *                           --------------
 *********************************************************************/

/**
 * Send REQ and wait for its ACK.
 * REQ packet is filled as much as possible with FIRST_BUFFER data.
 */
static void
mad_udp_sendreq(p_mad_connection_t  connection,
		p_mad_buffer_t      first_buffer,
		p_mad_udp_address_t p_remote_address)
{
  p_mad_udp_connection_specific_t specific;
  p_mad_udp_channel_specific_t    channel_specific;
  int                             bytes_read;
  p_mad_udp_socket_t              so_req, so_snd;
  mad_udp_in_addr_t               rm_in_addr;
  mad_udp_port_t                  rm_rcv_port;
  mad_udp_id_t                    msgid, bufid;
  mad_udp_address_t               remote_address;
  tbx_bool_t                      ack_received;
  char                            req[MAD_UDP_REQ_SIZE];
#ifdef MARCEL
  char                           *ack;
  mad_udp_marcel_poll_arg_t       poll_arg;
#else  // MARCEL
  p_mad_udp_socket_t              tmp_so_snd;
  char                            ack[MAD_UDP_RRM_HD_SIZE];
  int                             result;
#endif // MARCEL

#ifdef MAD_UDP_STATS
  int                             loop_cnt = -1;
#endif // MAD_UDP_STATS

  
  LOG_IN();
  specific         = connection->specific;
  channel_specific = connection->channel->specific;
  
  so_req           = &(channel_specific->socket[REQUEST]);
  so_snd           = &(channel_specific->socket[SND_DATA]);
  rm_in_addr       = specific->rm_in_addr;
  rm_rcv_port      = specific->rm_port[RCV_DATA];
  bytes_read       = tbx_min(first_buffer->bytes_written,
			 MAD_UDP_REQ_SIZE - MAD_UDP_REQ_HD_SIZE);

  MAD_UDP_REQ_SET_MSGID(req, specific->snd_msgid);
  memcpy(req + MAD_UDP_REQ_HD_SIZE, first_buffer->buffer, bytes_read);

#ifdef MARCEL
  // Common values: set them only once
  poll_arg.poll_on_socket[RCV_DATA] = tbx_false;
  poll_arg.channel                  = connection->channel;
  poll_arg.connection               = connection;
  poll_arg.rm_port                  = specific->rm_port[RCV_DATA];
#ifdef PM2DEBUG
  sprintf(poll_arg.fname, "sendreq");
#endif
#endif // MARCEL

  ack_received = tbx_false;

  do {
    
#ifdef MAD_UDP_STATS
    loop_cnt++;
#endif // MAD_UDP_STATS

    /* (Re)Send request */

#ifdef MARCEL
    poll_arg.poll_on_socket[REQUEST]  = tbx_true;
    poll_arg.poll_on_socket[SND_DATA] = tbx_false;
    poll_arg.p_rm_addr                = p_remote_address;
    mad_udp_nb_sendto(&poll_arg, req, MAD_UDP_REQ_SIZE);
#else  // MARCEL
    mad_udp_sendto(so_req, req, MAD_UDP_REQ_SIZE, p_remote_address);
#endif // MARCEL


    /* Wait for ack. */

#ifdef MARCEL
    // Minimum values (initial) for specific->rcv_ack_max_size
    // and channel_specific->ack_max_size are right => no need for realloc.
    //poll_arg.timeout_date = (unsigned int)0; // done by mad_udp_nb_sendto
    poll_arg.rm_port = rm_rcv_port;
    while (!poll_arg.timeout_date) {
      poll_arg.timeout_date             = marcel_clock() + 100;
#ifdef PM2DEBUG
      poll_arg.timeout_date_bak         = marcel_clock() + 100;
#endif
      poll_arg.mode                     = POLL_READ;
      poll_arg.poll_on_socket[REQUEST]  = tbx_false;
      poll_arg.poll_on_socket[SND_DATA] = tbx_true;
      poll_arg.p_rm_addr                = &remote_address;
      marcel_poll(channel_specific->pollid, (any_t)&poll_arg);
      (channel_specific->socket[SND_DATA]).read_free = tbx_true;
      if (!poll_arg.timeout_date)
	continue;
      // else
      ack = specific->rcv_ack;
#else  // MARCEL
    tmp_so_snd = so_snd;
    while ((result = mad_udp_select_r(&tmp_so_snd, 1, 100)) != 0) {
      if (result < 0)
	continue;
      mad_udp_recvfrom(so_snd,
		       ack, tbx_max(MAD_UDP_ACK_SIZE, MAD_UDP_RRM_HD_SIZE),
		       &remote_address);
#endif // MARCEL
      /* If first ACK is lost, then timeout on RCV_DATA socket make
	 receiver send a request for reemission.
	 That is why we choose to use an empty RRM for request ACK. */
      if (MAD_UDP_IS_RRM(ack)) {
	mad_udp_ack_rrm_extract_ids(ack, &msgid, &bufid);
	if ((msgid == specific->snd_msgid) && (bufid == 0)
	    && (remote_address.sin_addr.s_addr == rm_in_addr.s_addr)
	    && (remote_address.sin_port == rm_rcv_port)) {
	  ack_received = tbx_true;
	  break;
#if defined(MAD_UDP_STATS) && defined(MARCEL)
	} else { // Ignore all other RRM's
	  TBX_LOCK_SHARED(channel_specific);
	  if ((p_remote_address->sin_addr.s_addr != rm_in_addr.s_addr)
	      || (p_remote_address->sin_port != rm_rcv_port)) {
	    channel_specific->strange_rrms++;
	  } else if (msgid < specific->snd_msgid) {
	    channel_specific->old_rrms++;
	  } else {
	    channel_specific->wrong_rrms++;
	  }
	  TBX_UNLOCK_SHARED(channel_specific);
#endif // MAD_UDP_STATS & MARCEL
	}
	// Ignore all other RRM's
      } else if (MAD_UDP_IS_ACK(ack)) {
	/* An ACK of a buffer "End of reception".
	   It means that ACK for ACK was lost => resend it. */
#ifdef MARCEL
	if (MAD_UDP_IS_REEMITTED(ack)) {
	  mad_udp_nb_sendto(&poll_arg, ack, MAD_UDP_ACK_SIZE);
	  (channel_specific->socket[SND_DATA]).write_free = tbx_true;
#ifdef MAD_UDP_STATS
	  TBX_LOCK_SHARED(channel_specific);
	  channel_specific->reemitted_afacks++;
	  TBX_UNLOCK_SHARED(channel_specific);
#endif // MAD_UDP_STATS
	}
#else  // MARCEL
	mad_udp_sendto(so_snd, ack, MAD_UDP_ACK_SIZE, &remote_address);
#endif // MARCEL
#ifdef MAD_UDP_STATS
	channel_specific->reemitted_afacks++;
#endif // MAD_UDP_STATS
      }
    } // while (select) || while (!poll_arg.timeout_date)

    MAD_UDP_MARK_AS_REEMITTED(req);

  } while (!ack_received);

  CHECK((msgid <= specific->snd_msgid) || (msgid > specific->snd_msgid + 100),
	"Request ACK or RRM msgid too high !");

#ifdef MAD_UDP_STATS
  TBX_LOCK_SHARED_IF_MARCEL(channel_specific);
  channel_specific->reemitted_requests += loop_cnt;
  TBX_UNLOCK_SHARED_IF_MARCEL(channel_specific);
#endif // MAD_UDP_STATS
  
  first_buffer->bytes_read += bytes_read;

  LOG_OUT();
}


/**
 * Receive REQ and send its ACK
 * The data attached to the request are stored
 * in channel->specific->request_buffer.
 */
static p_mad_connection_t
mad_udp_recvreq(p_mad_channel_t channel)
{
  p_mad_udp_channel_specific_t    specific;
  mad_udp_address_t               remote_address;
  mad_udp_id_t                    msgid = 0;
  p_mad_connection_t              in;
  p_mad_udp_connection_specific_t in_specific = NULL;
  tbx_darray_index_t              idx;
  char                            buf[tbx_max(MAD_UDP_ACK_SIZE,
					  MAD_UDP_RRM_HD_SIZE)];
#ifdef MARCEL
  mad_udp_marcel_poll_arg_t       poll_arg;
#else  // MARCEL
  p_mad_udp_socket_t              sockets[2];
#endif // MARCEL

  //LOG_IN();
  LOG_STR("mad_udp_recvreq", channel->name);
  specific = channel->specific;

  /* Receive first request from unknown connection */

#ifdef MARCEL
  poll_arg.timeout_date             = (unsigned int)0; // No timeout
#ifdef PM2DEBUG
  poll_arg.timeout_date_bak         = 0;
#endif
  poll_arg.mode                     = POLL_SELECT_FOR_READ;
  poll_arg.poll_on_socket[REQUEST]  = tbx_true;
  poll_arg.poll_on_socket[SND_DATA] = tbx_true;
  poll_arg.poll_on_socket[RCV_DATA] = tbx_false;
  poll_arg.channel                  = channel;
  poll_arg.connection               = NULL;
  poll_arg.p_rm_addr                = &remote_address;
#ifdef PM2DEBUG
  sprintf(poll_arg.fname, "recvreq");
#endif
#endif // MARCEL

  do {
    mad_udp_port_t remote_port;
    unsigned long  remote_in_addr;
    tbx_bool_t     req_received = tbx_false;

#ifdef MARCEL
    memset(poll_arg.active_socket, tbx_false,
	   NB_SOCKET * (sizeof(tbx_bool_t)));
    marcel_poll(specific->pollid, (any_t)&poll_arg);
    fprintf(stderr, "recvreq: %d%d\n", poll_arg.active_socket[SND_DATA], poll_arg.active_socket[REQUEST]);

    if (poll_arg.active_socket[SND_DATA]) {
      // This means that an ACK or RRM has come on SND_DATA socket,
      // which no sending threads has required => probably lost AFACK.
      // It is impossible to maintain piggy-backing : so process only ACKs of message last buffer.
      mad_udp_recvfrom(&(specific->socket[SND_DATA]), buf, MAD_UDP_ACK_SIZE, &remote_address);
      (specific->socket[SND_DATA]).read_free = tbx_true;
      fprintf(stderr, "*** Received ACK: \"%s\" on socket %d, of channel \"%s\"\n", buf, (specific->socket[SND_DATA]).desc, channel->name);

      /* Search for THE connection matching remote_address */
      remote_port    = remote_address.sin_port;
      remote_in_addr = remote_address.sin_addr.s_addr;
      idx = -1;
      do {
	in = tbx_darray_next_idx(channel->in_connection_darray, &idx);
      } while ((in)
	       && (in_specific = in->specific)
	       && ((in_specific->rm_port[REQUEST]  != remote_port)
		   || (in_specific->rm_in_addr.s_addr != remote_in_addr)));
      if (in && MAD_UDP_IS_ACK(buf) && MAD_UDP_IS_REEMITTED(buf)) {
	mad_udp_id_t msgid, bufid;
	mad_udp_ack_rrm_extract_ids(buf, &msgid, &bufid);
	// Then it is really a lost AFACK for last buffer of a previous message
	poll_arg.poll_on_socket[REQUEST] = tbx_false;
	mad_udp_nb_sendto(&poll_arg, buf, MAD_UDP_ACK_SIZE);
	poll_arg.poll_on_socket[REQUEST] = tbx_true;
	poll_arg.mode                    = POLL_SELECT_FOR_READ;
      }
      // else ignore
    }

    if (poll_arg.active_socket[REQUEST]) {
      mad_udp_recvfrom(&(specific->socket[REQUEST]),
		       specific->request_buffer, MAD_UDP_REQ_SIZE,
		       &remote_address);
      (specific->socket[REQUEST]).read_free = tbx_true;
      if (MAD_UDP_IS_REQ(specific->request_buffer)) {
	msgid = MAD_UDP_REQ_MSGID(specific->request_buffer);
	req_received = tbx_true;
      } else {
#ifdef MAD_UDP_STATS
	TBX_LOCK_SHARED(specific);
	specific->wrong_reqs++;
	TBX_UNLOCK_SHARED(specific);
#endif // MAD_UDP_STATS
	continue;
      }
    }

#else  // MARCEL
    sockets[0] = &(specific->socket[REQUEST]);
    sockets[1] = &(specific->socket[SND_DATA]);
    while (mad_udp_select_r(sockets, 2, 1000) <= 0);
    
    if (sockets[1]) { // SND_DATA
      // This means that an ACK has come on socket SND_DATA, and thus that
      // an AFACK has been lost. This case can occur only when it was the
      // AFACK of the last buffer of a message, and when the sending node
      // has "decided" to receive a message afterwards.
      mad_udp_recvfrom(&(specific->socket[SND_DATA]), buf,
		       MAD_UDP_ACK_SIZE, &remote_address);
      if (MAD_UDP_IS_ACK(buf)) {
	mad_udp_sendto(&(specific->socket[SND_DATA]), buf,
		       MAD_UDP_ACK_SIZE, &remote_address);
      }
      // else ignore
    }

    if (sockets[0]) {
      mad_udp_recvfrom(&(specific->socket[REQUEST]),
		       specific->request_buffer,
		       MAD_UDP_REQ_SIZE, &remote_address);
      if (MAD_UDP_IS_REQ(specific->request_buffer)) {
	msgid = MAD_UDP_REQ_MSGID(specific->request_buffer);
	req_received = tbx_true;
      } else {
	continue;
      }

    }
#endif // MARCEL

    if (!req_received)
      continue;
    /* Search for THE connexion matching remote_address */
    remote_port    = remote_address.sin_port;
    remote_in_addr = remote_address.sin_addr.s_addr;
    idx = -1;
    do {
      in = tbx_darray_next_idx(channel->in_connection_darray, &idx);
    } while ((in)
	     && (in_specific = in->specific)
	     && ((in_specific->rm_port[REQUEST]  != remote_port)
		 || (in_specific->rm_in_addr.s_addr != remote_in_addr)));
    
    // Ignore doubled requests (msgid < in_specific->rcv_msgid)
    if ((in) && (msgid == in_specific->rcv_msgid))
      break;
#if defined(MAD_UDP_STATS) && defined(MARCEL)
    TBX_LOCK_SHARED(specific);
    if (msgid < in_specific->rcv_msgid)
      specific->old_reqs++;
    if (msgid > in_specific->rcv_msgid)
      specific->strange_reqs++;
    TBX_UNLOCK_SHARED(specific);
#endif // MAD_UDP_STATS && MARCEL

    CHECK(msgid <= in_specific->rcv_msgid, "REQ msgid too high !");
  } while (tbx_true);


  /* Then send ACK on remote SND_DATA port:
     if this ack is lost, timeout on RCV_DATA socket will make receiver
     send a request for reemission (or an ack if message consists of one
     request only), which will be taken as an ack by sender side.
     That is why we already send an empty RRM as request ACK. */
  MAD_UDP_RRM_SET_MSGID(buf, msgid);
  MAD_UDP_RRM_SET_BUFID(buf, 0);
  remote_address.sin_port = in_specific->rm_port[SND_DATA];
#ifdef MARCEL
  poll_arg.timeout_date = 0; // No timeout
#ifdef PM2DEBUG
  poll_arg.timeout_date_bak = 0;
#endif
  poll_arg.poll_on_socket[REQUEST]  = tbx_false;
  poll_arg.poll_on_socket[SND_DATA] = tbx_false;
  poll_arg.poll_on_socket[RCV_DATA] = tbx_true;
  memset(poll_arg.active_socket, tbx_false, NB_SOCKET * (sizeof(tbx_bool_t)));
  poll_arg.connection               = in;
  mad_udp_nb_sendto(&poll_arg, buf, MAD_UDP_RRM_HD_SIZE);
#else  // MARCEL
  mad_udp_sendto(&(specific->socket[RCV_DATA]),
		 buf, MAD_UDP_RRM_HD_SIZE, &remote_address);
#endif // MARCEL

  LOG_OUT();
  return in;
}



/*********************************************************************
 *                       Init and exit functions
 *                           --------------
 *********************************************************************/

void
mad_udp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef PM2DEBUG
  DEBUG_INIT(udp);
#endif // PM2DEBUG
  
  //LOG_IN();
  interface = driver->interface;
  
  driver->connection_type  = mad_bidirectional_connection;
  driver->buffer_alignment = 32;
  driver->name             = tbx_strdup("udp");
  
  interface->driver_init              = mad_udp_driver_init;
  interface->adapter_init             = mad_udp_adapter_init;
  interface->channel_init             = mad_udp_channel_init;
  interface->before_open_channel      = NULL;
  interface->connection_init          = mad_udp_connection_init;
  interface->link_init                = mad_udp_link_init;
  interface->accept                   = mad_udp_accept;
  interface->connect                  = mad_udp_connect;
  interface->after_open_channel       = NULL;
  interface->before_close_channel     = NULL; 
  interface->disconnect               = mad_udp_disconnect;
  interface->after_close_channel      = NULL;
  interface->link_exit                = NULL;
  interface->connection_exit          = NULL;
  interface->channel_exit             = mad_udp_channel_exit;
  interface->adapter_exit             = mad_udp_adapter_exit;
  interface->driver_exit              = NULL;
  interface->choice                   = NULL;
  interface->get_static_buffer        = NULL;
  interface->return_static_buffer     = NULL;
  interface->new_message              = mad_udp_new_message;
  interface->finalize_message         = mad_udp_finalize_message;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message             = NULL; //mad_udp_poll_message;
#endif // MAD_MESSAGE_POLLING 
  interface->receive_message          = mad_udp_receive_message;
  interface->message_received         = mad_udp_message_received;
  interface->send_buffer              = mad_udp_send_buffer;
  interface->receive_buffer           = mad_udp_receive_buffer;
  interface->send_buffer_group        = mad_udp_send_buffer_group;
  interface->receive_sub_buffer_group = mad_udp_receive_sub_buffer_group;
  //LOG_OUT();
}

void
mad_udp_driver_init(p_mad_driver_t driver, int *argc, char ***argv)
{
  //LOG_IN();
  TRACE("Initializing UDP driver");
  driver->specific = NULL;
  //LOG_OUT();
}

void
mad_udp_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_udp_adapter_specific_t specific         = NULL;
  p_tbx_string_t               parameter_string = NULL;
  ntbx_tcp_address_t           address;
  
  //LOG_IN();
  if (strcmp(adapter->dir_adapter->name, "default"))
    TBX_FAILURE("unsupported adapter");
  
  specific = TBX_MALLOC(sizeof(mad_udp_adapter_specific_t));
  specific->tcp_socket = -1;
  adapter->specific    = specific;
#ifdef SSIZE_MAX
  adapter->mtu         = tbx_min(SSIZE_MAX, MAD_FORWARD_MAX_MTU);
#else // SSIZE_MAX
  adapter->mtu         = MAD_FORWARD_MAX_MTU;
#endif // SSIZE_MAX
  
  /* A TCP socket is created on each node. It will be used only for
     for channel initialisations */
  specific->tcp_socket = ntbx_tcp_socket_create(&address, 0);
  SYSCALL(listen(specific->tcp_socket, tbx_min(5, SOMAXCONN)));
  
  /* adapter->parameter is only the TCP port */
  parameter_string   =
    tbx_string_init_to_int(ntohs(address.sin_port));
  adapter->parameter = tbx_string_to_cstring(parameter_string);
  tbx_string_free(parameter_string);
  
  //LOG_OUT();
}

void
mad_udp_channel_init(p_mad_channel_t channel)
{
  p_mad_udp_channel_specific_t specific;
  mad_udp_address_t            address;
  int                          i;
  
  //LOG_IN();
  specific = TBX_MALLOC(sizeof(mad_udp_channel_specific_t));
#ifdef MARCEL
  TBX_INIT_SHARED(specific);
  specific->pollid = marcel_pollid_create(mad_udp_marcel_group,
					  mad_udp_marcel_poll,
					  mad_udp_marcel_fast_poll,
					  (MARCEL_POLL_AT_TIMER_SIG
					   | MARCEL_POLL_AT_YIELD
					   | MARCEL_POLL_AT_IDLE));
  //specific->max_fds        = 0;
  specific->ack_max_size   = tbx_max(MAD_UDP_ACK_SIZE, MAD_UDP_RRM_HD_SIZE);
  specific->ack            = TBX_MALLOC(specific->ack_max_size);
  specific->nb_snd_threads = 0;
#endif // MARCEL

  /*
   * Init channel sockets
   */
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->socket[i].desc = -1;
    specific->socket[i].port = htons(0);
  }
  for (i = REQUEST; i < NB_SOCKET; i++) {
    p_mad_udp_socket_t p_sock = &(specific->socket[i]);

    mad_udp_socket_create(&address, p_sock);
    p_sock->port = (mad_udp_port_t)address.sin_port;
#ifdef MARCEL
    p_sock->read_free  = tbx_true;
    p_sock->write_free = tbx_true;
#endif // MARCEL
  }
  
  mad_udp_socket_setup(&(specific->socket[REQUEST]),
		       MAD_UDP_SO_BUF_SMALL, MAD_UDP_SO_BUF_LARGE);
  mad_udp_socket_setup(&(specific->socket[SND_DATA]),
		       MAD_UDP_SO_BUF_LARGE, MAD_UDP_SO_BUF_MID);
  mad_udp_socket_setup(&(specific->socket[RCV_DATA]),
		       MAD_UDP_SO_BUF_MID, MAD_UDP_SO_BUF_LARGE);
  memset(specific->request_buffer, 0, MAD_UDP_REQ_SIZE);

#ifdef MAD_UDP_STATS
  specific->nb_snd_msg         = 0;
  specific->nb_rcv_msg         = 0;
  specific->reemitted_requests = 0;
  specific->reemitted_dgrams   = 0;
  specific->reemitted_acks     = 0;
  specific->reemitted_afacks   = 0;
  specific->rrm_nb             = 0;
#ifdef MARCEL
  specific->old_acks           = 0;
  specific->wrong_acks         = 0;
  specific->strange_acks       = 0;
  specific->old_dgrams         = 0;
  specific->wrong_dgrams       = 0;
  specific->strange_dgrams     = 0;
  specific->old_reqs           = 0;
  specific->wrong_reqs         = 0;
  specific->strange_reqs       = 0;
  specific->old_rrms           = 0;
  specific->wrong_rrms         = 0;
  specific->strange_rrms       = 0;
#endif // MARCEL
#endif // MAD_UDP_STATS
  
  channel->specific = specific;
  //LOG_OUT();
}

void
mad_udp_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_udp_connection_specific_t specific = NULL;
  int                             i;
  
  //LOG_IN();
  specific = TBX_MALLOC(sizeof(mad_udp_connection_specific_t));
  specific->rm_in_addr.s_addr =  0;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->rm_port[i] = htons(0);
  }
  specific->snd_msgid        = 0;
  specific->snd_bufid        = 0;
  specific->rcv_msgid        = 0;
  specific->rcv_bufid        = 0;
  memset(specific->rcv_early_dgr, 0, MAD_UDP_ACKDG_DGR_SIZE);
  specific->pause            = 0;
  specific->pause_min_size   = 0;
#ifdef MARCEL
  specific->rcv_ack_max_size = tbx_max(MAD_UDP_ACK_SIZE, MAD_UDP_RRM_HD_SIZE);
  specific->rcv_ack          = TBX_MALLOC(specific->rcv_ack_max_size);
#endif // MARCEL
  in->specific = out->specific = specific;
  in->nb_link  = 1;
  out->nb_link = 1;
  //LOG_OUT();
}

void 
mad_udp_link_init(p_mad_link_t lnk)
{
  //LOG_IN();
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  lnk->specific    = NULL;
  //LOG_OUT();
}

void
mad_udp_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t adapter_info)
{
  p_mad_udp_channel_specific_t    channel_specific;
  p_mad_udp_connection_specific_t connection_specific;
  p_mad_udp_adapter_specific_t    adapter_specific;
  // int<->string ports manipulation
  char  ports_buffer[MAD_UDP_PORTS_BUF_MAX_LENGTH];
  char *port, *separator;
  int   i;
  // connection to remote node
  ntbx_tcp_address_t remote_address;
  int                lg             = sizeof(ntbx_tcp_address_t);
  ntbx_tcp_socket_t  tcp_socket;

  //LOG_IN();
  channel_specific    = in->channel->specific;
  connection_specific = in->specific;
  adapter_specific    = in->channel->adapter->specific;
  
  /*
   * Accept remote ports send
   */
  TRACE_STR("Accept for channel", in->channel->name);

  SYSCALL(tcp_socket = accept(adapter_specific->tcp_socket,
			      (struct sockaddr *)(&remote_address), &lg));
  ntbx_tcp_socket_setup(tcp_socket);
  // ... do not loose time : send before receive ...

  /*
   * Send local req and data ports to distant node
   */
  
  // Prepare ports buffer
  sprintf(ports_buffer, "%d%c%d%c%d",
	  ntohs(channel_specific->socket[REQUEST].port),
	  MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->socket[SND_DATA].port),
	  MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->socket[RCV_DATA].port));
  // Send local ports
  ntbx_tcp_write(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  
  fprintf(stderr, "  Local ports : %s; Local sockets : ", ports_buffer);
  sprintf(ports_buffer, "%d%c%d%c%d",
	  channel_specific->socket[REQUEST].desc,
	  MAD_UDP_PORT_SEPARATOR,
	  channel_specific->socket[SND_DATA].desc,
	  MAD_UDP_PORT_SEPARATOR,
	  channel_specific->socket[RCV_DATA].desc);
  fprintf(stderr, "%s\n", ports_buffer);

  /*
   * Receive req and data ports from distant node
   */
 
  ntbx_tcp_read(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  TRACE_STR("  Remote ports", ports_buffer);

  connection_specific->rm_in_addr =
    (mad_udp_in_addr_t)remote_address.sin_addr;
  port = ports_buffer;
  for (i = REQUEST; i < RCV_DATA; i++) {
    separator  = strchr(port, MAD_UDP_PORT_SEPARATOR);
    *separator = '\0';
    connection_specific->rm_port[i] = htons(atoi(port));
    port       = 1 + separator;
  }
  connection_specific->rm_port[i] = htons(atoi(port));

  SYSCALL(close(tcp_socket));
  //LOG_OUT();
}

void
mad_udp_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t adapter_info)
{
  p_mad_udp_channel_specific_t    channel_specific;
  p_mad_udp_connection_specific_t connection_specific;
  p_mad_udp_adapter_specific_t    adapter_specific;
  // int<->string ports manipulation
  char  ports_buffer[MAD_UDP_PORTS_BUF_MAX_LENGTH];
  char *port, *separator;
  int   i;
  // connection to remote node
  ntbx_tcp_address_t remote_address;
  ntbx_tcp_socket_t  tcp_socket;
  
  //LOG_IN();
  channel_specific    = out->channel->specific;
  connection_specific = out->specific;
  adapter_specific    = out->channel->adapter->specific;
  
  TRACE_STR("Connect for channel", out->channel->name);

  /*
   * Send local req and data ports to distant node
   */
  
  // Prepare ports buffer
  sprintf(ports_buffer, "%d%c%d%c%d",
	  ntohs(channel_specific->socket[REQUEST].port),
	  MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->socket[SND_DATA].port),
	  MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->socket[RCV_DATA].port));
  // Connect a TCP client socket
  tcp_socket = ntbx_tcp_socket_create(NULL, 0);
  ntbx_tcp_address_fill_ip(&remote_address,
			   atoi(adapter_info->dir_adapter->parameter),
			   &adapter_info->dir_node->ip);
  ntbx_tcp_socket_setup(tcp_socket);
  SYSCALL(connect(tcp_socket, (struct sockaddr *)&remote_address, 
		  sizeof(ntbx_tcp_address_t)));
  // Send local ports
  ntbx_tcp_write(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  fprintf(stderr, "  Local ports : %s; Local sockets : ", ports_buffer);
  sprintf(ports_buffer, "%d%c%d%c%d",
	  channel_specific->socket[REQUEST].desc,
	  MAD_UDP_PORT_SEPARATOR,
	  channel_specific->socket[SND_DATA].desc,
	  MAD_UDP_PORT_SEPARATOR,
	  channel_specific->socket[RCV_DATA].desc);
  fprintf(stderr, "%s\n", ports_buffer);
  
  /*
   * Receive req and data ports from distant node
   */
  
  ntbx_tcp_read(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  TRACE_STR("  Remote ports", ports_buffer);

  connection_specific->rm_in_addr =
    (mad_udp_in_addr_t)remote_address.sin_addr;
  port = ports_buffer;
  for (i = REQUEST; i < RCV_DATA; i++) {
    separator  = strchr(port, MAD_UDP_PORT_SEPARATOR);
    *separator = '\0';
    connection_specific->rm_port[i] = htons(atoi(port));
    port       = 1 + separator;
  }
  connection_specific->rm_port[i] = htons(atoi(port));
  
  SYSCALL(close(tcp_socket));
  //LOG_OUT();
}

void
mad_udp_disconnect(p_mad_connection_t connection)
{
  p_mad_udp_connection_specific_t specific = NULL;
  int i;
  
  //LOG_IN();
  specific = connection->specific;
  specific->rm_in_addr.s_addr = 0;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->rm_port[i] = htons(0);
  }
  specific->rcv_bufid = 0;
  specific->snd_bufid = 0;
#ifdef MAD_UDP_STATS
  DISP_VAL("connection pause         ", specific->pause);
  DISP_VAL("connection pause min size", specific->pause_min_size);
#endif // MAD_UDP_STATS
#ifdef MARCEL
  if (specific->rcv_ack) {
    CHECK(specific->rcv_ack_max_size != 0,
	  "Mismanagement of connection->specific->rcv_ack");
    TBX_FREE(specific->rcv_ack);
    specific->rcv_ack          = NULL;
    specific->rcv_ack_max_size = 0;
  }
#endif //MARCEL
  TBX_FREE(specific);
  connection->specific = NULL;
  //LOG_OUT();
}

void
mad_udp_channel_exit(p_mad_channel_t channel)
{
  p_mad_udp_channel_specific_t specific = NULL;
  int i;
  
  //LOG_IN();
  specific = channel->specific;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    SYSCALL(close(specific->socket[i].desc));
    specific->socket[i].port = htons(0);
  }
#ifdef MARCEL
  if (specific->ack) {
    CHECK(specific->ack_max_size != 0,
	  "Mismanagement of channel->specific->ack_tmp");
    TBX_FREE(specific->ack);
    specific->ack          = NULL;
    specific->ack_max_size = 0;
  }
#endif // MARCEL
#ifdef MAD_UDP_STATS
  DISP("Channel stats:");
  DISP_VAL("Sent msg                 ", specific->nb_snd_msg);
  DISP_VAL("Received msg             ", specific->nb_rcv_msg);
  DISP_VAL("Reemitted requests       ", specific->reemitted_requests);
  DISP_VAL("Reemitted dgrams         ", specific->reemitted_dgrams);
  DISP_VAL("Reemitted acks           ", specific->reemitted_acks);
  DISP_VAL("Reemitted acks for acks  ", specific->reemitted_afacks);
#ifdef MARCEL
  DISP_VAL("Old acks                 ", specific->old_acks);
  DISP_VAL("Not awaited acks         ", specific->wrong_acks);
  DISP_VAL("Acks coming too early    ", specific->strange_acks);
  DISP_VAL("Doubled dgrams           ", specific->old_dgrams);
  DISP_VAL("Not awaited dgrams       ", specific->wrong_dgrams);
  DISP_VAL("Dgrams coming too early  ", specific->strange_dgrams);
  DISP_VAL("Doubled requests         ", specific->old_reqs);
  DISP_VAL("Not awaited requests     ", specific->wrong_reqs);
  DISP_VAL("Requests coming too early", specific->strange_reqs);
#endif // MARCEL
  DISP_VAL("Requests for reemission  ", specific->rrm_nb);
#ifdef MARCEL
  DISP_VAL("Doubled req for reem.    ", specific->old_rrms);
  DISP_VAL("Not awaited rrms         ", specific->wrong_rrms);
  DISP_VAL("RRM coming too early     ", specific->strange_rrms);
#endif // MARCEL
#endif // MAD_UDP_STATS
  TBX_FREE(specific);
  channel->specific = NULL;
  //LOG_OUT();
}

void
mad_udp_adapter_exit(p_mad_adapter_t adapter)
{
  //LOG_IN();
  TBX_FREE(adapter->specific);
  adapter->specific = NULL;
  //LOG_OUT();
}



/*********************************************************************
 *                     Message exchange functions
 *                           --------------
 *********************************************************************/

void
mad_udp_new_message(p_mad_connection_t out)
{
  p_mad_udp_connection_specific_t specific;
  
  LOG_IN();
#ifdef MARCEL
  {
    p_mad_udp_channel_specific_t ch_sp = out->channel->specific;
    
    TBX_LOCK_SHARED(ch_sp);
    ch_sp->nb_snd_threads++;
    TBX_UNLOCK_SHARED(ch_sp);
  }
#endif // MARCEL
  specific = out->specific;
  specific->snd_bufid = 0;
  LOG_OUT();
}

void
mad_udp_finalize_message(p_mad_connection_t out)
{
  p_mad_udp_connection_specific_t specific;
  p_mad_udp_channel_specific_t    channel_specific;
  mad_udp_address_t               remote_address;
  char                            afack[MAD_UDP_ACK_SIZE];
#ifdef MARCEL
  mad_udp_marcel_poll_arg_t       poll_arg;
#endif // MARCEL

  LOG_IN();
  specific = out->specific;
  channel_specific = out->channel->specific;
#ifdef MARCEL
  TBX_LOCK_SHARED(channel_specific);
  channel_specific->nb_snd_threads--;
  TBX_UNLOCK_SHARED(channel_specific);
#endif // MARCEL

  /* Send ACK for ACK of message last buffer */
  MAD_UDP_ACK_SET_MSGID(afack, specific->snd_msgid);
  MAD_UDP_ACK_SET_BUFID(afack, specific->snd_bufid - 1);
  remote_address.sin_family = AF_INET;
  remote_address.sin_addr   = specific->rm_in_addr;
  remote_address.sin_port   = specific->rm_port[RCV_DATA];
  memset(remote_address.sin_zero, '\0', 8);
#ifdef MARCEL
  poll_arg.timeout_date             = (unsigned int)0; // No timeout
#ifdef PM2DEBUG
  poll_arg.timeout_date_bak = 0;
  sprintf(poll_arg.fname, "finalize_message");
#endif
  poll_arg.poll_on_socket[REQUEST]  = tbx_false;
  poll_arg.poll_on_socket[SND_DATA] = tbx_true;
  poll_arg.poll_on_socket[RCV_DATA] = tbx_false;
  memset(poll_arg.active_socket, tbx_false, NB_SOCKET * (sizeof(tbx_bool_t)));
  poll_arg.channel                  = out->channel;
  poll_arg.connection               = out;
  poll_arg.p_rm_addr                = &remote_address;
  mad_udp_nb_sendto(&poll_arg, afack, MAD_UDP_ACK_SIZE);  
#else  // MARCEL
  mad_udp_sendto(&(channel_specific->socket[SND_DATA]),
		 afack, MAD_UDP_ACK_SIZE, &remote_address);
#endif // MARCEL
  specific->snd_msgid = (specific->snd_msgid + 1) % MAD_UDP_REQ_MAXNO;
  specific->snd_bufid = 0;
#ifdef PM2DEBUG
  if (!specific->snd_msgid)
    DISP("*** Tour du compteur de req snd ***");
#endif // PM2DEBUG
#ifdef MAD_UDP_STATS
  {
    p_mad_udp_channel_specific_t cs;
    cs = out->channel->specific;
    TBX_LOCK_SHARED_IF_MARCEL(cs);
    cs->nb_snd_msg++;
    TBX_UNLOCK_SHARED_IF_MARCEL(cs);
  }
#endif // MAD_UDP_STATS
  LOG_OUT();
}


p_mad_connection_t
mad_udp_receive_message(p_mad_channel_t channel)
{
  p_mad_connection_t              in;
  p_mad_udp_connection_specific_t in_specific;
 
  in = mad_udp_recvreq(channel);
  in_specific = in->specific;
  in_specific->rcv_bufid = 0;
  return in;
}

void
mad_udp_message_received(p_mad_connection_t in)
{
  p_mad_udp_connection_specific_t specific;

  LOG_IN();
  specific = in->specific;
  specific->rcv_msgid = (specific->rcv_msgid + 1) % MAD_UDP_REQ_MAXNO;
#ifdef PM2DEBUG
  if (!specific->rcv_msgid)
    DISP("*** Tour du compteur de req rcv ***");
#endif // PM2DEBUG
#ifdef MAD_UDP_STATS
  {
    p_mad_udp_channel_specific_t cs = in->channel->specific;
    TBX_LOCK_SHARED_IF_MARCEL(cs);
    cs->nb_rcv_msg++;
    TBX_UNLOCK_SHARED_IF_MARCEL(cs);
  }
#endif // MAD_UDP_STATS
  LOG_OUT();
}


/**
 * Send buffer
 */

#ifdef VTHD_TEST_ICLUSTER
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_SMALL_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_SMALL_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_SMALL_ACKDG_DGR_SIZE
static tbx_bool_t print_send_values = tbx_true;
#endif // VTHD_TEST_ICLUSTER

#ifdef VTHD_TEST_SCI
// All three values are right (BIG == default, since they have not been redefined)
static tbx_bool_t print_send_values = tbx_true;
#endif // VTHD_TEST_SCI

void
mad_udp_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  p_mad_connection_t              connection;
  p_mad_udp_connection_specific_t connection_specific;
  p_mad_udp_channel_specific_t    channel_specific;
  mad_udp_address_t               remote_address;
  mad_udp_id_t                    bufid, msgid;
  char                            afack[MAD_UDP_ACKDG_HD_SIZE];
#ifdef MARCEL
  mad_udp_marcel_poll_arg_t       poll_arg;
#else  // MARCEL
  p_mad_udp_socket_t              so_snd;
#endif // MARCEL
  
  LOG_IN();

#if (defined(VTHD_TEST_ICLUSTER) || defined(VTHD_TEST_SCI))
  if (print_send_values) {
    print_send_values = tbx_false;
#ifdef VTHD_TEST_ICLUSTER
    DISP_VAL("ICLUSTER send: MAD_UDP_DGRAM_BODY_SIZE", MAD_UDP_DGRAM_BODY_SIZE);
    DISP_VAL("ICLUSTER send: MAD_UDP_ACKDG_BODY_SIZE", MAD_UDP_ACKDG_BODY_SIZE);
    DISP_VAL("ICLUSTER send: MAD_UDP_ACKDG_DGR_SIZE",  MAD_UDP_ACKDG_DGR_SIZE);
#endif // VTHD_TEST_ICLUSTER
#ifdef VTHD_TEST_SCI
    DISP_VAL("SCI send: MAD_UDP_DGRAM_BODY_SIZE", MAD_UDP_DGRAM_BODY_SIZE);
    DISP_VAL("SCI send: MAD_UDP_ACKDG_BODY_SIZE", MAD_UDP_ACKDG_BODY_SIZE);
    DISP_VAL("SCI send: MAD_UDP_ACKDG_DGR_SIZE",  MAD_UDP_ACKDG_DGR_SIZE);
#endif // VTHD_TEST_SCI
  }
#endif // VTHD_TEST_ICLUSTER || VTHD_TEST_SCI

  connection          = lnk->connection;
  connection_specific = connection->specific;
  channel_specific    = connection->channel->specific;
  bufid               = connection_specific->snd_bufid;
  msgid               = connection_specific->snd_msgid;
#ifdef MARCEL
  poll_arg.timeout_date             = (unsigned int)0; // No timeout
#ifdef PM2DEBUG
  poll_arg.timeout_date_bak = 0;
  sprintf(poll_arg.fname, "send_buffer");
#endif // PM2DEBUG
  poll_arg.poll_on_socket[REQUEST]  = tbx_false;
  poll_arg.poll_on_socket[SND_DATA] = tbx_true;
  poll_arg.poll_on_socket[RCV_DATA] = tbx_false;
  memset(poll_arg.active_socket, tbx_false, NB_SOCKET * (sizeof(tbx_bool_t)));
  poll_arg.channel                  = connection->channel;
  poll_arg.connection               = connection;
  poll_arg.p_rm_addr                = &remote_address;
#else  // MARCEL
  so_snd = &(channel_specific->socket[SND_DATA]);
#endif // MARCEL

  remote_address.sin_family = AF_INET;
  remote_address.sin_addr   = connection_specific->rm_in_addr;
  memset(remote_address.sin_zero, '\0', 8);
  
  /*
   * First, send request (with some data).
   */
  
  if (!bufid) {
    remote_address.sin_port = connection_specific->rm_port[REQUEST];
    mad_udp_sendreq(connection, buffer, &remote_address);
  }

  /*
   * Then send remaining data.
   */

  remote_address.sin_port = connection_specific->rm_port[RCV_DATA];

  if (mad_more_data(buffer)) {

    int                bytes_read, lg_to_snd, lg_snt;
    void              *ptr;
    int                nb_packet, last_packet_size;
    char              *dgram_hd;
    mad_udp_port_t     rm_rcv_port;
    mad_udp_in_addr_t  rm_rcv_in_addr;
    mad_udp_iovec_t    iov[2];
    tbx_bool_t         do_pause;
    char              *to_send;
    int                ack_rrm_len, reemission_nb;
    char              *ack_rrm;

    bytes_read       = buffer->bytes_read;
    lg_to_snd        = buffer->bytes_written - bytes_read;
    ptr              = (void *)buffer->buffer;
    nb_packet        = 0;
    last_packet_size = 0;
    dgram_hd         = (char *) afack + MAD_UDP_ACK_SIZE;
    rm_rcv_port      = connection_specific->rm_port[RCV_DATA];
    rm_rcv_in_addr   = connection_specific->rm_in_addr;
    do_pause         = tbx_false;

    MAD_UDP_DGRAM_SET_MSGID(dgram_hd, msgid);
    MAD_UDP_DGRAM_SET_BUFID(dgram_hd, bufid);

    /* Send ACK for ACK of previous buffer if exists */
    if (bufid) {
      MAD_UDP_ACK_SET_MSGID(afack, msgid);
      MAD_UDP_ACK_SET_BUFID(afack, bufid - 1);
      MAD_UDP_DGRAM_SET_DGRID(dgram_hd, nb_packet);
      last_packet_size = tbx_min(lg_to_snd, MAD_UDP_ACKDG_BODY_SIZE);
      iov[0].iov_base  = afack;
      iov[0].iov_len   = MAD_UDP_ACKDG_HD_SIZE;
      iov[1].iov_base  = (caddr_t)(ptr + bytes_read);
      iov[1].iov_len   = last_packet_size;
#ifdef MARCEL
      lg_snt = mad_udp_nb_sendmsg(&poll_arg, iov, 2);
#else  // MARCEL
      lg_snt = mad_udp_sendmsg(so_snd, iov, 2, &remote_address);
#endif // MARCEL
      CHECK(lg_snt == last_packet_size + MAD_UDP_ACKDG_HD_SIZE,
	    "First packet not fully sent !");
      bytes_read += last_packet_size;
      lg_to_snd  -= last_packet_size;
    }

    iov[0].iov_base = dgram_hd;
    iov[0].iov_len  = MAD_UDP_DGRAM_HD_SIZE;

    /*
     * Send buffer in MAX_DGRAM_SIZE-length packets.
     * Count number of sent packets and set last_packet_size.
     */
    do_pause = (connection_specific->pause
		&& (buffer->length >= connection_specific->pause_min_size));

    while (lg_to_snd > 0) {

      MAD_UDP_DGRAM_SET_DGRID(dgram_hd, nb_packet);
      last_packet_size = tbx_min(lg_to_snd, MAD_UDP_DGRAM_BODY_SIZE);
      iov[1].iov_base  = (caddr_t)(ptr + bytes_read);
      iov[1].iov_len   = last_packet_size;

      if (do_pause)
	mad_udp_pause(connection_specific->pause);
#ifdef MARCEL
      lg_snt = mad_udp_nb_sendmsg(&poll_arg, iov, 2);
#else  // MARCEL
      lg_snt = mad_udp_sendmsg(so_snd, iov, 2, &remote_address);
#endif // MARCEL
      nb_packet++;
      CHECK(lg_snt == last_packet_size + MAD_UDP_DGRAM_HD_SIZE,
	    "Packet not fully sent !");
      bytes_read += last_packet_size;
      lg_to_snd  -= last_packet_size;
    }
    
    /*
     * Keep on reemitting specified packets as long as requests
     * for reemission arrive.
     */
    
    ack_rrm_len = tbx_max(MAD_UDP_ACK_SIZE, MAD_UDP_RRM_HD_SIZE + nb_packet);
#ifdef MARCEL
    if (channel_specific->ack_max_size < ack_rrm_len) {
      TBX_LOCK_SHARED(channel_specific);
      channel_specific->ack =
	TBX_REALLOC(channel_specific->ack, ack_rrm_len);
      TBX_UNLOCK_SHARED(channel_specific);
    }
    if (connection_specific->rcv_ack_max_size < ack_rrm_len) {
      connection_specific->rcv_ack =
	TBX_REALLOC(connection_specific->rcv_ack, ack_rrm_len);
    }
    poll_arg.rm_port      = rm_rcv_port;
    poll_arg.timeout_date = 0;
#ifdef PM2DEBUG
    poll_arg.timeout_date_bak = 0;
#endif
    ack_rrm = connection_specific->rcv_ack;
#else  // MARCEL
    ack_rrm = TBX_MALLOC(ack_rrm_len);
#endif // MARCEL
    to_send = ack_rrm + MAD_UDP_RRM_HD_SIZE;
    memset(to_send, 0xFF, nb_packet);

    reemission_nb = 0;

    do {
      tbx_bool_t   reemit = tbx_false;
      mad_udp_id_t rcv_msgid, rcv_bufid;
      
      /* Wait for ACK or RRM on SND_DATA socket. */
#ifdef MARCEL
      poll_arg.mode = POLL_READ;
      marcel_poll(channel_specific->pollid, (any_t)&poll_arg);
      (channel_specific->socket[SND_DATA]).read_free = tbx_true;
      ack_rrm = connection_specific->rcv_ack;
#else  // MARCEL
      mad_udp_recvfrom(so_snd, ack_rrm, ack_rrm_len, &remote_address);
#endif // MARCEL
      if (MAD_UDP_IS_RRM(ack_rrm)) {
	mad_udp_ack_rrm_extract_ids(ack_rrm, &rcv_msgid, &rcv_bufid);
	reemit =
	  ((rcv_msgid == msgid && rcv_bufid == bufid)
	   && (remote_address.sin_port == rm_rcv_port)
	   && (remote_address.sin_addr.s_addr == rm_rcv_in_addr.s_addr));
	// if (!reemit) ignore
#if defined(MAD_UDP_STATS) && defined(MARCEL)
	if (!reemit) { // Ignore all other RRM's
	  TBX_LOCK_SHARED(channel_specific);
	  if ((remote_address.sin_addr.s_addr != rm_rcv_in_addr.s_addr)
	      || (remote_address.sin_port != rm_rcv_port))
	    channel_specific->strange_rrms++;
	  else if (rcv_msgid < msgid || rcv_bufid < bufid)
	    channel_specific->old_rrms++;
	  else
	    channel_specific->wrong_rrms++;
	  TBX_UNLOCK_SHARED(channel_specific);
	}
#endif // MAD_UDP_STATS && MARCEL

      } else if (MAD_UDP_IS_ACK(ack_rrm)) {
	mad_udp_ack_rrm_extract_ids(ack_rrm, &rcv_msgid, &rcv_bufid);
	if ((rcv_msgid == msgid)
	    && (remote_address.sin_port == rm_rcv_port)
	    && (remote_address.sin_addr.s_addr == rm_rcv_in_addr.s_addr)) {
	  if (rcv_bufid == bufid) {
	    /* Buffer ACK received. */
	    break;
	  } else if (rcv_bufid == bufid - 1) {
	    /* Received ACK for previous buffer, which means that
	       ACK for ACK for previous buffer was lost, so resend it with
	       buffer beginning */
	    MAD_UDP_ACK_SET_MSGID(afack, msgid);
	    MAD_UDP_ACK_SET_BUFID(afack, rcv_bufid);
	    MAD_UDP_DGRAM_SET_DGRID(dgram_hd, 0);
	    iov[0].iov_base  = afack;
	    iov[0].iov_len   = MAD_UDP_ACKDG_HD_SIZE;
	    iov[1].iov_base  = (caddr_t)(ptr + buffer->bytes_read);
	    iov[1].iov_len   = tbx_min(MAD_UDP_ACKDG_BODY_SIZE,
				   buffer->length - buffer->bytes_read);
#ifdef MARCEL
	    lg_snt = mad_udp_nb_sendmsg(&poll_arg, iov, 2);
#ifdef MAD_UDP_STATS
	    CHECK(lg_snt == (iov[0].iov_len + iov[1].iov_len),
		  "First packet not fully RE-sent !");
	  } else {
	    TBX_LOCK_SHARED(channel_specific);
	    channel_specific->strange_acks++;
	    TBX_UNLOCK_SHARED(channel_specific);
#endif // MAD_UDP_STATS

#else  // MARCEL
	    lg_snt = mad_udp_sendmsg(so_snd, iov, 2, &remote_address);
	    CHECK(lg_snt == (iov[0].iov_len + iov[1].iov_len),
		  "First packet not fully RE-sent !");
#endif // MARCEL
	  }
	}
#if defined(MAD_UDP_STATS) && defined(MARCEL)
      } else {
	TBX_LOCK_SHARED(channel_specific);
	channel_specific->strange_rrms++;
	TBX_UNLOCK_SHARED(channel_specific);
#endif // MAD_UDP_STATS && MARCEL
      }

      /* Reemit specified packets
	 (in to_send, == ack_rrm+MAD_UDP_RRM_HD_SIZE).
         NB: data sent with ACK for ACK have been received
	     (else, no RRM could have been returned). */

      if (reemit) {
	int i, reemitted_packet = 0;

	MAD_UDP_DGRAM_SET_MSGID(dgram_hd, msgid);
	MAD_UDP_DGRAM_SET_BUFID(dgram_hd, bufid);
	MAD_UDP_MARK_AS_REEMITTED(dgram_hd);
	iov[0].iov_base = dgram_hd;
	iov[0].iov_len  = MAD_UDP_DGRAM_HD_SIZE;
	bytes_read = buffer->bytes_read;
	
	// "Sream control"
	if (reemission_nb) {
	  connection_specific->pause_min_size = (connection_specific->pause_min_size)
	    ?  tbx_min(connection_specific->pause_min_size, buffer->length)
	    :  buffer->length;
	  for (i = 0; i < nb_packet; i++) {
	    if (to_send[i])
	      reemitted_packet++;
	    if ((reemitted_packet == (nb_packet >> 4))
		&& (connection_specific->pause < 1000)) {
	      if (connection_specific->pause / 100)
		connection_specific->pause += 100;
	      else if (connection_specific->pause / 10)
		connection_specific->pause += 10;
	      else
		connection_specific->pause++;
	    }
	  }
	  reemitted_packet = 0;
	}

	// Loop for undoubtedly full packets
	for (i = 0; i < nb_packet - 1; i++) {
	  if (to_send[i]) {
	    reemitted_packet++;
	    MAD_UDP_DGRAM_SET_DGRID(dgram_hd, i);
	    iov[1].iov_base = (caddr_t)(ptr + bytes_read);
	    iov[1].iov_len  = MAD_UDP_DGRAM_BODY_SIZE;
#ifdef MARCEL
	    mad_udp_nb_sendmsg(&poll_arg, iov, 2);
#else  // MARCEL
	    mad_udp_sendmsg(so_snd, iov, 2, &remote_address);
#endif // MARCEL
	    if (reemission_nb)
	      mad_udp_pause(connection_specific->pause);
	  }
	  bytes_read += MAD_UDP_DGRAM_BODY_SIZE;
	}

	// Send last packet (possibly full)
	if (to_send[nb_packet - 1]) {
	  reemitted_packet++;
	  MAD_UDP_DGRAM_SET_DGRID(dgram_hd, nb_packet - 1);
	  iov[1].iov_base = (caddr_t)(ptr + bytes_read);
	  iov[1].iov_len  = last_packet_size;
#ifdef MARCEL
	  mad_udp_nb_sendmsg(&poll_arg, iov, 2);
#else  // MARCEL
	  mad_udp_sendmsg(so_snd, iov, 2, &remote_address);
#endif // MARCEL
	}

	bytes_read += last_packet_size;

#ifdef MAD_UDP_STATS
	TBX_LOCK_SHARED_IF_MARCEL(channel_specific);
	channel_specific->reemitted_dgrams += reemitted_packet;
	TBX_UNLOCK_SHARED_IF_MARCEL(channel_specific);
#endif // MAD_UDP_STATS

	reemission_nb++;
      }
    } while (tbx_true);

    buffer->bytes_read += bytes_read;
#ifndef MARCEL
    TBX_FREE(ack_rrm);
#endif // MARCEL

  } else if (bufid) {
    int lg_snt;

    /* No more data to send but a previous buffer ACK to ack. */
    MAD_UDP_ACK_SET_MSGID(afack, msgid);
    MAD_UDP_ACK_SET_BUFID(afack, bufid - 1);
#ifdef MARCEL
    lg_snt = mad_udp_nb_sendto(&poll_arg, afack, MAD_UDP_ACK_SIZE);
#else  // MARCEL
    lg_snt = mad_udp_sendto(so_snd, afack, MAD_UDP_ACK_SIZE,
			    &remote_address);
#endif // MARCEL
    CHECK(lg_snt == MAD_UDP_ACK_SIZE, "First packet not fully sent !");
  }

  connection_specific->snd_bufid++;
  LOG_OUT();
}


/**
 * Receive buffer
 */

#ifdef VTHD_TEST_ICLUSTER
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_BIG_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_BIG_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_BIG_ACKDG_DGR_SIZE
static tbx_bool_t print_recv_values = tbx_true;
#endif //VTHD_TEST_ICLUSTER

#ifdef VTHD_TEST_SCI
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_SMALL_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_SMALL_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_SMALL_ACKDG_DGR_SIZE
static tbx_bool_t print_recv_values = tbx_true;
#endif // VTHD_TEST_SCI

void
mad_udp_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *p_buffer)
{
  p_mad_connection_t              connection;
  p_mad_udp_connection_specific_t connection_specific;
  p_mad_udp_channel_specific_t    channel_specific;
  p_mad_buffer_t                  buffer;
  p_mad_udp_socket_t              so_rcv;
  mad_udp_address_t               remote_address;
  mad_udp_port_t                  rm_snd_port;
  mad_udp_in_addr_t               rm_snd_in_addr;
  char                           *rrm_buf;
  mad_udp_id_t                    bufid, msgid;
  mad_udp_iovec_t                 iov[2];
#ifdef MARCEL
  mad_udp_marcel_poll_arg_t       poll_arg;
#endif // MARCEL


  LOG_IN();

#if (defined(VTHD_TEST_ICLUSTER) || defined(VTHD_TEST_SCI))
  if (print_recv_values) {
    print_recv_values = tbx_false;
#ifdef VTHD_TEST_ICLUSTER
    DISP_VAL("ICLUSTER recv: MAD_UDP_DGRAM_BODY_SIZE", MAD_UDP_DGRAM_BODY_SIZE);
    DISP_VAL("ICLUSTER recv: MAD_UDP_ACKDG_BODY_SIZE", MAD_UDP_ACKDG_BODY_SIZE);
    DISP_VAL("ICLUSTER recv: MAD_UDP_ACKDG_DGR_SIZE",  MAD_UDP_ACKDG_DGR_SIZE);
#endif // VTHD_TEST_ICLUSTER
#ifdef VTHD_TEST_SCI
    DISP_VAL("SCI recv: MAD_UDP_DGRAM_BODY_SIZE", MAD_UDP_DGRAM_BODY_SIZE);
    DISP_VAL("SCI recv: MAD_UDP_ACKDG_BODY_SIZE", MAD_UDP_ACKDG_BODY_SIZE);
    DISP_VAL("SCI recv: MAD_UDP_ACKDG_DGR_SIZE",  MAD_UDP_ACKDG_DGR_SIZE);
#endif // VTHD_TEST_SCI
  }
#endif // VTHD_TEST_ICLUSTER || VTHD_TEST_SCI

  connection          = lnk->connection;
  connection_specific = connection->specific;
  channel_specific    = connection->channel->specific;
  buffer              = *p_buffer;
  
  so_rcv              = &(channel_specific->socket[RCV_DATA]);
  rm_snd_port         = connection_specific->rm_port[SND_DATA];
  rm_snd_in_addr      = connection_specific->rm_in_addr;
  bufid               = connection_specific->rcv_bufid;  
  msgid               = connection_specific->rcv_msgid;  

#ifdef MARCEL
  poll_arg.poll_on_socket[REQUEST]  = tbx_false;
  poll_arg.poll_on_socket[SND_DATA] = tbx_false;
  poll_arg.poll_on_socket[RCV_DATA] = tbx_true;
  poll_arg.channel                  = connection->channel;
  poll_arg.connection               = connection;
  poll_arg.p_rm_addr                = &remote_address;
#ifdef PM2DEBUG
  sprintf(poll_arg.fname, "recv_buffer");
#endif
#endif // MARCEL


  /*
   * Copy the beginning of first buffer of message in BUFFER,
   * which is stored in channel_specific->request_buffer.
   */

  if (!bufid) {
    int bytes_written;

    bytes_written = tbx_min(buffer->length,
			(MAD_UDP_REQ_SIZE - MAD_UDP_REQ_HD_SIZE));
    memcpy(buffer->buffer,
	   channel_specific->request_buffer + MAD_UDP_REQ_HD_SIZE,
	   bytes_written);
    buffer->bytes_written += bytes_written;
  }

  // Mark remote_address port so that we can send requests
  // for reemission or send acks even if no packet has arrived.
  remote_address.sin_port = htons(0);
  memset(remote_address.sin_zero, '\0', 8);

  if (mad_buffer_full(buffer)) {
    rrm_buf = TBX_MALLOC(MAD_UDP_ACK_SIZE);
  } else {
    /*
     * Receive remaining data:
     *  BUFFER is "split" into MAD_UDP_DGRAM_BODY_SIZE-length slices, and
     *  a boolean array registers, for each of these parts,
     *  if it is waited or not.
     *  The packet IDs make it possible to fill in right BUFFER slices whereas
     *  a packet has arrived.
     *  Care must be taken on packets validity (to eliminate doubled and packets
     *  destined to other communication.
     * NB: for buffers which are not the first of message, the first slice
     *     has already been sent with ACK for ACK of previous buffer.
     */
  
    int   bytes_written;
    void *ptr;
    int   last_packet, last_packet_size;
    char  dgram_hd[MAD_UDP_DGRAM_HD_SIZE];
    char *waited;
    int   first_waited;

    bytes_written = buffer->bytes_written;
    ptr           = (void *)buffer->buffer;

    if (bufid) {
      int lg_to_read;

      CHECK(MAD_UDP_IS_DGRAM_HD(connection_specific->rcv_early_dgr),
	    "ACK for ACK data not saved !!!");
#ifdef PM2DEBUG
      {
	mad_udp_id_t rcv_bufid, rcv_dgrid, rcv_msgid;
	mad_udp_dgr_extract_ids(connection_specific->rcv_early_dgr,
				&rcv_msgid, &rcv_bufid, &rcv_dgrid);
	CHECK(rcv_msgid == msgid && rcv_bufid == bufid && !rcv_dgrid,
	      "Wrong data in connection_specific->rcv_early_dgr");
      }
#endif // PM2DEBUG

      // Read data in connection_specific->rcv_early_dgr
      lg_to_read = tbx_min(MAD_UDP_ACKDG_BODY_SIZE,
		       buffer->length - bytes_written);
      memcpy((caddr_t)(ptr + bytes_written),
	     connection_specific->rcv_early_dgr + MAD_UDP_DGRAM_HD_SIZE,
	     lg_to_read);
      bytes_written += lg_to_read;
    }

    // !!! warning: last_packet_size is first used to store lg to send.
    last_packet_size = buffer->length - bytes_written;

    if (last_packet_size) {
      last_packet      = last_packet_size / MAD_UDP_DGRAM_BODY_SIZE;
      last_packet_size = last_packet_size % MAD_UDP_DGRAM_BODY_SIZE;
      if (last_packet_size == 0)
	last_packet--;
      iov[0].iov_base = dgram_hd;
      iov[0].iov_len  = MAD_UDP_DGRAM_HD_SIZE;
      first_waited = 0;
    } else {
      last_packet  = 0;
      first_waited = 1; // To avoid the loop
    }

    rrm_buf = TBX_MALLOC(tbx_max(1 + last_packet + MAD_UDP_RRM_HD_SIZE,
			     MAD_UDP_ACK_SIZE));
    waited  = rrm_buf + MAD_UDP_RRM_HD_SIZE;
    memset(waited, 0xFF, last_packet + 1);

    while (first_waited <= last_packet) {
      int          lg_rcv, result;
      mad_udp_id_t rcv_bufid, rcv_dgrid, rcv_msgid;

#ifdef MARCEL
      poll_arg.timeout_date = marcel_clock() + 50;
#ifdef PM2DEBUG
      poll_arg.timeout_date_bak = poll_arg.timeout_date;
#endif
      result = mad_udp_nb_select_r(&poll_arg);
#else  // MARCEL
      {
	p_mad_udp_socket_t tmp_so_rcv = so_rcv;
	result = mad_udp_select_r(&tmp_so_rcv, 1, 50);
      }
#endif // MARCEL
      if (result) {
	if (result < 0)
	  continue;
	iov[1].iov_base = (caddr_t)(ptr + bytes_written);
	// If last_packet, receive only last_packet_size bytes.
	iov[1].iov_len  = (first_waited == last_packet)
	  ? last_packet_size : MAD_UDP_DGRAM_BODY_SIZE;
	lg_rcv = mad_udp_recvmsg(so_rcv, iov, 2, &remote_address);
#ifdef MARCEL
	so_rcv->read_free = tbx_true;
#endif // MARCEL
	if (MAD_UDP_IS_DGRAM_HD(dgram_hd)) {
	  mad_udp_dgr_extract_ids(dgram_hd,
				  &rcv_msgid, &rcv_bufid, &rcv_dgrid);
	} else {
#ifdef MARCEL
	  TBX_LOCK_SHARED_IF_MARCEL(channel_specific);
	  channel_specific->wrong_dgrams++;
	  TBX_UNLOCK_SHARED_IF_MARCEL(channel_specific);
#endif // MARCEL
	  remote_address.sin_port = htons(0);
	  continue;
	}

	// Ignore data of other messages.
	if ((rcv_msgid != msgid || rcv_bufid != bufid)
	    || (remote_address.sin_port != rm_snd_port)
	    || (remote_address.sin_addr.s_addr != rm_snd_in_addr.s_addr)) {
	  remote_address.sin_port = htons(0);
#if defined(MAD_UDP_STATS) && defined(MARCEL)
	  TBX_LOCK_SHARED(channel_specific);
	  if ((remote_address.sin_port != rm_snd_port)
	      || (remote_address.sin_addr.s_addr != rm_snd_in_addr.s_addr)) {
	    channel_specific->wrong_dgrams++;
	  } else if (rcv_msgid < msgid || rcv_bufid < bufid) {
	    channel_specific->old_dgrams++;
	  } else {
	    channel_specific->strange_dgrams++;
	  }
	  TBX_UNLOCK_SHARED(channel_specific);
#endif // MAD_UDP_STATS && MARCEL
	  continue;
	}

	if (rcv_dgrid == first_waited) {
	  waited[first_waited] = 0x00;
	  // Go forward until next waited packet
	  while ((first_waited < last_packet) && (!waited[first_waited])) {
	    first_waited++;
	    bytes_written += MAD_UDP_DGRAM_BODY_SIZE;
	  }
	  if ((first_waited == last_packet) && (!waited[last_packet])) {
	    first_waited++;
	    bytes_written += last_packet_size;
	  }
	} else if (waited[rcv_dgrid]) {
	  // rcv_dgrid > first_waited >= 0
	  int offset =
	    MAD_UDP_ACKDG_BODY_SIZE + (rcv_dgrid - 1) * MAD_UDP_DGRAM_BODY_SIZE;

	  waited[rcv_dgrid] = 0x00;
	  memcpy(ptr + offset, ptr + bytes_written,
		 tbx_min(MAD_UDP_DGRAM_BODY_SIZE, buffer->length - offset));
#if defined(MAD_UDP_STATS) && defined(MARCEL)
	} else { // Ignore doubled packets.
	  TBX_LOCK_SHARED_IF_MARCEL(channel_specific);
	  channel_specific->old_dgrams++;
	  TBX_UNLOCK_SHARED_IF_MARCEL(channel_specific);
#endif // MAD_UDP_STATS && MARCEL
	}

      } else { // (result)

	if (remote_address.sin_port == htons(0)) {
	  // No packet has arrived to set remote_address.
	  remote_address.sin_family = AF_INET;
	  remote_address.sin_port   = rm_snd_port;
	  remote_address.sin_addr   = rm_snd_in_addr;
	}
	MAD_UDP_RRM_SET_MSGID(rrm_buf, msgid);
	MAD_UDP_RRM_SET_BUFID(rrm_buf, bufid);
#ifdef MARCEL
	DISP_VAL("Recv_buf: send RRM",*((long *)rrm_buf));
	mad_udp_nb_sendto(&poll_arg, rrm_buf,
			  MAD_UDP_RRM_HD_SIZE + last_packet + 1);
#else  // MARCEL
	mad_udp_sendto(so_rcv, rrm_buf, MAD_UDP_RRM_HD_SIZE + last_packet + 1,
		       &remote_address);
#endif // MARCEL
#ifdef MAD_UDP_STATS
	TBX_LOCK_SHARED_IF_MARCEL(channel_specific);
	channel_specific->rrm_nb++;
	TBX_UNLOCK_SHARED_IF_MARCEL(channel_specific);
#endif // MAD_UDP_STATS
      }
    }

    buffer->bytes_written += bytes_written;

    /* All data received: send ACK. */
    if (remote_address.sin_port == htons(0)) {
      // No packet has arrived to set remote_address.
      remote_address.sin_family = AF_INET;
      remote_address.sin_port   = rm_snd_port;
      remote_address.sin_addr   = rm_snd_in_addr;
    }
    MAD_UDP_ACK_SET_MSGID(rrm_buf, msgid);
    MAD_UDP_ACK_SET_BUFID(rrm_buf, bufid);
#ifdef MARCEL
    mad_udp_nb_sendto(&poll_arg, rrm_buf, MAD_UDP_ACK_SIZE);
#else  // MARCEL
    mad_udp_sendto(so_rcv, rrm_buf, MAD_UDP_ACK_SIZE, &remote_address);
#endif // MARCEL
  }

  /*
   * Keep on reemitting ACK while its ACK does not arrive
   */

  // re-use rrm_buf
  iov[0].iov_base = rrm_buf;
  iov[0].iov_len  = MAD_UDP_ACK_SIZE;
  iov[1].iov_base = connection_specific->rcv_early_dgr;
  iov[1].iov_len  = MAD_UDP_ACKDG_DGR_SIZE;

  do {
    int          result;
    mad_udp_id_t rcv_bufid, rcv_msgid;

    /* Wait for ack for ack ... */
#ifdef MARCEL
    poll_arg.timeout_date = marcel_clock() + 100;
#ifdef PM2DEBUG
    poll_arg.timeout_date_bak = poll_arg.timeout_date;
#endif
    result = mad_udp_nb_select_r(&poll_arg);
#else  // MARCEL
    {
      p_mad_udp_socket_t tmp_so_rcv = so_rcv;
      result = mad_udp_select_r(&tmp_so_rcv, 1, 100);
    }
#endif // MARCEL
    if (result) {
      if (result < 0)
	continue;
      mad_udp_recvmsg(so_rcv, iov, 2, &remote_address);
#ifdef MARCEL
      so_rcv->read_free = tbx_true;
#endif // MARCEL
      mad_udp_ack_rrm_extract_ids(rrm_buf, &rcv_msgid, &rcv_bufid);
      if (MAD_UDP_IS_ACK(rrm_buf)
	  && (rcv_msgid == msgid && rcv_bufid == bufid)
	  && (remote_address.sin_port == rm_snd_port)
	  && (remote_address.sin_addr.s_addr == rm_snd_in_addr.s_addr))
	break;
      else { // Ignore packet but reset rcv_early_dgr
        remote_address.sin_port = htons(0);
	memset(connection_specific->rcv_early_dgr, 0, MAD_UDP_DGRAM_HD_SIZE);
#if defined(MAD_UDP_STATS) && defined(MARCEL)
	TBX_LOCK_SHARED(channel_specific);
	if (!MAD_UDP_IS_ACK(rrm_buf)) {
	  if (MAD_UDP_IS_DGRAM_HD(rrm_buf))
	    channel_specific->strange_dgrams++;
	  else
	    channel_specific->wrong_acks++;
	} else {
	  if (rcv_msgid < msgid)
	    channel_specific->old_acks++;
	  else
	    channel_specific->strange_acks++;
	}
	TBX_UNLOCK_SHARED(channel_specific);
#endif // MAD_UDP_STATS && MARCEL
      }

    }

    /* Timeout, or wrong ACK for ACK: resend ACK */
    if (remote_address.sin_port == htons(0)) {
      // No packet has arrived to set remote_address.
      remote_address.sin_family = AF_INET;
      remote_address.sin_port   = rm_snd_port;
      remote_address.sin_addr   = rm_snd_in_addr;
      memset(remote_address.sin_zero, '\0', 8);
    }
    MAD_UDP_ACK_SET_MSGID(rrm_buf, msgid);
    MAD_UDP_ACK_SET_BUFID(rrm_buf, bufid);
    MAD_UDP_MARK_AS_REEMITTED(rrm_buf);
#ifdef MARCEL
    mad_udp_nb_sendto(&poll_arg, rrm_buf, MAD_UDP_ACK_SIZE);
#else  // MARCEL
    mad_udp_sendto(so_rcv, rrm_buf, MAD_UDP_ACK_SIZE, &remote_address);
#endif // MARCEL
#ifdef MAD_UDP_STATS
    TBX_LOCK_SHARED_IF_MARCEL(channel_specific);
    channel_specific->reemitted_acks++;
    TBX_UNLOCK_SHARED_IF_MARCEL(channel_specific);
#endif // MAD_UDP_STATS

  } while (tbx_true);

  connection_specific->rcv_bufid++;
  TBX_FREE(rrm_buf);
  LOG_OUT();
}


/**
 * Send / Receive buffer group
 */


#ifdef VTHD_TEST_ICLUSTER
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_SMALL_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_SMALL_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_SMALL_ACKDG_DGR_SIZE
#endif // VTHD_TEST_ICLUSTER

#ifdef VTHD_TEST_SCI
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_BIG_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_BIG_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_BIG_ACKDG_DGR_SIZE
#endif // VTHD_TEST_SCI

void
mad_udp_send_buffer_group(p_mad_link_t         lnk TBX_UNUSED,
			  p_mad_buffer_group_t buffer_group TBX_UNUSED)
{
  /*p_mad_connection_t              connection;
    p_mad_udp_connection_specific_t connection_specific;
    p_mad_udp_channel_specific_t    channel_specific;
    mad_udp_address_t               remote_address;*/

  LOG_IN();

#if 0
  //specific            = lnk->specific;
  connection          = lnk->connection;
  connection_specific = connection->specific;
  channel_specific    = connection->channel->specific;

  remote_address.sin_family = AF_INET;
  remote_address.sin_port   = htons(connection_specific->remote_data_port);
  remote_address.sin_addr   = connection_specific->remote_in_addr;
  memset(remote_address.sin_zero, '\0', 8);
#endif // 0

  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_udp_send_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while (tbx_forward_list_reference(&ref));
    }

  LOG_OUT();
}

#ifdef VTHD_TEST_ICLUSTER
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_BIG_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_BIG_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_BIG_ACKDG_DGR_SIZE
#endif //VTHD_TEST_ICLUSTER

#ifdef VTHD_TEST_SCI
#undef  MAD_UDP_DGRAM_BODY_SIZE
#define MAD_UDP_DGRAM_BODY_SIZE MAD_UDP_SMALL_DGRAM_BODY_SIZE
#undef  MAD_UDP_ACKDG_BODY_SIZE
#define MAD_UDP_ACKDG_BODY_SIZE MAD_UDP_SMALL_ACKDG_BODY_SIZE
#undef  MAD_UDP_ACKDG_DGR_SIZE
#define MAD_UDP_ACKDG_DGR_SIZE  MAD_UDP_SMALL_ACKDG_DGR_SIZE
#endif // VTHD_TEST_SCI

void
mad_udp_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_sub_group
				     __attribute__ ((unused)),
				 p_mad_buffer_group_t buffer_group)
{
  /*p_mad_connection_t              connection;
    p_mad_udp_connection_specific_t connection_specific;
    p_mad_udp_channel_specific_t    channel_specific;
    mad_udp_address_t              remote_address;*/

  LOG_IN();
#if 0
  //specific            = lnk->specific;
  connection = lnk->connection;
  connection_specific = connection->specific;
  channel_specific = connection->channel->specific;
#endif //0
  if (!tbx_empty_list(&(buffer_group->buffer_list)))
    {
      tbx_list_reference_t ref;

      tbx_list_reference_init(&ref, &(buffer_group->buffer_list));
      do
	{
	  mad_udp_receive_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while (tbx_forward_list_reference(&ref));
    }

  LOG_OUT();
}
