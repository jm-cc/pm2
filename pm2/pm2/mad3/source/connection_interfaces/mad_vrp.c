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
 * Mad_vrp.c
 * =========
 */


#include "madeleine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

#ifndef VRP_MADELEINE
#  define VRP_MADELEINE
#endif
#include <VRP.h>

#define MAD_VRP_LEVEL1_POLLING_MODE \
    (MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD | MARCEL_POLL_AT_IDLE)
#define MAD_VRP_LEVEL2_POLLING_MODE \
    (MARCEL_POLL_AT_TIMER_SIG | MARCEL_POLL_AT_YIELD | MARCEL_POLL_AT_IDLE)
#define MAD_VRP_MAX_FIRST_PACKET_LENGTH 65536

/*
 * local structures
 * ----------------
 */
typedef struct s_mad_vrp_driver_specific
{
  marcel_pollid_t level1_pollid;
  marcel_pollid_t level2_pollid;
} mad_vrp_driver_specific_t, *p_mad_vrp_driver_specific_t;

typedef struct s_mad_vrp_adapter_specific
{
  p_ntbx_server_t server;
} mad_vrp_adapter_specific_t, *p_mad_vrp_adapter_specific_t;

typedef struct s_mad_vrp_channel_specific
{
  int next;
} mad_vrp_channel_specific_t, *p_mad_vrp_channel_specific_t;

typedef struct s_mad_vrp_in_connection_specific
{
  int             socket;
  int             port;
  vrp_incoming_t  vrp_in;
  marcel_t        thread;
  marcel_sem_t    sem_put;
  marcel_sem_t    sem_get;
  p_mad_buffer_t  buffer;
  unsigned char   first_packet_data[MAD_VRP_MAX_FIRST_PACKET_LENGTH];
  size_t          first_packet_length;
  tbx_bool_t      active;

  size_t         *lost;             /**< loss description array (size of array=loss_num) */
  int             loss_num;         /**< number of lost fragments */
  size_t          loss_granularity; /**< granularity of loss, in bytes */

} mad_vrp_in_connection_specific_t, *p_mad_vrp_in_connection_specific_t;

typedef struct s_mad_vrp_out_connection_specific
{
  int            socket;
  int            port;
  vrp_outgoing_t vrp_out;
  tbx_bool_t     first;
  marcel_t       thread;
} mad_vrp_out_connection_specific_t, *p_mad_vrp_out_connection_specific_t;

typedef struct s_mad_vrp_link_specific
{
  int dummy;
} mad_vrp_link_specific_t, *p_mad_vrp_link_specific_t;

typedef struct s_mad_vrp_poll_req
{
  p_mad_channel_t    ch;
  p_mad_connection_t c;
} mad_vrp_poll_req_t, *p_mad_vrp_poll_req_t;

typedef struct s_mad_vrp_select_req
{
  int fd;
  int status;
} mad_vrp_select_req_t, *p_mad_vrp_select_req_t;

/*
 * static functions
 * ----------------
 */

static
void
mad_vrp_frame_handler(vrp_in_buffer_t vrp_b)
{
  p_mad_vrp_in_connection_specific_t is = NULL;
  p_mad_buffer_t                     b  = NULL;

  LOG_IN();
  is = vrp_b->source;
  marcel_sem_P(&(is->sem_put));
  b = is->buffer;

  if (b)
    {
      size_t copy_len = tbx_min(vrp_b->size, b->length - b->bytes_written);

      memcpy(b->buffer + b->bytes_written, vrp_b->data, copy_len);
      b->bytes_written += copy_len;
    }
  else
    {
      is->active = tbx_true;

      if (vrp_b->size <= MAD_VRP_MAX_FIRST_PACKET_LENGTH)
        {
          memcpy(is->first_packet_data, vrp_b->data, vrp_b->size);
          is->first_packet_length = vrp_b->size;
        }
    }

  is->loss_num		= vrp_b->loss_num;
  is->loss_granularity	= vrp_b->loss_granularity;

  if (is->loss_num)
    {
      is->lost = TBX_MALLOC(vrp_b->loss_num * sizeof(size_t));
      {
        int i = 0;

        for (i = 0; i < is->loss_num; i++)
          {
            is->lost[i] = vrp_b->lost[i] - vrp_b->data;
          }
      }

    }
  else
    {
      is->lost = NULL;
    }

  marcel_sem_V(&(is->sem_get));
  LOG_OUT();
}

