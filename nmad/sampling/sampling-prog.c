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

#ifndef CONFIG_PROTO_MAD3
#  error "sampling-prog requires option 'mad3_emu'"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include <madeleine.h>

/* Maximum size of a small message for a faked strategy */
#define NM_MAX_SMALL  (NM_SO_MAX_UNEXPECTED -	  \
		       NM_SO_GLOBAL_HEADER_SIZE - \
		       NM_SO_DATA_HEADER_SIZE)

static const int param_min_size = 1;
static const int param_max_size = (8*1024*1024);
static const int param_nb_samples = 200;
static const int param_dryrun_count = 10;

static int nm_ns_initialize_pw(struct nm_core *p_core,
			       struct nm_drv  *p_drv,
			       struct nm_gate *p_gate,
			       unsigned char *ptr,
			       struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw = NULL;
  const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER;
  const nm_tag_t tag = 0;
  const uint8_t seq = 0;
  int err = nm_so_pw_alloc(flags, &p_pw);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while initializing pw.\n", err);
      abort();
    }
  err = nm_so_pw_add_data(p_pw, tag + 128, seq, ptr, param_min_size, 0, 1, flags);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while initializing pw.\n", err);
      abort();
    }

  p_pw->p_gate = p_gate;
  p_pw->p_drv  = p_drv;
  p_pw->trk_id = NM_TRK_SMALL;

  *pp_pw	= p_pw;

 out:
  return err;
}

static void nm_ns_fill_data(unsigned char * data)
{
  unsigned char c = 0;
  int i = 0, n = 0;

  for (i = 0; i < param_max_size; i++){
    n += 7;
    c = (unsigned char)(n % 256);
    data[i] = c;
  }
}

/* *** raw driver send/recv ******************************** */

