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

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Macros and constants
 */

#ifdef MARCEL
#error MARCEL support not implemented for UDP driver
#endif // MARCEL

DEBUG_DECLARE(udp)

#undef  DEBUG_NAME
#define DEBUG_NAME udp

#ifdef DEBUG
#define MAD_UDP_STATS     1
#else  // DEBUG
#define MAD_UDP_STATS     0
#endif // DEBUG

#define MAD_UDP_USE_IOVEC 0

#undef  max
#undef  min
#define max(a,b)    ((a) > (b)) ? (a) : (b)
#define min(a,b)    ((a) < (b)) ? (a) : (b)
#define max3(a,b,c) max((a), max((b),(c)))
#define min3(a,b,c) min((a), min((b),(c)))


/* Each node must tell the others what are its request and data ports. This
   two ports are dumped in a string, separated by MAD_UDP_PORT_SEPARATOR. */
#define MAD_UDP_PORT_SEPARATOR '-'
/* Length UDP ports decimal representation */
#define MAD_UDP_PORT_MAX_LENGTH 6
#define MAD_UDP_PORTS_BUF_MAX_LENGTH ((MAD_UDP_PORT_MAX_LENGTH<<2) + 4)

/* Definition of various packet types formats */

#define MAD_UDP_SET_MSGID(buf,fmt,msgid) \
        sprintf((buf), (fmt), (msgid))
#define MAD_UDP_MSGID(buf,beg_size) \
        atoi((buf)+(beg_size))

#define MAD_UDP_ACK_SIZE                         16
#define MAD_UDP_ACK_FMT                  "ACK-%-11d"
#define MAD_UDP_ACK_SET_MSGID(ack,msgid) \
        MAD_UDP_SET_MSGID((ack),MAD_UDP_ACK_FMT,(msgid))
#define MAD_UDP_ACK_MSGID(ack) \
        MAD_UDP_MSGID((ack),4)
#define MAD_UDP_IS_ACK(ack) \
        (!strncmp((ack), MAD_UDP_ACK_FMT, 4))

#define MAD_UDP_RRM_HD_SIZE                      16
#define MAD_UDP_RRM_HD_FMT               "RRM-%-11d"
#define MAD_UDP_RRM_SET_MSGID(rrm,msgid) \
        MAD_UDP_SET_MSGID((rrm),MAD_UDP_RRM_HD_FMT,(msgid))
#define MAD_UDP_RRM_MSGID(rrm) \
        MAD_UDP_MSGID((rrm),4)
#define MAD_UDP_IS_RRM(rrm) \
        (!strncmp((rrm), MAD_UDP_RRM_HD_FMT, 4))

#define MAD_UDP_REQ_SIZE                         64
#define MAD_UDP_REQ_HD_SIZE                      16
#define MAD_UDP_REQ_HD_FMT               "REQ-%-11d"
#define MAD_UDP_REQ_MAXNO               10000000000 // 10 G
#define MAD_UDP_REQ_SET_MSGID(req,msgid) \
        MAD_UDP_SET_MSGID((req),MAD_UDP_REQ_HD_FMT,(msgid))
#define MAD_UDP_REQ_MSGID(req) \
        MAD_UDP_MSGID((req),4)
#define MAD_UDP_IS_REQ(req) \
        (!strncmp((req), MAD_UDP_REQ_HD_FMT, 4))

#define MAD_UDP_DGRAM_HD_SIZE                    32
#define MAD_UDP_DGRAM_HD_FMT          "DGR-%-11d:%-15d"
#define MAD_UDP_DGRAM_BODY_SIZE \
        (NTBX_UDP_MAX_DGRAM_SIZE - MAD_UDP_DGRAM_HD_SIZE)
#define MAD_UDP_DGRAM_SET_MSGID(dgr,msgid) \
        sprintf((dgr), "DGR-%-11d:", (msgid))
#define MAD_UDP_DGRAM_SET_ID(dgr,id) \
        sprintf((dgr)+16, "%-15d", (id))
#define MAD_UDP_IS_DGRAM_HD(dgr) \
        (!strncmp((dgr), MAD_UDP_DGRAM_HD_FMT, 4))


static
void
mad_udp_dgram_extract_ids(char *dgram_header, int *msgid, int *no)
{
  char *pos = dgram_header + 15;
  
  *pos   = '\0';
  *msgid = atoi(dgram_header + 4);
  *no    = atoi(pos + 1);
  *pos   = ':';
}

/*
 * Local structures
 * ----------------
 */

/* Each adapter has got a socket to transmit infos
   on local request and data ports */
typedef struct s_mad_udp_adapter_specific
{
  //ntbx_tcp_port_t   tcp_port;
  ntbx_tcp_socket_t tcp_socket;
} mad_udp_adapter_specific_t, *p_mad_udp_adapter_specific_t;

/* Each node create four sockets:
 *  one to send and receive request, and send their ack;
 *  one to send data and receive their ack
 *    or request for reemission (needs a big send buffer);
 *  one to receive data and send ack for ack,
 *    or request for reemission (needs a big receive buffer).
 */
enum {
  REQUEST,
  SND_DATA,
  RCV_DATA,
  NB_SOCKET
};