static
void *
mad_vrp_incoming_thread(void *arg)
{
  p_mad_connection_t                 in     = NULL;
  p_mad_vrp_in_connection_specific_t is     = NULL;
  p_mad_driver_t                     d      = NULL;
  p_mad_vrp_driver_specific_t        ds     = NULL;
  vrp_incoming_t                     vrp_in = NULL;
  mad_vrp_select_req_t               req;

  LOG_IN();
  in     = arg;
  d      = in->channel->adapter->driver;
  ds     = d->specific;
  is     = in->specific;
  vrp_in = is->vrp_in;
  req.fd     = vrp_incoming_fd(vrp_in);

  while (1)
    {
      req.status = 0;
      marcel_poll(ds->level1_pollid, &req);

      if (req.status == -1)
        break;

      vrp_incoming_read_callback(req.fd, vrp_in);
    }

  LOG_OUT();

  return NULL;
}

static
void *
mad_vrp_outgoing_thread(void *arg)
{
  p_mad_connection_t                  out     = NULL;
  p_mad_vrp_out_connection_specific_t os      = NULL;
  p_mad_driver_t                      d       = NULL;
  p_mad_vrp_driver_specific_t         ds      = NULL;
  vrp_outgoing_t                      vrp_out = NULL;
  mad_vrp_select_req_t                req;

  LOG_IN();
  out     = arg;
  d       = out->channel->adapter->driver;
  ds      = d->specific;
  os      = out->specific;
  vrp_out = os->vrp_out;
  req.fd      = vrp_outgoing_fd(vrp_out);

  while (1)
    {
      req.status = 0;
      marcel_poll(ds->level1_pollid, &req);

      if (req.status == -1)
        break;

      vrp_outgoing_read_callback(req.fd, vrp_out);
    }

  LOG_OUT();

  return NULL;
}

/* Level 2 Marcel polling */
static
void
mad_vrp_level2_marcel_group(marcel_pollid_t id)
{
  return;
}

static
int
mad_vrp_level2_do_poll(p_mad_vrp_poll_req_t req)
{
  p_mad_channel_t              ch     = NULL;
  p_mad_vrp_channel_specific_t chs    = NULL;
  p_tbx_darray_t               darray = NULL;
  int                          r      =    0;
  int                          n      =    0;
  int                          i      =    0;

  LOG_IN();
  ch     = req->ch;
  chs    = ch->specific;
  darray = ch->in_connection_darray;

  i = n = tbx_darray_length(darray);

  while (i--)
    {
      p_mad_connection_t _in = NULL;

      _in = tbx_darray_get(darray, chs->next);
      chs->next = (chs->next + 1) % n;

      if (_in)
        {
          p_mad_vrp_in_connection_specific_t _is = NULL;

          _is = _in->specific;

          if (_is->active)
            {
              _is->active =  tbx_false;
              req->c      = _in;
              r           =  1;

              break;
            }
        }
    }
  LOG_OUT();

  return r;
}

static
void *
mad_vrp_level2_marcel_fast_poll(marcel_pollid_t id,
                         any_t           req,
                         boolean         first_call)
{
  void *status = MARCEL_POLL_FAILED;

  LOG_IN();
  if (mad_vrp_level2_do_poll((p_mad_vrp_poll_req_t) req)) {
    status = MARCEL_POLL_SUCCESS_FOR(id);
  }
  LOG_OUT();

  return status;
}

static
void *
mad_vrp_level2_marcel_poll(marcel_pollid_t id,
                    unsigned        active,
                    unsigned        sleeping,
                    unsigned        blocked)
{
  p_mad_vrp_poll_req_t  req    = NULL;
  void                 *status = MARCEL_POLL_FAILED;

  LOG_IN();
  FOREACH_POLL(id) { GET_ARG(id, req);
    if (mad_vrp_level2_do_poll((p_mad_vrp_poll_req_t) req)) {
      status = MARCEL_POLL_SUCCESS(id);
      goto found;
    }
  }

 found:
  LOG_OUT();

  return status;
}


/* Level 1 Marcel polling */
static
void
mad_vrp_level1_marcel_group(marcel_pollid_t id)
{
  return;
}

