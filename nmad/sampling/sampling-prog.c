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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>

/* Maximum size of a small message for a faked strategy */
#define NM_MAX_SMALL  (NM_SO_MAX_UNEXPECTED - NM_SO_DATA_HEADER_SIZE)

static const int param_min_size = 1;
static const int param_max_size = (8*1024*1024);
static const int param_dryrun_count = 10;

static unsigned char*data_send = NULL;
static unsigned char*data_recv = NULL;

struct nm_sample_s
{
  int size;
  double drv_lat;
  double drv_bw;
  double trk0_copy_lat;
  double trk0_iov_lat;
  double trk1_rdv_lat;
  double trk0_x2_lat;
  double aggreg_lat;
};
PUK_VECT_TYPE(nm_sample, struct nm_sample_s);
static nm_sample_vect_t samples = NULL;

static inline int nm_ns_nb_samples(int size)
{
  if(size < 32768)
    return 1000;
  else if(size < 1024 * 1024)
    return 100;
  else
    return 20;
}

static void nm_ns_initialize_pw(struct nm_core *p_core,
				struct nm_drv  *p_drv,
				struct nm_gate *p_gate,
				unsigned char *ptr,
				struct nm_pkt_wrap **pp_pw)
{
  struct nm_pkt_wrap *p_pw = NULL;
  const nm_tag_t tag = 0;
  const uint8_t seq = 0;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  nm_so_pw_add_raw(p_pw, ptr, param_min_size, 0);
  nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
  *pp_pw = p_pw;
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
  p_pw->length = len;
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
  p_pw->length = len;
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

static void nm_ns_eager_send_copy(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{ 
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag = 0;
  const uint8_t seq  = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  if(len <= NM_MAX_SMALL)
    {
      nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
      nm_so_pw_add_data_in_header(p_pw, tag, seq, ptr, len, 0, 0);
    }
  else
    {
      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      nm_so_pw_add_raw(p_pw, ptr, len, 0);
    }
  nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
  nm_so_pw_finalize(p_pw);
  int err = r->driver->post_send_iov(r->_status, p_pw);
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

static void nm_ns_eager_send_iov(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{ 
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag = 0;
  const uint8_t seq  = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  if(len <= NM_MAX_SMALL)
    {
      nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
      nm_so_pw_add_data_in_iovec(p_pw, tag, seq, ptr, len, 0, 0);
    }
  else
    {
      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      nm_so_pw_add_raw(p_pw, ptr, len, 0);
    }
  nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
  nm_so_pw_finalize(p_pw);
  int err = r->driver->post_send_iov(r->_status, p_pw);
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
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  char*buf = NULL;
  if(len <= NM_MAX_SMALL)
    {
      nm_so_pw_alloc(NM_PW_BUFFER, &p_pw);
    }
  else
    {
      buf = malloc(len);
      nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
      nm_so_pw_add_raw(p_pw, buf, len, 0);
    }
  nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
  int err = r->driver->post_recv_iov(r->_status, p_pw);
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
      const nm_proto_t*p_proto = p_pw->v[0].iov_base;
      const nm_proto_t proto_id = (*p_proto) & NM_PROTO_ID_MASK;
      if(proto_id == NM_PROTO_SHORT_DATA)
	{
	  const struct nm_so_short_data_header*dh = (const struct nm_so_short_data_header*)p_proto;
	  const size_t data_len = dh->len;
	  memcpy(ptr, dh + 1, data_len);
	}
      else if(proto_id == NM_PROTO_DATA)
	{
	  const struct nm_so_data_header*dh = (const struct nm_so_data_header*)p_proto;
	  const size_t data_len = dh->len;
	  memcpy(ptr, dh + 1, data_len);
	}
    }
  else
    {
      memcpy(ptr, buf, len);
      free(buf);
    }
  nm_so_pw_free(p_pw);
}

/* *** eager, aggregation ********************************** */

static void nm_ns_eager_send_aggreg(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t _len)
{ 
  const size_t len = _len / 2;
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag = 0;
  const uint8_t seq  = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  if(_len <= NM_MAX_SMALL)
    {
      nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
      nm_so_pw_add_data_in_header(p_pw, tag, seq, ptr, len, 0, 0);
      nm_so_pw_add_data_in_header(p_pw, tag, seq, ptr + len, len, 0, 0);
      nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
      nm_so_pw_finalize(p_pw);
      int err = r->driver->post_send_iov(r->_status, p_pw);
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
}

static void nm_ns_eager_recv_aggreg(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t _len)
{
  const size_t len = _len / 2;
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  char*buf = NULL;
  if(_len <= NM_MAX_SMALL)
    {
      nm_so_pw_alloc(NM_PW_BUFFER, &p_pw);
      nm_so_pw_assign(p_pw, NM_TRK_SMALL, p_drv, p_gate);
      int err = r->driver->post_recv_iov(r->_status, p_pw);
      while(err == -NM_EAGAIN)
	{
	  err = r->driver->poll_recv_iov(r->_status, p_pw);
	}
      if(err != NM_ESUCCESS)
	{
	  fprintf(stderr, "sampling: error %d while receiving.\n", err);
	  abort();
	}
      /* very rough header decoder... should be sufficient for our basic use here. */
      nm_proto_t*p_proto = p_pw->v[0].iov_base;
      nm_proto_t proto;
      do
	{
	  proto = *p_proto;
	  const nm_proto_t proto_id = (*p_proto) & NM_PROTO_ID_MASK;
	  if(proto_id == NM_PROTO_SHORT_DATA)
	    {
	      const struct nm_so_short_data_header*dh = (const struct nm_so_short_data_header*)p_proto;
	      const size_t data_len = dh->len;
	      memcpy(ptr, dh + 1, dh->len);
	      p_proto += NM_SO_SHORT_DATA_HEADER_SIZE + dh->len;
	      ptr += dh->len;
	    }
	  else if(proto_id == NM_PROTO_DATA)
	    {
	      const struct nm_so_data_header*dh = (const struct nm_so_data_header*)p_proto;
	      const size_t data_len = dh->len;
	      memcpy(ptr, dh + dh->skip, dh->len);
	      const size_t size = (dh->flags & NM_PROTO_FLAG_ALIGNED) ? nm_so_aligned(dh->len) : dh->len;
	      p_proto += NM_SO_DATA_HEADER_SIZE;
	      if(dh->skip == 0)
		p_proto += size;
	      ptr += dh->len;
	    }
	  else
	    TBX_FAILUREF("unexpected proto %x; len = %d (%d)\n", proto_id, len, _len);
	} while( !(proto & NM_PROTO_LAST) );
      nm_so_pw_free(p_pw);
    }
}


/* *** rdv send/recv on trk #1 ***************************** */

static void nm_ns_rdv_send(struct nm_drv*p_drv, nm_gate_t p_gate, void*ptr, size_t len)
{ 
  int rdv_req = 1, ok_to_send = 0;
  nm_ns_eager_send_copy(p_drv, p_gate, &rdv_req, sizeof(rdv_req));
  nm_ns_eager_recv(p_drv, p_gate, &ok_to_send, sizeof(ok_to_send));
  
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag = 0;
  const uint8_t seq  = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  nm_so_pw_add_raw(p_pw, ptr, len, 0);
  nm_so_pw_assign(p_pw, NM_TRK_LARGE, p_drv, p_gate);
  int err = r->driver->post_send_iov(r->_status, p_pw);
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
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  const nm_tag_t tag  = 0;
  const uint8_t seq   = 0;
  struct nm_pkt_wrap*p_pw = NULL;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  nm_so_pw_add_raw(p_pw, ptr, len, 0);
  nm_so_pw_assign(p_pw, NM_TRK_LARGE, p_drv, p_gate);
  int err = r->driver->post_recv_iov(r->_status, p_pw);

  int rdv_req = 0, ok_to_send = 1;
  nm_ns_eager_recv(p_drv, p_gate, &rdv_req, sizeof(rdv_req));
  nm_ns_eager_send_copy(p_drv, p_gate, &ok_to_send, sizeof(ok_to_send));

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

/* *** pingpong ******************************************** */

static int nm_ns_ping(struct nm_drv *p_drv, struct nm_gate *p_gate, FILE*sampling_file)
{
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  struct nm_pkt_wrap * sending_pw = NULL;
  struct nm_pkt_wrap * receiving_pw = NULL;
  int size = 0;

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
      const int param_nb_samples = nm_ns_nb_samples(size);
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
	  nm_ns_eager_send_copy(p_drv, p_gate, data_send, size);
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
	  nm_ns_eager_send_iov(p_drv, p_gate, data_send, size);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size);
	}
      TBX_GET_TICK(t2);
      double eager_iov_lat = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);

      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  memcpy(data_recv, data_send, size);
	}
      TBX_GET_TICK(t2);
      double memcpy_lat = TBX_TIMING_DELAY(t1, t2) / param_nb_samples;
      
      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_send_iov(p_drv, p_gate, data_send, size/2);
	  nm_ns_eager_send_iov(p_drv, p_gate, data_send+size/2, size/2);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size/2);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv+size/2, size/2);
	}
      TBX_GET_TICK(t2);
      double eager_iov_split = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);

      TBX_GET_TICK(t1);
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_send_aggreg(p_drv, p_gate, data_send, size);
	  nm_ns_eager_recv_aggreg(p_drv, p_gate, data_recv, size);
	}
      TBX_GET_TICK(t2);
      double eager_iov_aggreg = TBX_TIMING_DELAY(t1, t2) / (2 * param_nb_samples);

      fprintf(stderr, "%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
	      size, lat, bw_mbyte, eager_lat, rdv_lat, eager_iov_lat, memcpy_lat, eager_iov_split, eager_iov_aggreg);
      fprintf(sampling_file, "%d\t%lf\n", size, bw_mbyte);
      struct nm_sample_s sample =
	{
	  .size = size,
	  .drv_lat = lat,
	  .drv_bw  = bw_mbyte,
	  .trk0_copy_lat = eager_lat,
	  .trk0_iov_lat = eager_iov_lat,
	  .trk1_rdv_lat = rdv_lat,
	  .trk0_x2_lat = eager_iov_split,
	  .aggreg_lat = eager_iov_aggreg
	};
      nm_sample_vect_push_back(samples, sample);
      size = (size==0? 1:size);
    }

  nm_so_pw_free(sending_pw);
  nm_so_pw_free(receiving_pw);

  return NM_ESUCCESS;
}