/* The four socket are created at channel level */
typedef struct s_mad_udp_channel_specific
{
  ntbx_udp_port_t   port[NB_SOCKET];
  ntbx_udp_socket_t socket[NB_SOCKET];
#if MAD_UDP_USE_IOVEC
  char              request_buffer[MAD_UDP_REQ_SIZE - MAD_UDP_REQ_HD_SIZE];
#else  // MAD_UDP_USE_IOVEC
  char              request_buffer[MAD_UDP_REQ_SIZE];
#endif // MAD_UDP_USE_IOVEC
#if MAD_UDP_STATS
  int               nb_snd_msg;
  int               nb_rcv_msg;
  int               reemitted_requests;
  int               reemitted_dgrams;
  int               reemitted_acks;
  int               reemitted_afacks;
  int               old_acks;
  int               wrong_acks;
  int               strange_acks;
  int               old_dgrams;
  int               wrong_dgrams;
  int               strange_dgrams;
  int               old_reqs;
  int               wrong_reqs;
  int               strange_reqs;
  int               rrm_nb;
  int               old_rrms;
  int               wrong_rrms;
  int               strange_rrms;
#endif // MAD_UDP_STATS
} mad_udp_channel_specific_t, *p_mad_udp_channel_specific_t;

/* A connection registers remote (rm) ports for all sockets */
typedef struct s_mad_udp_connection_specific
{
  ntbx_udp_in_addr_t    rm_in_addr;
  ntbx_udp_port_t       rm_port[NB_SOCKET];
  ntbx_udp_request_id_t snd_msgid; // ID of next msg to send
  ntbx_udp_request_id_t rcv_msgid; // ID of next msg to be received
  tbx_bool_t            rcv_first_of_msg;
  tbx_bool_t            snd_first_of_msg;
} mad_udp_connection_specific_t, *p_mad_udp_connection_specific_t;



/*
 * Static functions
 * ----------------
 */

/**
 * Send REQ and wait for its ACK.
 * REQ packet is filled as much as possible with BUFFER data,
 *     so that
 */
static
void
mad_udp_send_request(p_mad_connection_t   connection,
		     p_mad_buffer_t       first_buffer,
		     p_ntbx_udp_address_t p_remote_address)
{
  p_mad_udp_connection_specific_t specific;
  p_mad_udp_channel_specific_t    channel_specific;
  int                             bytes_read;
  ntbx_udp_socket_t               so_req, so_snd;
  ntbx_udp_in_addr_t              rm_in_addr;
  ntbx_udp_port_t                 rm_rcv_port;
#if MAD_UDP_USE_IOVEC
  ntbx_udp_iovec_t                iov[2];
  char                            buf[max3(MAD_UDP_ACK_SIZE,
					   MAD_UDP_RRM_HD_SIZE,
					   MAD_UDP_REQ_HD_SIZE)];
#else  // MAD_UDP_USE_IOVEC
  char                            buf[max3(MAD_UDP_ACK_SIZE,
					   MAD_UDP_RRM_HD_SIZE,
					   MAD_UDP_REQ_SIZE)];
#endif // MAD_UDP_USE_IOVEC
  ntbx_udp_request_id_t           msgid;
  tbx_bool_t                      ack_received;
#if MAD_UDP_STATS
  int                             loop_cnt = -1;
#endif // MAD_UDP_STATS
  
  LOG_IN();
  specific         = connection->specific;
  channel_specific = connection->channel->specific;
  
  so_req           = channel_specific->socket[REQUEST];
  so_snd           = channel_specific->socket[SND_DATA];
  rm_in_addr       = specific->rm_in_addr;
  rm_rcv_port      = specific->rm_port[RCV_DATA];
  bytes_read       = min(first_buffer->bytes_written,
			 MAD_UDP_REQ_SIZE - MAD_UDP_REQ_HD_SIZE);
  
  MAD_UDP_REQ_SET_MSGID(buf, specific->snd_msgid);
#if MAD_UDP_USE_IOVEC
  iov[0].iov_base = (caddr_t)buf;
  iov[0].iov_len  = MAD_UDP_REQ_HD_SIZE;
  iov[1].iov_base = (caddr_t)first_buffer->buffer;
  iov[1].iov_len  = bytes_read;
#endif // MAD_UDP_USE_IOVEC
  
  ack_received = tbx_false;
  
  do {
    
    /* (Re)Send request */
    
#if MAD_UDP_STATS
    loop_cnt++;
#endif // MAD_UDP_STATS
#if MAD_UDP_USE_IOVEC
    ntbx_udp_sendmsg(so_req, (p_ntbx_udp_iovec_t)iov, 2, p_remote_address);
#else  // MAD_UDP_USE_IOVEC
    memcpy(buf + MAD_UDP_REQ_HD_SIZE, first_buffer->buffer, bytes_read);
    ntbx_udp_sendto(so_req, buf, MAD_UDP_REQ_SIZE, p_remote_address);
#endif // MAD_UDP_USE_IOVEC
    
    
    /* Wait for ack. */
    
    while (ntbx_udp_select(so_snd, 100000)) {
      ntbx_udp_recvfrom(so_snd,
			buf, max(MAD_UDP_RRM_HD_SIZE, MAD_UDP_ACK_SIZE),
			p_remote_address);
      /* If first ACK is lost, then timeout on RCV_DATA socket make
	 receiver send a request for reemission.
	 That is why we choose to use an empty RRM for request ACK. */
      if (MAD_UDP_IS_RRM(buf)) {
	msgid = MAD_UDP_RRM_MSGID(buf);
	if (msgid == specific->snd_msgid
	    && (p_remote_address->sin_addr.s_addr == rm_in_addr.s_addr)
	    && (p_remote_address->sin_port == rm_rcv_port)) {
	  ack_received = tbx_true;
	  break;
#if MAD_UDP_STATS
	} else { // Ignore all other RRM's
	  if (   (p_remote_address->sin_addr.s_addr != rm_in_addr.s_addr)
	      || (p_remote_address->sin_port != rm_rcv_port)) {
	    channel_specific->strange_rrms++;
	  } else if (msgid < specific->snd_msgid) {
	    channel_specific->old_rrms++;
	  } else {
	    channel_specific->wrong_rrms++;
	  }
#endif // MAD_UDP_STATS
	}
      } else if (MAD_UDP_IS_ACK(buf)) {
	// An ACK of a buffer "End of reception".
	// It means that ACK for ACK was lost => resend it.
	ntbx_udp_sendto(so_snd, buf, MAD_UDP_ACK_SIZE, p_remote_address);
#if MAD_UDP_STATS
	channel_specific->reemitted_afacks++;
      } else {
	channel_specific->wrong_rrms++;
	DISP_STR("warning: received sth else than",
		 "ACK or RRM on SND_DATA socket.");
#endif // MAD_UDP_STATS
      }
    }
    
  } while (!ack_received);
  
#ifdef DEBUG
  if (msgid > specific->snd_msgid)
    DISP("Request ACK or RRM msgid too high !");
#endif // DEBUG
#if MAD_UDP_STATS
  channel_specific->reemitted_requests += loop_cnt;
#endif // MAD_UDP_STATS
  
  first_buffer->bytes_read += bytes_read;

  LOG_OUT();
}