static
int
mad_vrp_level1_do_poll(p_mad_vrp_select_req_t req)
{
  int fd     = -1;
  int r      =  0;
  int status = -1;
  struct timeval timeout;
  fd_set fds;

  LOG_IN();
  fd = req->fd;

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  status = select(fd+1, &fds, NULL, NULL, &timeout);

  if ((status == -1) && (errno != EINTR))
    {
      if (errno == EINTR)
        {
          goto end;
        }
      else if (errno == EBADF)
        {
          r = 1;
          goto end;
        }

      perror("select");
      FAILURE("system call failed");
    }

  if (status > 0)
    {
      if (!FD_ISSET(fd, &fds))
        FAILURE("invalid state");

      r = 1;
    }

 end:
  req->status = status;
  LOG_OUT();

  return r;
}

static
void *
mad_vrp_level1_marcel_fast_poll(marcel_pollid_t id,
                                any_t           req,
                                boolean         first_call)
{
  void *status = MARCEL_POLL_FAILED;

  LOG_IN();
  if (mad_vrp_level1_do_poll((p_mad_vrp_select_req_t) req)) {
    status = MARCEL_POLL_SUCCESS_FOR(id);
  }
  LOG_OUT();

  return status;
}

static
void *
mad_vrp_level1_marcel_poll(marcel_pollid_t id,
                    unsigned        active,
                    unsigned        sleeping,
                    unsigned        blocked)
{
  p_mad_vrp_poll_req_t  req    = NULL;
  void                 *status = MARCEL_POLL_FAILED;

  LOG_IN();
  FOREACH_POLL(id) { GET_ARG(id, req);
    if (mad_vrp_level1_do_poll((p_mad_vrp_select_req_t) req)) {
      status = MARCEL_POLL_SUCCESS(id);
      goto found;
    }
  }

 found:
  LOG_OUT();

  return status;
}




/*
 * Registration function
 * ---------------------
 */

void
mad_vrp_register(p_mad_driver_t driver)
{
  p_mad_driver_interface_t interface = NULL;

  LOG_IN();
  TRACE("Registering VRP driver");
  interface = driver->interface;

  driver->connection_type  = mad_unidirectional_connection;
  driver->buffer_alignment = 32;
  driver->name             = tbx_strdup("vrp");

  interface->driver_init                = mad_vrp_driver_init;
  interface->adapter_init               = mad_vrp_adapter_init;
  interface->channel_init               = mad_vrp_channel_init;
  interface->before_open_channel        = NULL;
  interface->connection_init            = mad_vrp_connection_init;
  interface->link_init                  = mad_vrp_link_init;
  interface->accept                     = mad_vrp_accept;
  interface->connect                    = mad_vrp_connect;
  interface->after_open_channel         = NULL;
  interface->before_close_channel       = NULL;
  interface->disconnect                 = mad_vrp_disconnect;
  interface->after_close_channel        = NULL;
  interface->link_exit                  = NULL;
  interface->connection_exit            = NULL;
  interface->channel_exit               = NULL;
  interface->adapter_exit               = NULL;
  interface->driver_exit                = mad_vrp_driver_exit;
  interface->choice                     = NULL;
  interface->get_static_buffer          = NULL;
  interface->return_static_buffer       = NULL;
  interface->new_message                = mad_vrp_new_message;
  interface->finalize_message           = NULL;
#ifdef MAD_MESSAGE_POLLING
  interface->poll_message               = mad_vrp_poll_message;
#endif // MAD_MESSAGE_POLLING
  interface->receive_message            = mad_vrp_receive_message;
  interface->message_received           = mad_vrp_message_received;
  interface->send_buffer                = mad_vrp_send_buffer;
  interface->receive_buffer             = mad_vrp_receive_buffer;
  interface->send_buffer_group          = mad_vrp_send_buffer_group;
  interface->receive_sub_buffer_group   = mad_vrp_receive_sub_buffer_group;
  LOG_OUT();
}


void
mad_vrp_driver_init(p_mad_driver_t d)
{
  p_mad_vrp_driver_specific_t ds = NULL;

  LOG_IN();
  TRACE("Initializing VRP driver");
  ds          = TBX_MALLOC(sizeof(mad_vrp_driver_specific_t));

  ds->level1_pollid = marcel_pollid_create(mad_vrp_level1_marcel_group,
                                    mad_vrp_level1_marcel_poll,
                                    mad_vrp_level1_marcel_fast_poll,
                                    MAD_VRP_LEVEL1_POLLING_MODE);

  ds->level2_pollid = marcel_pollid_create(mad_vrp_level2_marcel_group,
                                    mad_vrp_level2_marcel_poll,
                                    mad_vrp_level2_marcel_fast_poll,
                                    MAD_VRP_LEVEL2_POLLING_MODE);

  d->specific = ds;

  vrp_init();
  LOG_OUT();
}