static void nm_ns_pw_send(struct nm_pkt_wrap*p_pw, struct puk_receptacle_NewMad_Driver_s*r, void*ptr, size_t len)
{
  int err;
  p_pw->v[0].iov_base = ptr;
  p_pw->v[0].iov_len  = len;
  err = r->driver->post_send_iov(r->_status, p_pw);
  while(err == -NM_EAGAIN)
    {
      err = r->driver->poll_send_iov(r->_status, p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while sending.\n", err);
      abort();
    }
}

static void nm_ns_pw_recv(struct nm_pkt_wrap*p_pw, struct puk_receptacle_NewMad_Driver_s*r, void*ptr, size_t len)
{
  int err;
  p_pw->v[0].iov_base = ptr;
  p_pw->v[0].iov_len  = len;
  err = r->driver->post_recv_iov(r->_status, p_pw);
  while(err == -NM_EAGAIN)
    {
      err = r->driver->poll_recv_iov(r->_status, p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while receiving.\n", err);
      abort();
    }
}

/* *** eager send/recv on trk #0 *************************** */

static void nm_ns_eager_send(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{ 
  struct puk_receptacle_NewMad_Driver_s*r = &p_gate->p_gate_drv_array[p_drv->id]->receptacle;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  int err;
  if(len <= NM_MAX_SMALL)
    {
      const uint8_t flags = NM_SO_DATA_USE_COPY;
      err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, ptr, len, 0, 1, flags, &p_pw);
    }
  else
    {
      const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER;
      err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, ptr, len, 0, 1, flags, &p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while initializing pw [eager send].\n", err);
      abort();
    }
  p_pw->p_drv  = p_drv;
  p_pw->p_gate = p_gate;
  p_pw->trk_id = NM_TRK_SMALL;
  nm_so_pw_finalize(p_pw);
  err = r->driver->post_send_iov(r->_status, p_pw);
  while(err == -NM_EAGAIN)
    {
      err = r->driver->poll_send_iov(r->_status, p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while sending [eager send].\n", err);
      abort();
    }
  nm_so_pw_free(p_pw);
}

static void nm_ns_eager_recv(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{
  struct puk_receptacle_NewMad_Driver_s*r = &p_gate->p_gate_drv_array[p_drv->id]->receptacle;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  int err;
  if(len <= NM_MAX_SMALL)
    {
      const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER | NM_SO_DATA_PREPARE_RECV;
      err = nm_so_pw_alloc(flags, &p_pw);
    }
  else
    {
      const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER;
      err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, ptr, len, 0, 1, flags, &p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while initializing pw [eager recv].\n", err);
      abort();
    }
  p_pw->p_drv  = p_drv;
  p_pw->p_gate = p_gate;
  p_pw->trk_id = NM_TRK_SMALL;
  err = r->driver->post_recv_iov(r->_status, p_pw);
  while(err == -NM_EAGAIN)
    {
      err = r->driver->poll_recv_iov(r->_status, p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while receiving.\n", err);
      abort();
    }
  if(len <= NM_MAX_SMALL)
    {
      /* very rough header decoder... should be sufficient for our basic use here. */
      struct nm_so_global_header*gh = p_pw->v[0].iov_base;
      nm_tag_t proto_id = *(nm_tag_t*)(gh + 1);
      if(proto_id >= NM_SO_PROTO_DATA_FIRST)
	{
	  struct nm_so_data_header*dh = (struct nm_so_data_header*)(gh + 1);
	  size_t data_len = dh->len;
	  memcpy(ptr, dh + 1, data_len);
	}
    }
  nm_so_pw_free(p_pw);
}


/* *** rdv send/recv on trk #1 ***************************** */

static void nm_ns_rdv_send(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{ 
  int rdv_req = 1, ok_to_send = 0;
  nm_ns_eager_send(p_drv, p_gate, &rdv_req, sizeof(rdv_req));
  nm_ns_eager_recv(p_drv, p_gate, &ok_to_send, sizeof(ok_to_send));
  
  struct puk_receptacle_NewMad_Driver_s*r = &p_gate->p_gate_drv_array[p_drv->id]->receptacle;
  const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  int err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, ptr, len, 0, 1, flags, &p_pw);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while initializing pw [rdv send].\n", err);
      abort();
    }
  p_pw->p_drv  = p_drv;
  p_pw->p_gate = p_gate;
  p_pw->trk_id = NM_TRK_LARGE;
  err = r->driver->post_send_iov(r->_status, p_pw);
  while(err == -NM_EAGAIN)
    {
      err = r->driver->poll_send_iov(r->_status, p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while sending [rdv send].\n", err);
      abort();
    }
  nm_so_pw_free(p_pw);
}

static void nm_ns_rdv_recv(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{

  struct puk_receptacle_NewMad_Driver_s*r = &p_gate->p_gate_drv_array[p_drv->id]->receptacle;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  int err;
  const uint8_t flags = NM_SO_DATA_DONT_USE_HEADER;
  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq, ptr, len, 0, 1, flags, &p_pw);
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while initializing pw [rdv recv].\n", err);
      abort();
    }
  p_pw->p_drv  = p_drv;
  p_pw->p_gate = p_gate;
  p_pw->trk_id = NM_TRK_LARGE;
  err = r->driver->post_recv_iov(r->_status, p_pw);

  int rdv_req = 0, ok_to_send = 1;
  nm_ns_eager_recv(p_drv, p_gate, &rdv_req, sizeof(rdv_req));
  nm_ns_eager_send(p_drv, p_gate, &ok_to_send, sizeof(ok_to_send));

  while(err == -NM_EAGAIN)
    {
      err = r->driver->poll_recv_iov(r->_status, p_pw);
    }
  if(err != NM_ESUCCESS)
    {
      fprintf(stderr, "sampling: error %d while receiving.\n", err);
      abort();
    }
  nm_so_pw_free(p_pw);
}


static int nm_ns_ping(struct nm_drv *p_drv, struct nm_gate *p_gate, FILE*sampling_file)
{
  struct puk_receptacle_NewMad_Driver_s*r = &p_gate->p_gate_drv_array[0]->receptacle;
  struct nm_pkt_wrap * sending_pw = NULL;
  struct nm_pkt_wrap * receiving_pw = NULL;
  unsigned char *data_send = TBX_MALLOC(param_max_size);
  unsigned char *data_recv = TBX_MALLOC(param_max_size);
  int size = 0;

  nm_ns_fill_data(data_send);
  nm_ns_initialize_pw(p_drv->p_core, p_drv, p_gate, data_send, &sending_pw);
  nm_ns_initialize_pw(p_drv->p_core, p_drv, p_gate, data_recv, &receiving_pw);

  int i;
  for(i = 0; i < param_dryrun_count; i++)
    {
      nm_ns_pw_send(sending_pw, r, data_send, param_min_size);
      nm_ns_pw_recv(receiving_pw, r, data_recv, param_min_size);
    }

  for (size = param_min_size; size <= param_max_size; size*= 2)
    {
      int nb_samples;
      tbx_tick_t t1, t2;
      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_pw_send(sending_pw, r, data_send, size);
	  nm_ns_pw_recv(receiving_pw, r, data_recv, size);
	}
      TBX_GET_TICK(t2);
      double lat = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);
      double bw_million_byte = size / lat;
      double bw_mbyte        = bw_million_byte / 1.048576;

      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_send(p_drv, p_gate, data_send, size);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size);
	}
      TBX_GET_TICK(t2);
      double eager_lat = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);

      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_rdv_send(p_drv, p_gate, data_send, size);
	  nm_ns_rdv_recv(p_drv, p_gate, data_recv, size);
	}
      TBX_GET_TICK(t2);
      double rdv_lat = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);

      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_send(p_drv, p_gate, data_send, size/2);
	  nm_ns_eager_send(p_drv, p_gate, data_send + size/2, size/2);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size/2);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv + size/2, size/2);
	}
      TBX_GET_TICK(t2);
      double eager_noaggreg_lat = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);

      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  memcpy(data_recv, data_send, size);
	}
      TBX_GET_TICK(t2);
      double memcpy_lat = TBX_TIMING_DELAY(t1, t2) / param_nb_samples;
      
      fprintf(stderr, "%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", size, lat, bw_mbyte, eager_lat, rdv_lat, eager_noaggreg_lat, memcpy_lat);
      fprintf(sampling_file, "%d\t%lf\n", size, bw_mbyte);
      
      size = (size==0? 1:size);
    }

  nm_so_pw_free(sending_pw);
  nm_so_pw_free(receiving_pw);

  return NM_ESUCCESS;
}