/**
 * Receive REQ and send its ACK
 */
static
p_mad_connection_t
mad_udp_recv_request(p_mad_channel_t channel)
{
  p_mad_udp_channel_specific_t    specific;
  ntbx_udp_address_t              remote_address;
  ntbx_udp_request_id_t           msgid = 0;
  
  p_mad_connection_t              in;
  p_mad_udp_connection_specific_t in_specific = NULL;
  tbx_darray_index_t              idx;
  
  ntbx_udp_socket_t               so_req;
#if MAD_UDP_USE_IOVEC
  ntbx_udp_iovec_t                iov[2];
#endif // MAD_UDP_USE_IOVEC
  char                            buf[max(MAD_UDP_RRM_HD_SIZE,
					  MAD_UDP_REQ_HD_SIZE)];
  
  LOG_IN();
  specific = channel->specific;
  

  /* Receive first request from unknown connection */
  
  so_req = specific->socket[REQUEST];
#if MAD_UDP_USE_IOVEC
  iov[0].iov_base = buf;
  iov[0].iov_len  = MAD_UDP_REQ_HD_SIZE;
  iov[1].iov_base = (caddr_t)specific->request_buffer;
  iov[1].iov_len  = MAD_UDP_REQ_SIZE - MAD_UDP_REQ_HD_SIZE;
#endif // MAD_UDP_USE_IOVEC  

  do {
    ntbx_udp_port_t remote_port;
    unsigned long   remote_in_addr;

#if MAD_UDP_USE_IOVEC
    ntbx_udp_recvmsg(so_req, (p_ntbx_udp_iovec_t)iov, 2, &remote_address);
    if (MAD_UDP_IS_REQ(buf))
      msgid = MAD_UDP_REQ_MSGID(buf);
    else {
#if MAD_UDP_STATS
      specific->wrong_reqs++;
#endif // MAD_UDP_STATS
      continue;
    }
#else  // MAD_UDP_USE_IOVEC
    ntbx_udp_recvfrom(so_req, specific->request_buffer, MAD_UDP_REQ_SIZE,
		      &remote_address);
    if (MAD_UDP_IS_REQ(specific->request_buffer))
      msgid = MAD_UDP_REQ_MSGID(specific->request_buffer);
    else {
#if MAD_UDP_STATS
      specific->wrong_reqs++;
#endif // MAD_UDP_STATS
      continue;
    }
#endif // MAD_UDP_USE_IOVEC
    
    /* Search for THE connexion matching remote_address */
    remote_port    = remote_address.sin_port;
    remote_in_addr = remote_address.sin_addr.s_addr;
    idx = -1;
    do {
      in = tbx_darray_next_idx(channel->in_connection_darray, &idx);
    } while (   (in)
	     && (in_specific = in->specific)
	     && (   (in_specific->rm_port[REQUEST]  != remote_port)
		 || (in_specific->rm_in_addr.s_addr != remote_in_addr)));
    
    // Ignore doubled requests (msgid < in_specific->rcv_msgid)
    if ((in) && (msgid == in_specific->rcv_msgid))
      break;
#if MAD_UDP_STATS
    if (msgid < in_specific->rcv_msgid)
      specific->old_reqs++;
    if (msgid > in_specific->rcv_msgid)
      specific->strange_reqs++;
#endif // MAD_UDP_STATS
#ifdef DEBUG
    if (msgid > in_specific->rcv_msgid) {
      DISP("REQ msgid too high !");
      break;
    }
#endif // DEBUG
    
  } while (tbx_true);
  
  
  /* Then send ACK on remote SND_DATA port:
     if this ack is lost, timeout on RCV_DATA socket will make receiver
     send a request for reemission (or an ack if message consists of one
     request only), which will be taken as an ack by sender side.
     That is why we already send an empty RRM as request ACK. */
  MAD_UDP_RRM_SET_MSGID(buf, msgid);
  remote_address.sin_port = in_specific->rm_port[SND_DATA];
  ntbx_udp_sendto(specific->socket[RCV_DATA],
		  buf, MAD_UDP_RRM_HD_SIZE, &remote_address);
  
  LOG_OUT();
  return in;
}



/*
 * Registration function
 * ---------------------
 */

void
mad_udp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

#ifdef DEBUG
  DEBUG_INIT(udp);
#endif // DEBUG
  
  LOG_IN();
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
  LOG_OUT();
}


void
mad_udp_driver_init(p_mad_driver_t driver)
{
  LOG_IN();
  TRACE("Initializing UDP driver");
  driver->specific = NULL;
  LOG_OUT();
}