void
mad_vrp_adapter_init(p_mad_adapter_t a)
{
  p_mad_vrp_adapter_specific_t as = NULL;
  p_ntbx_server_t              ns = NULL;

  LOG_IN();
  if (strcmp(a->dir_adapter->name, "default"))
    FAILURE("unsupported adapter");

  as = TBX_MALLOC(sizeof(mad_vrp_adapter_specific_t));
  ns = ntbx_server_cons();
  ntbx_tcp_server_init(ns);
  as->server = ns;
  a->specific = as;

#ifdef SSIZE_MAX
  a->mtu = tbx_min(SSIZE_MAX, MAD_FORWARD_MAX_MTU);
#else // SSIZE_MAX
  a->mtu = MAD_FORWARD_MAX_MTU;
#endif // SSIZE_MAX

  a->parameter = tbx_strdup(ns->connection_data.data);
  LOG_OUT();
}

void
mad_vrp_channel_init(p_mad_channel_t c)
{
  p_mad_vrp_channel_specific_t cs = NULL;

  LOG_IN();
  cs          = TBX_MALLOC(sizeof(mad_vrp_channel_specific_t));
  cs->next    = 0;
  c->specific = cs;
  LOG_OUT();
}

void
mad_vrp_connection_init(p_mad_connection_t in,
			p_mad_connection_t out)
{
  p_mad_vrp_in_connection_specific_t  is = NULL;
  p_mad_vrp_out_connection_specific_t os = NULL;

  LOG_IN();
  if (in)
    {
      is = TBX_MALLOC(sizeof(mad_vrp_in_connection_specific_t));

      is->socket              =   -1;
      is->port                =   -1;
      is->vrp_in              = NULL;
      is->thread              = NULL;
      is->buffer              = NULL;
      is->active              = tbx_false;
      memset(is->first_packet_data, 0, MAD_VRP_MAX_FIRST_PACKET_LENGTH);
      is->first_packet_length =    0;

      marcel_sem_init(&(is->sem_put), 1);
      marcel_sem_init(&(is->sem_get), 0);

      in->specific = is;
      in->nb_link  =  1;
    }

  if (out)
    {
      os = TBX_MALLOC(sizeof(mad_vrp_out_connection_specific_t));

      os->socket  =   -1;
      os->port    =   -1;
      os->vrp_out = NULL;
      os->first   = tbx_true;
      os->thread  = NULL;

      out->specific = os;
      out->nb_link  =  1;
    }
  LOG_OUT();
}

void
mad_vrp_link_init(p_mad_link_t lnk)
{
  LOG_IN();
  lnk->link_mode   = mad_link_mode_buffer;
  lnk->buffer_mode = mad_buffer_mode_dynamic;
  lnk->group_mode  = mad_group_mode_split;
  LOG_OUT();
}

void
mad_vrp_accept(p_mad_connection_t   in,
	       p_mad_adapter_info_t ai TBX_UNUSED)
{
  p_mad_vrp_adapter_specific_t       as         = NULL;
  p_mad_vrp_in_connection_specific_t is         = NULL;
  p_ntbx_client_t                    net_client = NULL;
  int                                vrp_port   =    0;

  LOG_IN();
  is = in->specific;
  as = in->channel->adapter->specific;

  net_client = ntbx_client_cons();
  ntbx_tcp_client_init(net_client);
  ntbx_tcp_server_accept(as->server, net_client);

  is->vrp_in = vrp_incoming_construct(&vrp_port, mad_vrp_frame_handler);
  vrp_incoming_set_source(is->vrp_in, is);
  mad_ntbx_send_int(net_client, vrp_port);
  marcel_create(&(is->thread), NULL, mad_vrp_incoming_thread, in);
  ntbx_tcp_client_disconnect(net_client);
  ntbx_client_dest(net_client);
  LOG_OUT();
}