static int nm_ns_pong(struct nm_drv *p_drv, struct nm_gate *p_gate)
{
  struct puk_receptacle_NewMad_Driver_s*r = &p_gate->p_gate_drv_array[0]->receptacle;
  struct nm_pkt_wrap * sending_pw = NULL;
  struct nm_pkt_wrap * receiving_pw = NULL;
  unsigned char *data_send = TBX_MALLOC(param_max_size);
  unsigned char *data_recv = TBX_MALLOC(param_max_size);
  int size = 0;

  nm_ns_fill_data(data_send);
  nm_ns_initialize_pw(p_drv->p_core, p_drv, p_gate, data_send, &sending_pw);
  nm_ns_initialize_pw(p_drv->p_core, p_drv, p_gate, data_recv, &receiving_pw);

  int i;
  for(i = 0; i < param_dryrun_count; i++)
    {
      nm_ns_pw_recv(receiving_pw, r, data_recv, param_min_size);
      nm_ns_pw_send(sending_pw, r, data_send, param_min_size);
    }

  for (size = param_min_size; size <= param_max_size; size*= 2)
    {
     int nb_samples;
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_pw_recv(receiving_pw, r, data_recv, size);
	  nm_ns_pw_send(sending_pw, r, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size);
	  nm_ns_eager_send(p_drv, p_gate, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_rdv_recv(p_drv, p_gate, data_recv, size);
	  nm_ns_rdv_send(p_drv, p_gate, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size/2);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv + size/2, size/2);
	  nm_ns_eager_send(p_drv, p_gate, data_send, size/2);
	  nm_ns_eager_send(p_drv, p_gate, data_send + size/2, size/2);
	}
      size = (size==0? 1:size);
    }
  
  nm_so_pw_free(sending_pw);
  nm_so_pw_free(receiving_pw);
  TBX_FREE(data_send);
  TBX_FREE(data_recv);

  return NM_ESUCCESS;
}

int main(int argc, char **argv)
{
  /* Initialisation de l'émulation */
  p_mad_madeleine_t madeleine = mad_init(&argc, argv);
  p_mad_session_t   session   = madeleine->session;
  int               is_server = session->process_rank;

  /* Initialisation de Nmad */
  struct nm_core  *p_core = mad_nmad_get_core();
  struct nm_drv   *p_drv  = &p_core->driver_array[0];
  struct nm_gate  *p_gate = &p_core->gate_array[0];

  if(!is_server)
    {
      const char*filename = argv[1];
      fprintf(stderr, "# sampling:  writing file %s\n", filename);
      fprintf(stderr, "# size\t|raw latency\t|raw bw \t| trk#0 latency\t| trk#1 latency \n");
      FILE*sampling_file = fopen(filename, "w");
      if(sampling_file == NULL)
	{
	  fprintf(stderr, "# ## sampling: cannot write file %s.\n", filename);
	  abort();
	}
      nm_ns_ping(p_drv, p_gate, sampling_file);
      fclose(sampling_file);
    }
  else
    {
      nm_ns_pong(p_drv, p_gate);
    }

  mad_exit(madeleine);
  return 0;
}