static int nm_ns_pong(struct nm_drv *p_drv, struct nm_gate *p_gate)
{
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct puk_receptacle_NewMad_Driver_s*r = &p_gdrv->receptacle;
  struct nm_pkt_wrap * sending_pw = NULL;
  struct nm_pkt_wrap * receiving_pw = NULL;
  int size = 0;

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
      const int param_nb_samples = nm_ns_nb_samples(size);
      int nb_samples;
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_pw_recv(receiving_pw, r, data_recv, size);
	  nm_ns_pw_send(sending_pw, r, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size);
	  nm_ns_eager_send_copy(p_drv, p_gate, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_rdv_recv(p_drv, p_gate, data_recv, size);
	  nm_ns_rdv_send(p_drv, p_gate, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size);
	  nm_ns_eager_send_iov(p_drv, p_gate, data_send, size);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_recv(p_drv, p_gate, data_recv, size/2);
	  nm_ns_eager_recv(p_drv, p_gate, data_recv+size/2, size/2);
	  nm_ns_eager_send_iov(p_drv, p_gate, data_send, size/2);
	  nm_ns_eager_send_iov(p_drv, p_gate, data_send+size/2, size/2);
	}
      for(nb_samples = 0; nb_samples < param_nb_samples; nb_samples++)
	{
	  nm_ns_eager_recv_aggreg(p_drv, p_gate, data_recv, size);
	  nm_ns_eager_send_aggreg(p_drv, p_gate, data_send, size);
	}
      size = (size==0? 1:size);
    }
  
  nm_so_pw_free(sending_pw);
  nm_so_pw_free(receiving_pw);
  TBX_FREE(data_send);
  TBX_FREE(data_recv);

  return NM_ESUCCESS;
}