void
mad_vrp_connect(p_mad_connection_t   out,
		p_mad_adapter_info_t ai)
{
  p_mad_vrp_out_connection_specific_t os         = NULL;
  p_mad_dir_node_t                    r_node     = NULL;
  p_mad_dir_adapter_t                 r_adapter  = NULL;
  p_ntbx_client_t                     net_client = NULL;
  int                                 vrp_port   =    0;

  LOG_IN();
  os        = out->specific;
  r_node    = ai->dir_node;
  r_adapter = ai->dir_adapter;

  net_client = ntbx_client_cons();
  ntbx_tcp_client_init(net_client);
  {
    ntbx_connection_data_t cnx_data;

    strncpy(cnx_data.data, r_adapter->parameter, NTBX_CONNECTION_DATA_LEN-1);
    cnx_data.data[NTBX_CONNECTION_DATA_LEN-1] = 0;
    ntbx_tcp_client_connect(net_client, r_node->name, &cnx_data);
  }
  vrp_port = mad_ntbx_receive_int(net_client);

  os->vrp_out = vrp_outgoing_construct(r_node->name, vrp_port, 0, 0, -1);
  marcel_create(&(os->thread), NULL, mad_vrp_outgoing_thread, out);
  ntbx_tcp_client_disconnect(net_client);
  ntbx_client_dest(net_client);
  vrp_outgoing_connect(os->vrp_out);
  LOG_OUT();
}

void
mad_vrp_disconnect(p_mad_connection_t c)
{
  LOG_IN();
  if (c->way == mad_incoming_connection)
    {
      p_mad_vrp_in_connection_specific_t is = NULL;

      is = c->specific;
      vrp_incoming_close(is->vrp_in);
      marcel_join(is->thread, NULL);
      TBX_FREE(is->vrp_in);
    }
  else if (c->way == mad_outgoing_connection)
    {
      p_mad_vrp_out_connection_specific_t os = NULL;

      os = c->specific;
      vrp_outgoing_close(os->vrp_out);
      marcel_join(os->thread, NULL);
      TBX_FREE(os->vrp_out);
    }
  else
    FAILURE("invalid connection way");

  LOG_OUT();
}

void
mad_vrp_driver_exit(p_mad_driver_t d TBX_UNUSED)
{
  LOG_IN();
  vrp_shutdown();
  LOG_OUT();
}

void
mad_vrp_new_message(p_mad_connection_t out)
{
  p_mad_vrp_out_connection_specific_t os = NULL;

  LOG_IN();
  os = out->specific;
  os->first = tbx_true;
  LOG_OUT();
}

#ifdef MAD_MESSAGE_POLLING
p_mad_connection_t
mad_vrp_poll_message(p_mad_channel_t ch)
{
  p_mad_vrp_channel_specific_t chs       = NULL;
  p_tbx_darray_t               in_darray = NULL;
  p_mad_connection_t           in        = NULL;

  LOG_IN();
  chs       = ch->specific;
  in_darray = ch->in_connection_darray;

  /* TODO */

  LOG_OUT();

  return NULL;
}
#endif // MAD_MESSAGE_POLLING

p_mad_connection_t
mad_vrp_receive_message(p_mad_channel_t ch)
{
  p_mad_connection_t          in = NULL;
  p_mad_vrp_driver_specific_t ds = NULL;
  mad_vrp_poll_req_t req;

  LOG_IN();
  ds = ch->adapter->driver->specific;

  req.ch = ch;
  req.c  = NULL;

  marcel_poll(ds->level2_pollid, &req);

  in = req.c;
  LOG_OUT();

  return in;
}

void
mad_vrp_message_received(p_mad_connection_t in)
{
  p_mad_vrp_in_connection_specific_t is = NULL;

  LOG_IN();
  is = in->specific;

  is->buffer = NULL;
  marcel_sem_V(&(is->sem_put));
  LOG_OUT();
}


void
mad_vrp_send_buffer(p_mad_link_t   lnk,
		    p_mad_buffer_t b)
{
  p_mad_vrp_out_connection_specific_t  os        = NULL;
  int                                  s         =    0;
  void                                *ptr       = NULL;
  size_t                               len       =    0;
  int                                  loss_rate =    0;
  vrp_buffer_t                         vrp_b;

  LOG_IN();
  os = lnk->connection->specific;

  ptr = b->buffer + b->bytes_read;
  len = b->bytes_written - b->bytes_read;

  if (os->first)
    {
      if (len > MAD_VRP_MAX_FIRST_PACKET_LENGTH)
        {
          unsigned char c = 0;

          vrp_b = vrp_buffer_construct(&c, 1);
          vrp_buffer_set_loss_rate(vrp_b, 0, 0);
          s = vrp_outgoing_send_frame(os->vrp_out, vrp_b);
          vrp_buffer_destroy(vrp_b);
        }

      os->first = tbx_false;
    }

  if (b->parameter_slist && !tbx_slist_is_nil(b->parameter_slist))
    {
      tbx_slist_ref_to_head(b->parameter_slist);
      do
        {
          p_mad_buffer_slice_parameter_t param = NULL;

          param = tbx_slist_ref_get(b->parameter_slist);
          if (param->opcode == mad_op_optional_block)
            {
              // note: offset and length are ignored for now
              // the whole block is either mandatory or optional
              loss_rate = 100;
            }
          else
            FAILURE("invalid vrp parameter opcode");
        }
      while(tbx_slist_ref_forward(b->parameter_slist));
    }

  TRACE_VAL("loss_rate", loss_rate);
  vrp_b = vrp_buffer_construct(ptr, len);
  vrp_buffer_set_loss_rate(vrp_b, loss_rate, len);
  s = vrp_outgoing_send_frame(os->vrp_out, vrp_b);
  vrp_buffer_destroy(vrp_b);
  b->bytes_read += len;
  LOG_OUT();
}