void
mad_udp_adapter_init(p_mad_adapter_t adapter)
{
  p_mad_udp_adapter_specific_t specific         = NULL;
  p_tbx_string_t               parameter_string = NULL;
  ntbx_tcp_address_t           address;
  
  LOG_IN();
  if (strcmp(adapter->dir_adapter->name, "default"))
    FAILURE("unsupported adapter");
  
  specific = TBX_MALLOC(sizeof(mad_udp_adapter_specific_t));
  specific->tcp_socket = -1;
  adapter->specific    = specific;
#ifdef SSIZE_MAX
  adapter->mtu         = min(SSIZE_MAX, MAD_FORWARD_MAX_MTU);
#else // SSIZE_MAX
  adapter->mtu         = MAD_FORWARD_MAX_MTU;
#endif // SSIZE_MAX
  
  /* A TCP socket is created on each node. It will be used only for
     for channel initialisations */
  specific->tcp_socket = ntbx_tcp_socket_create(&address, 0);
  SYSCALL(listen(specific->tcp_socket, min(5, SOMAXCONN)));
  
  /* adapter->parameter is only the TCP port */
  parameter_string   =
    tbx_string_init_to_int(ntohs(address.sin_port));
  adapter->parameter = tbx_string_to_cstring(parameter_string);
  tbx_string_free(parameter_string);
  
  LOG_OUT();
}


void
mad_udp_channel_init(p_mad_channel_t channel)
{
  p_mad_udp_channel_specific_t specific;
  ntbx_udp_address_t           address;
  int                          i;
  
  LOG_IN();
  specific = TBX_MALLOC(sizeof(mad_udp_channel_specific_t));

  /*
   * Init channel sockets
   */
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->port[i]   = (short int) -1;
    specific->socket[i] = -1;
  }
  channel->specific     = specific;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->socket[i] = ntbx_udp_socket_create(&address);
    specific->port[i]   = (ntbx_udp_port_t)address.sin_port;
  }
  
  ntbx_udp_socket_setup(specific->socket[REQUEST],
			NTBX_UDP_SO_BUF_SMALL, NTBX_UDP_SO_BUF_LARGE);
  ntbx_udp_socket_setup(specific->socket[SND_DATA],
			NTBX_UDP_SO_BUF_LARGE, NTBX_UDP_SO_BUF_MID);
  ntbx_udp_socket_setup(specific->socket[RCV_DATA],
			NTBX_UDP_SO_BUF_MID, NTBX_UDP_SO_BUF_LARGE);
#if MAD_UDP_STATS
  specific->nb_snd_msg         = 0;
  specific->nb_rcv_msg         = 0;
  specific->reemitted_requests = 0;
  specific->reemitted_dgrams   = 0;
  specific->reemitted_acks     = 0;
  specific->reemitted_afacks   = 0;
  specific->old_acks           = 0;
  specific->wrong_acks         = 0;
  specific->strange_acks       = 0;
  specific->old_dgrams         = 0;
  specific->wrong_dgrams       = 0;
  specific->strange_dgrams     = 0;
  specific->old_reqs           = 0;
  specific->wrong_reqs         = 0;
  specific->strange_reqs       = 0;
  specific->rrm_nb             = 0;
  specific->old_rrms           = 0;
  specific->wrong_rrms         = 0;
  specific->strange_rrms       = 0;
#endif // MAD_UDP_STATS
  
  LOG_OUT();
}

void
mad_udp_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_udp_connection_specific_t specific = NULL;
  int                             i;
  
  LOG_IN();
  specific = TBX_MALLOC(sizeof(mad_udp_connection_specific_t));
  specific->rm_in_addr.s_addr  =  0;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->rm_port[i]       = htons(0);
  }
  specific->snd_msgid          = 0;
  specific->rcv_msgid          = 0;
  specific->rcv_first_of_msg   = tbx_true;
  specific->snd_first_of_msg   = tbx_true;
  in->specific = out->specific = specific;
  in->nb_link  = 1;
  out->nb_link = 1;
  LOG_OUT();
}


void 
mad_udp_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  lnk->specific    = NULL;
  LOG_OUT();
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

  LOG_IN();
  channel_specific    = in->channel->specific;
  connection_specific = in->specific;
  adapter_specific    = in->channel->adapter->specific;
  
  /*
   * Accept remote ports send
   */

  SYSCALL(tcp_socket = accept(adapter_specific->tcp_socket,
			      (struct sockaddr *)(&remote_address), &lg));
  ntbx_tcp_socket_setup(tcp_socket);
  // ... do not loose time : send before receive ...

  /*
   * Send local req and data ports to distant node
   */
  
  // Prepare ports buffer
  sprintf(ports_buffer, "%d%c%d%c%d",
	  ntohs(channel_specific->port[REQUEST]),  MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->port[SND_DATA]), MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->port[RCV_DATA]));
  
  // Send local ports
  ntbx_tcp_write(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  
  /*
   * Receive req and data ports from distant node
   */
 
  ntbx_tcp_read(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  
  connection_specific->rm_in_addr =
    (ntbx_udp_in_addr_t)remote_address.sin_addr;
  port = ports_buffer;
  for (i = REQUEST; i < RCV_DATA; i++) {
    separator  = strchr(port, MAD_UDP_PORT_SEPARATOR);
    *separator = '\0';
    connection_specific->rm_port[i] = htons(atoi(port));
    port       = 1 + separator;
  }
  connection_specific->rm_port[i] = htons(atoi(port));
  
  SYSCALL(close(tcp_socket));
  LOG_OUT();
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
  
  LOG_IN();
  channel_specific    = out->channel->specific;
  connection_specific = out->specific;
  adapter_specific    = out->channel->adapter->specific;
  
  /*
   * Send local req and data ports to distant node
   */
  
  // Prepare ports buffer
  sprintf(ports_buffer, "%d%c%d%c%d",
	  ntohs(channel_specific->port[REQUEST]),  MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->port[SND_DATA]), MAD_UDP_PORT_SEPARATOR,
	  ntohs(channel_specific->port[RCV_DATA]));
  
  // Connect a TCP client socket
  tcp_socket = ntbx_tcp_socket_create(NULL, 0);
  ntbx_tcp_address_fill_ip(&remote_address,
			   atoi(adapter_info->dir_adapter->parameter),
			   &adapter_info->dir_node->ip);
  ntbx_tcp_socket_setup(tcp_socket);
  SYSCALL(connect(tcp_socket, (struct sockaddr *)&remote_address, 
		  sizeof(ntbx_tcp_address_t)));
  ntbx_tcp_write(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  
  /*
   * Receive req and data ports from distant node
   */
  
  ntbx_tcp_read(tcp_socket, ports_buffer, MAD_UDP_PORTS_BUF_MAX_LENGTH);
  
  connection_specific->rm_in_addr =
    (ntbx_udp_in_addr_t)remote_address.sin_addr;
  port = ports_buffer;
  for (i = REQUEST; i < RCV_DATA; i++) {
    separator  = strchr(port, MAD_UDP_PORT_SEPARATOR);
    *separator = '\0';
    connection_specific->rm_port[i] = htons(atoi(port));
    port       = 1 + separator;
  }
  connection_specific->rm_port[i] = htons(atoi(port));
  
  SYSCALL(close(tcp_socket));
  LOG_OUT();
}


void
mad_udp_disconnect(p_mad_connection_t connection)
{
  p_mad_udp_connection_specific_t specific = NULL;
  int i;
  
  LOG_IN();
  specific = connection->specific;
  specific->rm_in_addr.s_addr = 0;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    specific->rm_port[i] = htons(0);
  }
  specific->rcv_first_of_msg = tbx_true;
  specific->snd_first_of_msg = tbx_true;
  LOG_OUT();
}