/* ********************************************************* */

static void nm_ns_compute_thresholds(void)
{
  /* copy/iov threshold */
  int size = 0;
  int i = 1; /* init to 1 to leave room for interpolation (i-1 must exist) */
  struct nm_sample_s s;
  while(size < NM_MAX_SMALL)
    {
      s = nm_sample_vect_at(samples, i);
      size = s.size;
      if(s.trk0_copy_lat > s.trk0_iov_lat)
	{
	  break;
	}
      i++;
    }
  fprintf(stderr, "# copy_on_send_threshold = %d\n", size);
  fprintf(stderr, "#   copy = %f; iov = %f\n", s.trk0_copy_lat, s.trk0_iov_lat);
  struct nm_sample_s s0 = nm_sample_vect_at(samples, i - 1);
  struct nm_sample_s s1 = nm_sample_vect_at(samples, i);
  const int copy_on_send = s0.size +
    ( ( (s1.size - s0.size)*(s0.trk0_copy_lat - s0.trk0_iov_lat) ) / 
      ( s1.trk0_iov_lat - s0.trk0_iov_lat + s0.trk0_copy_lat - s1.trk0_copy_lat ) );
  fprintf(stderr, "#  interpolated = %d\n", copy_on_send);
  /* don't re-initialize i; rdv threshold must be higher than copy_on_send */
  while(size < NM_MAX_SMALL)
    {
      s = nm_sample_vect_at(samples, i);
      size = s.size;
      if(s.trk0_iov_lat > s.trk1_rdv_lat)
	{
	  break;
	}
      i++;
    }
  fprintf(stderr, "# rdv_threshold = %d\n", size);
  fprintf(stderr, "#   eager = %f; rdv = %f\n", s.trk0_iov_lat, s.trk1_rdv_lat);
  s0 = nm_sample_vect_at(samples, i - 1);
  s1 = nm_sample_vect_at(samples, i);
  const int rdv_threshold = s0.size +
    ( ( (s1.size - s0.size)*(s0.trk1_rdv_lat - s0.trk0_iov_lat) ) / 
      ( s1.trk0_iov_lat - s0.trk0_iov_lat + s0.trk1_rdv_lat - s1.trk1_rdv_lat ) );
  fprintf(stderr, "#  interpolated = %d\n", rdv_threshold);

  i = 1;
  do
    {
      s = nm_sample_vect_at(samples, i);
      size = s.size;
      if(s.aggreg_lat > s.trk0_x2_lat)
	{
	  break;
	}
      i++;
    }
  while(size < NM_MAX_SMALL);
  fprintf(stderr, "# aggreg_threshold = %d\n", size);
  fprintf(stderr, "#   2 packs = %f; aggreg = %f\n", s.trk0_x2_lat, s.aggreg_lat);
  s0 = nm_sample_vect_at(samples, i - 1);
  s1 = nm_sample_vect_at(samples, i);
  const int aggreg_threshold = s0.size +
    ( ( (s1.size - s0.size)*(s0.aggreg_lat - s0.trk0_x2_lat) ) / 
      ( s1.trk0_x2_lat - s0.trk0_x2_lat + s0.aggreg_lat - s1.aggreg_lat ) );
  fprintf(stderr, "#  interpolated = %d\n", aggreg_threshold);
}