void
mad_vrp_receive_buffer(p_mad_link_t    lnk,
		       p_mad_buffer_t *p_b)
{
  p_mad_vrp_in_connection_specific_t is    = NULL;
  p_mad_buffer_t                     b     = NULL;
  tbx_bool_t                         first = tbx_false;

  LOG_IN();
  is    =  lnk->connection->specific;
  b     = *p_b;
  first =  (is->buffer == NULL);

  is->buffer = b;

  if (!first)
    {
      marcel_sem_V(&(is->sem_put));
    }

  marcel_sem_P(&(is->sem_get));

  if (first)
    {
      size_t len = 0;

      len = b->length - b->bytes_written;

      if (len <= MAD_VRP_MAX_FIRST_PACKET_LENGTH)
        {
          if (len != is->first_packet_length)
            FAILURE("stream out of sync");

          memcpy(b->buffer+b->bytes_written, is->first_packet_data, len);
          b->bytes_written += len;
        }
      else
        {
          TBX_CFREE2(is->lost);
          is->loss_num =    0;
          marcel_sem_V(&(is->sem_put));
          marcel_sem_P(&(is->sem_get));
        }
    }

  if (is->loss_num)
    {
      p_tbx_slist_t                  slist = NULL;
      p_mad_buffer_slice_parameter_t bsp   = NULL;
      int                            i     =    0;

      DISP_VAL("loss_num", is->loss_num);

      slist = tbx_slist_nil();

      for (i = 0; i < is->loss_num; i++)
        {
          if (i > 0 && is->lost[i] == (bsp->offset + bsp->length))
            {
              bsp->length += is->loss_granularity;
              continue;
            }

          bsp = mad_alloc_slice_parameter();
          bsp->base   = b->buffer;
          bsp->offset = is->lost[i];
          bsp->length = is->loss_granularity;
          bsp->opcode = mad_os_lost_block;
          bsp->value  = 0; // unused

          tbx_slist_append(slist, bsp);
        }

      b->parameter_slist = slist;
    }

  TBX_CFREE2(is->lost);
  is->loss_num = 0;
  LOG_OUT();
}

void
mad_vrp_send_buffer_group(p_mad_link_t         lnk,
			  p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      p_mad_vrp_out_connection_specific_t os = NULL;
      tbx_list_reference_t                ref;

      os = lnk->connection->specific;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
          mad_vrp_send_buffer(lnk, tbx_get_list_reference_object(&ref));
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

void
mad_vrp_receive_sub_buffer_group(p_mad_link_t         lnk,
                                 tbx_bool_t           first TBX_UNUSED,
				 p_mad_buffer_group_t bg)
{
  LOG_IN();
  if (!tbx_empty_list(&(bg->buffer_list)))
    {
      p_mad_vrp_in_connection_specific_t is = NULL;
      tbx_list_reference_t               ref;

      is = lnk->connection->specific;

      tbx_list_reference_init(&ref, &(bg->buffer_list));
      do
	{
          p_mad_buffer_t b = NULL;

          b = tbx_get_list_reference_object(&ref);
          mad_vrp_receive_buffer(lnk, &b);
	}
      while(tbx_forward_list_reference(&ref));
    }
  LOG_OUT();
}

/*
 * Local variables:
 *  c-basic-offset: 2
 *  c-hanging-braces-alist: '((defun-open before after) (class-open before after) (inline-open before after) (block-open before after) (brace-list-open) (brace-entry-open) (substatement-open before after) (block-close . c-snug-do-while) (extern-lang-open before after) (inexpr-class-open before after) (inexpr-class-close before))
 * End:
 */