void
mad_udp_channel_exit(p_mad_channel_t channel)
{
  p_mad_udp_channel_specific_t specific = NULL;
  int i;
  
  LOG_IN();
  specific = channel->specific;
  for (i = REQUEST; i < NB_SOCKET; i++) {
    SYSCALL(close(specific->socket[i]));
  }
#if MAD_UDP_STATS
  DISP("Channel stats:");
  DISP_VAL("Sent msg                 ", specific->nb_snd_msg);
  DISP_VAL("Received msg             ", specific->nb_rcv_msg);
  DISP_VAL("Reemitted requests       ", specific->reemitted_requests);
  DISP_VAL("Reemitted dgrams         ", specific->reemitted_dgrams);
  DISP_VAL("Reemitted acks           ", specific->reemitted_acks);
  DISP_VAL("Reemitted acks for acks  ", specific->reemitted_afacks);
  DISP_VAL("Old acks                 ", specific->old_acks);
  DISP_VAL("Not awaited acks         ", specific->wrong_acks);
  DISP_VAL("Acks coming too early    ", specific->strange_acks);
  DISP_VAL("Doubled dgrams           ", specific->old_dgrams);
  DISP_VAL("Not awaited dgrams       ", specific->wrong_dgrams);
  DISP_VAL("Dgrams coming too early  ", specific->strange_dgrams);
  DISP_VAL("Doubled requests         ", specific->old_reqs);
  DISP_VAL("Not awaited requests     ", specific->wrong_reqs);
  DISP_VAL("Requests coming too early", specific->strange_reqs);
  DISP_VAL("Requests for reemission  ", specific->rrm_nb);
  DISP_VAL("Doubled req for reem.    ", specific->old_rrms);
  DISP_VAL("Not awaited rrms         ", specific->wrong_rrms);
  DISP_VAL("RRM coming too early     ", specific->strange_rrms);
#endif // MAD_UDP_STATS
  TBX_FREE(specific);
  channel->specific = NULL;
  LOG_OUT();
}


void
mad_udp_adapter_exit(p_mad_adapter_t adapter)
{
  LOG_IN();
  TBX_FREE(adapter->specific);
  adapter->specific = NULL;
  LOG_OUT();
}


void
mad_udp_new_message(p_mad_connection_t out)
{
  p_mad_udp_connection_specific_t specific;
  
  LOG_IN();
  specific = out->specific;
  specific->snd_first_of_msg = tbx_true;
  LOG_OUT();
}

void
mad_udp_finalize_message(p_mad_connection_t out)
{
  p_mad_udp_connection_specific_t specific;
  
  LOG_IN();
  specific = out->specific;
  specific->snd_msgid = (specific->snd_msgid + 1) % MAD_UDP_REQ_MAXNO;
#ifdef DEBUG
  if (!specific->snd_msgid)
    DISP("*** Tour du compteur de req snd ***");
#endif // DEBUG
#if MAD_UDP_STATS
  {
    p_mad_udp_channel_specific_t cs;
    cs = out->channel->specific;
    cs->nb_snd_msg++;
  }
#endif // MAD_UDP_STATS
  LOG_OUT();
}


p_mad_connection_t
mad_udp_receive_message(p_mad_channel_t channel)
{
  p_mad_connection_t              in;
  p_mad_udp_connection_specific_t in_specific;
 
  in = mad_udp_recv_request(channel);
  in_specific = in->specific;
  in_specific->rcv_first_of_msg = tbx_true;
  return in;
}

void
mad_udp_message_received(p_mad_connection_t in)
{
  p_mad_udp_connection_specific_t specific;

  LOG_IN();
  specific = in->specific;
  specific->rcv_msgid = (specific->rcv_msgid + 1) % MAD_UDP_REQ_MAXNO;
#ifdef DEBUG
  if (!specific->rcv_msgid)
    DISP("*** Tour du compteur de req rcv ***");
#endif // DEBUG
#if MAD_UDP_STATS
  {
    p_mad_udp_channel_specific_t cs;
    cs = in->channel->specific;
    cs->nb_rcv_msg++;
  }
#endif // MAD_UDP_STATS
  LOG_OUT();
}