int main(int argc, char **argv)
{
  int is_server = -1, rank, peer;
  nm_session_t p_session = NULL;
  nm_gate_t p_gate = NULL;
  nm_launcher_init(&argc, argv);
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  is_server = !rank;
  peer = 1 - rank;
  nm_launcher_get_gate(peer, &p_gate);
  struct nm_drv*p_drv = nm_drv_default(p_gate);

  assert(p_gate != NULL);
  assert(p_gate->status == NM_GATE_STATUS_CONNECTED);

  samples = nm_sample_vect_new();

  data_send = TBX_MALLOC(param_max_size);
  data_recv = TBX_MALLOC(param_max_size);
  nm_ns_fill_data(data_send);

  if(!is_server)
    {
      const char*filename = argv[1];
      fprintf(stderr, "# sampling:  writing file %s\n", filename);
      fprintf(stderr, "# size\t|raw latency\t|raw bw \t| trk#0 latency\t| trk#1 latency\t| trk#0 iov\t| memcpy\t|2 pkts \t| aggreg\n");
      FILE*sampling_file = fopen(filename, "w");
      if(sampling_file == NULL)
	{
	  fprintf(stderr, "# ## sampling: cannot write file %s.\n", filename);
	  abort();
	}
      nm_ns_ping(p_drv, p_gate, sampling_file);
      fclose(sampling_file);
      nm_ns_compute_thresholds();
    }
  else
    {
      nm_ns_pong(p_drv, p_gate);
    }

  nm_launcher_exit();

  return 0;
}