/**
 * Send / Receive buffer
 */


void
mad_udp_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t buffer)
{
  //p_mad_udp_link_specific_t       specific;
  p_mad_connection_t              connection;
  p_mad_udp_connection_specific_t connection_specific;
  p_mad_udp_channel_specific_t    channel_specific;
  ntbx_udp_address_t              remote_address;
  char                           *ack_rrm;
  ntbx_udp_socket_t               so_snd;
  
  LOG_IN();
  //specific            = lnk->specific;
  connection          = lnk->connection;
  connection_specific = connection->specific;
  channel_specific    = connection->channel->specific;
  so_snd              = channel_specific->socket[SND_DATA];
  
  remote_address.sin_family = AF_INET;
  remote_address.sin_addr   = connection_specific->rm_in_addr;
  memset(remote_address.sin_zero, '\0', 8);
  
  /*
   * First, send request (with some data).
   */
  
  if (connection_specific->snd_first_of_msg) {
    remote_address.sin_port = connection_specific->rm_port[REQUEST];
    mad_udp_send_request(connection, buffer, &remote_address);
    connection_specific->snd_first_of_msg = tbx_false;
  }
  
  /*
   * Then send remaining data.
   */
  
  if (mad_more_data(buffer)) {
    
    int                bytes_read, lg_to_snd;
    void              *ptr;
    int                nb_packet, last_packet_size;
    int                msgid;
    char               dgram_hd[MAD_UDP_DGRAM_HD_SIZE];
    ntbx_udp_iovec_t   iov[2];
    ntbx_udp_port_t    rm_rcv_port;
    ntbx_udp_in_addr_t rm_rcv_in_addr;
    char              *to_send;
    int                ack_rrm_len;
    
    bytes_read       = buffer->bytes_read;
    lg_to_snd        = buffer->bytes_written - bytes_read;
    ptr              = (void *)buffer->buffer;
    nb_packet        = 0;
    last_packet_size = 0;
    msgid            = connection_specific->snd_msgid;
    iov[0].iov_base  = dgram_hd;
    iov[0].iov_len   = MAD_UDP_DGRAM_HD_SIZE;
    MAD_UDP_DGRAM_SET_MSGID(dgram_hd, msgid);
    rm_rcv_port      = connection_specific->rm_port[RCV_DATA];
    rm_rcv_in_addr   = connection_specific->rm_in_addr;
    
    /*
     * Send buffer in MAX_DGRAM_SIZE-length packets.
     * Count number of sent packets and set last_packet_size.
     */
    remote_address.sin_port = rm_rcv_port;
    while (lg_to_snd > 0) {
      int lg_snt;
      
      MAD_UDP_DGRAM_SET_ID(dgram_hd, nb_packet);
      last_packet_size = min(lg_to_snd, MAD_UDP_DGRAM_BODY_SIZE);
      iov[1].iov_base  = (caddr_t)(ptr +  bytes_read);
      iov[1].iov_len   = last_packet_size;
      
      lg_snt = ntbx_udp_sendmsg(so_snd, iov, 2, &remote_address);
      nb_packet++;
#ifdef DEBUG
      if (lg_snt != last_packet_size + MAD_UDP_DGRAM_HD_SIZE)
	printf("warning: %de packet not fully sent !\n", nb_packet);
#endif // DEBUG
      bytes_read += last_packet_size;
      lg_to_snd  -= last_packet_size;
    }
    
    /*
     * Keep on reemitting specified packets as long as requests
     * for reemission arrive.
     */
    
    ack_rrm_len = max(MAD_UDP_ACK_SIZE, MAD_UDP_RRM_HD_SIZE + nb_packet);
    ack_rrm     = TBX_MALLOC(ack_rrm_len);
    to_send     = ack_rrm + MAD_UDP_RRM_HD_SIZE;
    memset(to_send, 0xFF, nb_packet);
    
    do {
      tbx_bool_t reemit = tbx_false;
      
      /* Wait for ACK or RRM on SND_DATA socket. */

      ntbx_udp_recvfrom(so_snd, ack_rrm, ack_rrm_len, &remote_address);
      if (MAD_UDP_IS_RRM(ack_rrm)) {
	reemit =
	  ((MAD_UDP_RRM_MSGID(ack_rrm) == msgid)
	   && (remote_address.sin_port == rm_rcv_port)
	   && (remote_address.sin_addr.s_addr == rm_rcv_in_addr.s_addr));
#if MAD_UDP_STATS
	if (!reemit) { // Ignore all other RRM's
	  if ((remote_address.sin_addr.s_addr != rm_rcv_in_addr.s_addr)
	      || (remote_address.sin_port != rm_rcv_port)) {
	    channel_specific->strange_rrms++;
	  } else if (msgid < connection_specific->snd_msgid) {
	    channel_specific->old_rrms++;
	  } else {
	    channel_specific->wrong_rrms++;
	  }
	}
#endif // MAD_UDP_STATS
	
      } else if (MAD_UDP_IS_ACK(ack_rrm)) {
	if (   (MAD_UDP_ACK_MSGID(ack_rrm) == msgid)
	       && (remote_address.sin_port == rm_rcv_port)
	       && (remote_address.sin_addr.s_addr == rm_rcv_in_addr.s_addr)) {
	  /* Buffer ACK received. */
	  break;
	} else {
	  // An old buffer "End of emission" ACK was received.
	  // It means that ACK for ACK was lost => resend it.
	  MAD_UDP_ACK_SET_MSGID(ack_rrm, MAD_UDP_ACK_MSGID(ack_rrm));
	  ntbx_udp_sendto(so_snd,
			  ack_rrm, MAD_UDP_ACK_SIZE, &remote_address);
	}
#if MAD_UDP_STATS
      } else {
	channel_specific->strange_rrms++;
#endif // MAD_UDP_STATS
      }
      
      /* Reemit specified packets
	 (in to_send, == ack_rrm+MAD_UDP_RRM_HD_SIZE). */
      
      if (reemit) {
	int i;
	
	bytes_read = buffer->bytes_read;
	// Loop for full packets
	for (i = 0; i < nb_packet - 1; i++) {
	  if (to_send[i]) {
	    MAD_UDP_DGRAM_SET_ID(dgram_hd, i);
	    iov[1].iov_base = (caddr_t)(ptr + bytes_read);
	    iov[1].iov_len  = MAD_UDP_DGRAM_BODY_SIZE;
	    ntbx_udp_sendmsg(so_snd, iov, 2, &remote_address);
#if MAD_UDP_STATS
	    channel_specific->reemitted_dgrams++;
#endif // DEBUG
	  }
	  bytes_read += MAD_UDP_DGRAM_BODY_SIZE;
	}
	// Send last packet (possibly full)
	if (to_send[nb_packet - 1]) {
	  MAD_UDP_DGRAM_SET_ID(dgram_hd, nb_packet - 1);
	  iov[1].iov_base = (caddr_t)(ptr + bytes_read);
	  iov[1].iov_len  = last_packet_size;
	  ntbx_udp_sendmsg(so_snd, iov, 2, &remote_address);	  
#if MAD_UDP_STATS
	  channel_specific->reemitted_dgrams++;
#endif // MAD_UDP_STATS
	}
	bytes_read += last_packet_size;
      }
    } while (tbx_true);
    
    buffer->bytes_read += bytes_read;
    
    
  } else { // mad_more_data
    ack_rrm = TBX_MALLOC(MAD_UDP_ACK_SIZE);
    MAD_UDP_ACK_SET_MSGID(ack_rrm, connection_specific->snd_msgid);
  }
  
  /**
   * Buffer ACK received : send ACK for ACK.
   */
  ntbx_udp_sendto(so_snd,
		  ack_rrm, MAD_UDP_ACK_SIZE, &remote_address);
  
  TBX_FREE(ack_rrm);
  LOG_OUT();
}


void
mad_udp_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *p_buffer)
{
  //p_mad_udp_link_specific_t       specific;
  p_mad_connection_t              connection;
  p_mad_udp_connection_specific_t connection_specific;
  p_mad_udp_channel_specific_t    channel_specific;
  p_mad_buffer_t                  buffer;
  ntbx_udp_address_t              remote_address;
  ntbx_udp_port_t                 rm_snd_port;
  ntbx_udp_in_addr_t              rm_snd_in_addr;
  ntbx_udp_socket_t               so_rcv;
  char                           *rrm_buf;
  int                             msgid;
  
  LOG_IN();
  //specific            = lnk->specific;
  connection          = lnk->connection;
  connection_specific = connection->specific;
  channel_specific    = connection->channel->specific;
  buffer              = *p_buffer;
  
  so_rcv              = channel_specific->socket[RCV_DATA];
  rm_snd_port         = connection_specific->rm_port[SND_DATA];
  rm_snd_in_addr      = connection_specific->rm_in_addr;
  msgid               = connection_specific->rcv_msgid;  
  
  
  /*
   * Copy the beginning of first buffer of message in BUFFER,
   * which is stored in channel_specific->request_buffer.
   */
  
  if (connection_specific->rcv_first_of_msg) {
    int bytes_written;

    bytes_written = min(buffer->length,
			(MAD_UDP_REQ_SIZE - MAD_UDP_REQ_HD_SIZE));
    memcpy(buffer->buffer,
#if MAD_UDP_USE_IOVEC
	   channel_specific->request_buffer,
#else  // MAD_UDP_USE_IOVEC
	   channel_specific->request_buffer + MAD_UDP_REQ_HD_SIZE,
#endif // MAD_UDP_USE_IOVEC
	   bytes_written);
    buffer->bytes_written += bytes_written;
    connection_specific->rcv_first_of_msg = tbx_false;
  }
  
  
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
     */
  
    int                bytes_written;
    void              *ptr;
    int                last_packet, last_packet_size;
    char               dgram_hd[MAD_UDP_DGRAM_HD_SIZE];
    char              *waited;
    ntbx_udp_iovec_t   iov[2];
    int                first_waited;
    
    // Mark remote_address port so that we can send requests
    // for reemission even if no packet has arrived.
  
    remote_address.sin_port = htons(0);
    
    bytes_written  = buffer->bytes_written;
    ptr            = (void *)buffer->buffer;
    
    // This calculus is optimized with -O6
    // !!! last_packet_size is first used to store lg to send.
    last_packet_size = buffer->length - bytes_written;
    last_packet      = last_packet_size / MAD_UDP_DGRAM_BODY_SIZE;
    last_packet_size = last_packet_size % MAD_UDP_DGRAM_BODY_SIZE;
    if (last_packet_size == 0) last_packet--;
    
    iov[0].iov_base = dgram_hd;
    iov[0].iov_len  = MAD_UDP_DGRAM_HD_SIZE;
    rrm_buf = TBX_MALLOC(max(1 + last_packet + MAD_UDP_RRM_HD_SIZE,
			     MAD_UDP_ACK_SIZE));
    waited  = rrm_buf + MAD_UDP_RRM_HD_SIZE;
    memset(waited, 0xFF, last_packet + 1);
    first_waited = 0;
    
    while (first_waited <= last_packet) {
      int lg_rcv, rcv_no, rcv_msgid;
      
      if (ntbx_udp_select(so_rcv, 500000)) {
	
	iov[1].iov_base = (caddr_t)(ptr + bytes_written);
	// If last_packet, receive only last_packet_size bytes.
	iov[1].iov_len  = (first_waited == last_packet)
	  ? last_packet_size : MAD_UDP_DGRAM_BODY_SIZE;
	lg_rcv = ntbx_udp_recvmsg(so_rcv, iov, 2, &remote_address);
	
	if (MAD_UDP_IS_DGRAM_HD(dgram_hd)) {
	  mad_udp_dgram_extract_ids(dgram_hd, &rcv_msgid, &rcv_no);
	} else {
#if MAD_UDP_SET_MSGID
	  channel_specific->wrong_dgrams++;
#endif // MAD_UDP_SET_MSGID
	  continue;
	}
	
	// Ignore data of other messages.
	if ((rcv_msgid != msgid)
	    || (remote_address.sin_port != rm_snd_port)
	    || (remote_address.sin_addr.s_addr != rm_snd_in_addr.s_addr)) {
	  remote_address.sin_port = htons(0);
#if MAD_UDP_STATS
	  if ((remote_address.sin_port != rm_snd_port)
	      || (remote_address.sin_addr.s_addr != rm_snd_in_addr.s_addr)) {
	    channel_specific->wrong_dgrams++;
	  } else if (rcv_msgid < msgid) {
	    channel_specific->old_dgrams++;
	  } else {
	    channel_specific->strange_dgrams++;
	  }
#endif // MAD_UDP_STATS
	  continue;
	}
	
	if (rcv_no == first_waited) {
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
	} else if (waited[rcv_no]) {
	  int offset    = rcv_no * MAD_UDP_DGRAM_BODY_SIZE;
	  
	  waited[rcv_no] = 0x00;
	  memcpy(ptr + offset, ptr + bytes_written,
		 min(MAD_UDP_DGRAM_BODY_SIZE, buffer->length - offset));
#if MAD_UDP_STATS
	} else { // Ignore doubled packets.
	  channel_specific->old_dgrams++;
#endif // MAD_UDP_STATS
	}
	
      } else { // ntbx_udp_select
	
	if (remote_address.sin_port == htons(0)) {
	  // No packet has arrived to set remote_address.
	  remote_address.sin_family = AF_INET;
	  remote_address.sin_port   = htons(rm_snd_port);
	  remote_address.sin_addr   = connection_specific->rm_in_addr;
	  memset(remote_address.sin_zero, '\0', 8);
	}
	MAD_UDP_RRM_SET_MSGID(rrm_buf, msgid);
	ntbx_udp_sendto(so_rcv,
			rrm_buf, MAD_UDP_RRM_HD_SIZE + last_packet + 1,
			&remote_address);
#if MAD_UDP_STATS
	channel_specific->rrm_nb++;
#endif // MAD_UDP_STATS
      }
    }
    
    /* All data received: send ACK so that homogeneity is kept with
     * the case "whole buffer in request" */
    MAD_UDP_ACK_SET_MSGID(rrm_buf, msgid);
    ntbx_udp_sendto(so_rcv, rrm_buf, MAD_UDP_ACK_SIZE, &remote_address);
      
  }

  /*
   * Keep on reemitting ACK while its ACK does not arrive
   */

  do {
    
    /* Wait for ack for ack ... */
    if (ntbx_udp_select(so_rcv, 500000)) {
      // re-use rrm_buf
      ntbx_udp_recvfrom(so_rcv, rrm_buf, MAD_UDP_ACK_SIZE, &remote_address);
      if (MAD_UDP_IS_ACK(rrm_buf)
	  && (MAD_UDP_ACK_MSGID(rrm_buf) == msgid)
	  && (remote_address.sin_port == rm_snd_port)
	  && (remote_address.sin_addr.s_addr == rm_snd_in_addr.s_addr))
	break;
#if MAD_UDP_STATS
      else { // Ignore packet ...
	if (!MAD_UDP_IS_ACK(rrm_buf)) {
	  if (MAD_UDP_IS_DGRAM_HD(rrm_buf))
	    channel_specific->strange_dgrams++;
	  else
	    channel_specific->wrong_acks++;
	} else {
	  if (MAD_UDP_ACK_MSGID(rrm_buf) < msgid)
	    channel_specific->old_acks++;
	  else
	    channel_specific->strange_acks++;
	}
      }
#endif // MAD_UDP_STATS

    }
    
    /* Timeout, or wrong ACK for ACK: resend ACK */
    MAD_UDP_ACK_SET_MSGID(rrm_buf, msgid);
    ntbx_udp_sendto(so_rcv, rrm_buf, MAD_UDP_ACK_SIZE, &remote_address);
    
  } while (tbx_true);
  
  TBX_FREE(rrm_buf);
  LOG_OUT();
}

/**
 * Send / Receive buffer group
 */


void
mad_udp_send_buffer_group(p_mad_link_t         lnk TBX_UNUSED,
			  p_mad_buffer_group_t buffer_group TBX_UNUSED)
{
  /*p_mad_connection_t              connection;
    p_mad_udp_connection_specific_t connection_specific;
    p_mad_udp_channel_specific_t    channel_specific;
    ntbx_udp_address_t              remote_address;*/

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


void
mad_udp_receive_sub_buffer_group(p_mad_link_t         lnk,
				 tbx_bool_t           first_sub_group
				     __attribute__ ((unused)),
				 p_mad_buffer_group_t buffer_group)
{
  /*p_mad_connection_t              connection;
    p_mad_udp_connection_specific_t connection_specific;
    p_mad_udp_channel_specific_t    channel_specific;
    ntbx_udp_address_t              remote_address;*/

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
