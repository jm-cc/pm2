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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <assert.h>
#include <values.h>

#include <nm_private.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <nm_launcher.h>
#include <tbx.h>

/* Maximum size of a small message for a faked strategy */
#define NM_MAX_SMALL  (NM_SO_MAX_UNEXPECTED - NM_HEADER_DATA_SIZE)

static const int param_min_size = 1;
static const int param_max_size = (32*1024*1024);

static unsigned char*p_buffer_send = NULL;
static unsigned char*p_buffer_recv = NULL;

struct nm_sample_bench_s
{
  const char*name;
  void (*send)(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len);
  void (*recv)(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len);
};

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
  else if(size < 512 * 1024)
    return 100;
  else if(size < 8 * 1024 * 1024)
    return 20;
  else 
    return 5;
}

static void nm_ns_fill_data(unsigned char*data)
{
  unsigned char c = 0;
  int i = 0, n = 0;

  for (i = 0; i < param_max_size; i++){
    n += 7;
    c = (unsigned char)(n % 256);
    data[i] = c;
  }
}

/* ** helper for minidriver ******************************** */

static inline void minidriver_send_iov(struct puk_receptacle_NewMad_minidriver_s*r, struct iovec*v, int n)
{
  int rc = -1;
  struct nm_data_s data;
  if(r->driver->send_post)
    {
      (*r->driver->send_post)(r->_status, v, n);
    }
  else
    {
      nm_data_iov_build(&data, v, n);
      const nm_len_t len = nm_data_size(&data);
      (*r->driver->send_data)(r->_status, &data, 0, len);
    }
  do
    {
      rc = (*r->driver->send_poll)(r->_status);
    }
  while(rc != 0);
}

static inline void minidriver_send_data(struct puk_receptacle_NewMad_minidriver_s*r, const struct nm_data_s*p_data)
{
  int rc = -1;
  const nm_len_t len = nm_data_size(p_data);
  if(r->driver->send_data)
    {
      (*r->driver->send_data)(r->_status, p_data, 0, len);
    }
  else
    {
      void*buf = nm_data_baseptr_get(p_data);
      struct iovec v = { .iov_base = buf, .iov_len = len };
      (*r->driver->send_post)(r->_status, &v, 1);
    }
  do
    {
      rc = (*r->driver->send_poll)(r->_status);
    }
  while(rc != 0);
}

static inline void minidriver_send(struct puk_receptacle_NewMad_minidriver_s*r, void*buf, size_t len)
{
  int rc = -1;
  struct iovec v = { .iov_base = buf, .iov_len = len };
  struct nm_data_s data;
  if(r->driver->send_post)
    {
      (*r->driver->send_post)(r->_status, &v, 1);
    }
  else
    {
      nm_data_contiguous_build(&data, buf, len);
      (*r->driver->send_data)(r->_status, &data, 0, len);
    }
  do
    {
      rc = (*r->driver->send_poll)(r->_status);
    }
  while(rc != 0);
}

static inline void minidriver_recv(struct puk_receptacle_NewMad_minidriver_s*r, void*buf, size_t len)
{
  int rc = -1;
  struct iovec v = { .iov_base = buf, .iov_len = len };
  struct nm_data_s data;
  if(r->driver->recv_init)
    {
      (*r->driver->recv_init)(r->_status, &v, 1);
    }
  else
    {
      nm_data_contiguous_build(&data, buf, len);
      (*r->driver->recv_data)(r->_status, &data, 0, len);
    }
  do
    {
      rc = (*r->driver->poll_one)(r->_status);
    }
  while(rc != 0);
}
static inline void minidriver_recv_data(struct puk_receptacle_NewMad_minidriver_s*r, const struct nm_data_s*p_data)
{
  int rc = -1;
  const nm_len_t len = nm_data_size(p_data);
  if(r->driver->recv_data)
    {
      (*r->driver->recv_data)(r->_status, p_data, 0, len);
    }
  else
    {
      void*buf = nm_data_baseptr_get(p_data);
      struct iovec v = { .iov_base = buf, .iov_len = len };
      (*r->driver->recv_init)(r->_status, &v, 1);
    }
  do
    {
      rc = (*r->driver->poll_one)(r->_status);
    }
  while(rc != 0);
}


/* *** raw driver send/recv ******************************** */

static void nm_ns_driver_send(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  if(len > max_small)
    minidriver_send(&p_trk_small->receptacle, buf, len);
  else
    minidriver_send(&p_trk_large->receptacle, buf, len);
}

static void nm_ns_driver_recv(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  if(len > max_small)
    minidriver_recv(&p_trk_small->receptacle, buf, len);
  else
    minidriver_recv(&p_trk_large->receptacle, buf, len);
}

/* *** eager send/recv on trk #0 *************************** */

static void nm_ns_eager_send_copy(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{ 
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  const nm_core_tag_t tag = nm_core_tag_build(0, 0);
  const nm_seq_t seq = NM_SEQ_FIRST;
  struct nm_pkt_wrap_s*p_pw = NULL;
  struct nm_data_s data;
  nm_data_contiguous_build(&data, buf, len);
  if(len <= max_small)
    {
      p_pw = nm_pw_alloc_global_header();
      nm_pw_add_data_in_header(p_pw, tag, seq, &data, len, 0, 0);
      p_pw->trk_id = NM_TRK_SMALL;
      nm_pw_finalize(p_pw);
      minidriver_send(&p_trk_small->receptacle, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
    }
  else
    {
      p_pw = nm_pw_alloc_noheader();
      nm_pw_set_data_raw(p_pw, &data, len, 0);
      minidriver_send_data(&p_trk_large->receptacle, p_pw->p_data);
    }
  nm_pw_free(p_pw);
}

static void nm_ns_eager_send_iov(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{ 
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  const nm_core_tag_t tag = nm_core_tag_build(0, 0);
  const nm_seq_t seq = NM_SEQ_FIRST;
  struct nm_pkt_wrap_s*p_pw = NULL;
  struct nm_data_s data;
  nm_data_contiguous_build(&data, buf, len);
  if(len <= max_small)
    {
      p_pw = nm_pw_alloc_global_header();
      nm_data_pkt_pack(p_pw, tag, seq, &data, 0 /* chunk_offset = 0 */, len, 0);
      p_pw->trk_id = NM_TRK_SMALL;
      nm_pw_finalize(p_pw);
      minidriver_send_iov(&p_trk_small->receptacle, p_pw->v, p_pw->v_nb);
    }
  else
    {
      p_pw = nm_pw_alloc_noheader();
      nm_pw_set_data_raw(p_pw, &data, len, 0);
      minidriver_send_data(&p_trk_large->receptacle, p_pw->p_data);
    }
  nm_pw_free(p_pw);
}


static void nm_ns_eager_recv(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  struct nm_pkt_wrap_s*p_pw = NULL;
  char*buf2 = NULL;
  if(len <= max_small)
    {
      p_pw = nm_pw_alloc_buffer();
      minidriver_recv(&p_trk_small->receptacle, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
    }
  else
    {
      struct nm_data_s data;
      nm_data_contiguous_build(&data, buf2, len);
      buf2 = malloc(len);
      p_pw = nm_pw_alloc_noheader();
      nm_pw_set_data_raw(p_pw, &data, len, 0);
      minidriver_recv_data(&p_trk_large->receptacle, &data);
    }
  if(len <= NM_MAX_SMALL)
    {
      /* very rough header decoder... should be sufficient for our basic use here. */
      struct nm_header_global_s*p_global_header = p_pw->v[0].iov_base;
      void*ptr = p_global_header + 1;
      const nm_proto_t*p_proto = ptr;
      const nm_proto_t proto_id = (*p_proto) & NM_PROTO_ID_MASK;
      if(proto_id == NM_PROTO_SHORT_DATA)
	{
	  const struct nm_header_short_data_s*dh = (const struct nm_header_short_data_s*)p_proto;
	  const size_t data_len = dh->len;
	  assert(data_len == len);
	  memcpy(buf, dh + 1, data_len);
	}
      else if(proto_id == NM_PROTO_DATA)
	{
	  const struct nm_header_data_s*dh = (const struct nm_header_data_s*)p_proto;
	  const size_t data_len = dh->len;
	  assert(data_len == len);
	  memcpy(buf, dh + 1, data_len);
	}
      else if(proto_id == NM_PROTO_PKT_DATA)
	{
	  const struct nm_header_pkt_data_s*dh = (const struct nm_header_pkt_data_s*)p_proto;
	  const size_t data_len = dh->data_len;
	  assert(data_len == len);
	  struct nm_data_s data;
	  nm_data_contiguous_build(&data, buf, len);
	  nm_data_pkt_unpack(&data, dh, p_pw, 0, len);
	}
      else
	{
	  NM_FATAL("unexpected proto 0x%02x; len = %d\n", proto_id, (int)len);
	}
    }
  else
    {
      memcpy(buf, buf2, len);
      free(buf2);
    }
  nm_pw_free(p_pw);
}

/* *** eager, aggregation ********************************** */

static void nm_ns_eager_send_aggreg(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  const nm_len_t len1 = len / 2;
  const nm_len_t len2 = len - len1;
  const nm_core_tag_t tag = nm_core_tag_build(0, 0);
  const nm_seq_t seq = NM_SEQ_FIRST;
  struct nm_pkt_wrap_s*p_pw = NULL;
  if(len <= max_small)
    {
      struct nm_data_s data1;
      nm_data_contiguous_build(&data1, buf, len1);
      struct nm_data_s data2;
      nm_data_contiguous_build(&data2, buf + len1, len2);
      p_pw = nm_pw_alloc_global_header();
      nm_pw_add_data_in_header(p_pw, tag, seq, &data1, len1, 0, 0);
      nm_pw_add_data_in_header(p_pw, tag, seq, &data2, len2, len1, 0);
      p_pw->trk_id = NM_TRK_SMALL;
      nm_pw_finalize(p_pw);
      minidriver_send_iov(&p_trk_small->receptacle, p_pw->v, p_pw->v_nb);
      nm_pw_free(p_pw);
    }
}

static void nm_ns_eager_recv_aggreg(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  const nm_len_t max_small = nm_drv_max_small(p_trk_small->p_drv);
  struct nm_pkt_wrap_s*p_pw = NULL;
  nm_len_t done = 0;
  if(len <= max_small)
    {
      p_pw = nm_pw_alloc_buffer();
      minidriver_recv(&p_trk_small->receptacle, p_pw->v[0].iov_base, p_pw->v[0].iov_len);
      /* very rough header decoder... should be sufficient for our basic use here. */
      struct nm_header_global_s*p_global_header = p_pw->v[0].iov_base;
      void*ptr = p_global_header + 1;
      const nm_proto_t*p_proto = ptr;
      nm_proto_t proto;
      do
	{
	  proto = *p_proto;
	  const nm_proto_t proto_id = (*p_proto) & NM_PROTO_ID_MASK;
	  if(proto_id == NM_PROTO_SHORT_DATA)
	    {
	      const struct nm_header_short_data_s*dh = (const struct nm_header_short_data_s*)p_proto;
	      const size_t data_len = dh->len;
	      memcpy(buf + done, dh + 1, data_len);
	      p_proto += NM_HEADER_SHORT_DATA_SIZE + data_len;
	      done += data_len;
	    }
	  else if(proto_id == NM_PROTO_DATA)
	    {
	      const struct nm_header_data_s*dh = (const struct nm_header_data_s*)p_proto;
	      const size_t data_len = dh->len;
	      memcpy(buf + done, dh + (dh->skip == 0xFFFF ? 0 : dh->skip), data_len);
	      const size_t size = (proto & NM_PROTO_FLAG_ALIGNED) ? nm_aligned(data_len) : data_len;
	      p_proto += NM_HEADER_DATA_SIZE;
	      if(dh->skip == 0xFFFF)
		p_proto += size;
	      done += data_len;
	    }
	  else if(proto_id == NM_PROTO_PKT_DATA)
	    {
	      const struct nm_header_pkt_data_s*dh = (const struct nm_header_pkt_data_s*)p_proto;
	      const size_t data_len = dh->data_len;
	      struct nm_data_s data;
	      nm_data_contiguous_build(&data, buf, len);
	      nm_data_pkt_unpack(&data, dh, p_pw, done, data_len);
	      done += data_len;
	      p_proto += dh->hlen;
	    }
	  else
	    {
	      NM_FATAL("unexpected proto 0x%02x; len = %d\n", proto_id, (int)len);
	    }
	}
      while(done < len);
      nm_pw_free(p_pw);
    }
}


/* *** rdv send/recv on trk #1 ***************************** */

static void nm_ns_rdv_send(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  int flag = 1;
  minidriver_send(&p_trk_small->receptacle, &flag, sizeof(flag));
  minidriver_recv(&p_trk_small->receptacle, &flag, sizeof(flag));
  minidriver_send(&p_trk_large->receptacle, buf, len);
}

static void nm_ns_rdv_recv(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  int flag = 1;
  minidriver_recv(&p_trk_small->receptacle, &flag, sizeof(flag));
  minidriver_send(&p_trk_small->receptacle, &flag, sizeof(flag));
  minidriver_recv(&p_trk_large->receptacle, buf, len);
}

/* *** memcpy ********************************************** */

static struct
{
  char*buffer;
  size_t size;
} workspace = { .buffer = NULL, .size = 0};

static void nm_ns_copy_send(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  if(workspace.size < len)
    {
      workspace.buffer = realloc(workspace.buffer, len);
      workspace.size = len;
    }
  memcpy(workspace.buffer, buf, len);
}
static void nm_ns_copy_recv(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  if(workspace.size < len)
    {
      workspace.buffer = realloc(workspace.buffer, len);
      workspace.size = len;
    }
  memcpy(buf, workspace.buffer, len);
}

/* *** eager x2 ******************************************** */

static void nm_ns_eager_send_x2(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  nm_ns_eager_send_iov(p_trk_small, p_trk_large, buf, len / 2);
  nm_ns_eager_send_iov(p_trk_small, p_trk_large, buf, len / 2);
}
static void nm_ns_eager_recv_x2(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, void*buf, nm_len_t len)
{
  nm_ns_eager_recv(p_trk_small, p_trk_large, buf, len / 2);
  nm_ns_eager_recv(p_trk_small, p_trk_large, buf, len / 2);
}

/* *** pingpong ******************************************** */

static double nm_ns_ping_one(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, const struct nm_sample_bench_s*p_bench, size_t size)
{
  const int roundtrips = nm_ns_nb_samples(size);
  double*time_array = malloc(sizeof(double)*roundtrips);
  int i;
  for(i = 0; i < roundtrips; i++)
    {
      tbx_tick_t t1, t2;
      TBX_GET_TICK(t1);
      (*p_bench->send)(p_trk_small, p_trk_large, p_buffer_send, size);
      (*p_bench->recv)(p_trk_small, p_trk_large, p_buffer_recv, size);
      TBX_GET_TICK(t2);
      const double delay = TBX_TIMING_DELAY(t1, t2);
      const double t = delay / 2.0;
      time_array[i] = t;
    }
  double min = DBL_MAX;
  for(i = 0; i < roundtrips; i++)
    {
      if(time_array[i] < min)
	min = time_array[i];
    }
  free(time_array);
  return min;
}

static void nm_ns_pong_one(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, const struct nm_sample_bench_s*p_bench, size_t size)
{
  const int roundtrips = nm_ns_nb_samples(size);
  int i;
  for(i = 0; i < roundtrips; i++)
    {
      (*p_bench->recv)(p_trk_small, p_trk_large, p_buffer_recv, size);
      (*p_bench->send)(p_trk_small, p_trk_large, p_buffer_send, size);
    }
}

/* *** benches ********************************************* */

const static struct nm_sample_bench_s nm_sample_bench_drv =
  {
    .name = "raw driver trk#0",
    .send = &nm_ns_driver_send,
    .recv = &nm_ns_driver_recv
  };
const static struct nm_sample_bench_s nm_sample_bench_trk0_copy =
  {
    .name = "trk#0 protocol with copy",
    .send = &nm_ns_eager_send_copy,
    .recv = &nm_ns_eager_recv
  };
const static struct nm_sample_bench_s nm_sample_bench_trk1 =
  {
    .name = "trk#1 protocol",
    .send = &nm_ns_rdv_send,
    .recv = &nm_ns_rdv_recv
  };
const static struct nm_sample_bench_s nm_sample_bench_trk0_iov =
  {
    .name = "trk#0 protocol with iov",
    .send = &nm_ns_eager_send_iov,
    .recv = &nm_ns_eager_recv
  };
const static struct nm_sample_bench_s nm_sample_bench_memcpy =
  {
    .name = "memcpy",
    .send = &nm_ns_copy_send,
    .recv = &nm_ns_copy_recv
  };
const static struct nm_sample_bench_s nm_sample_bench_trk0_x2 =
  {
    .name = "trk#0 protocol split packets",
    .send = &nm_ns_eager_send_x2,
    .recv = &nm_ns_eager_recv_x2
  };
const static struct nm_sample_bench_s nm_sample_bench_trk0_aggreg =
  {
    .name = "trk#0 aggreg protocol",
    .send = &nm_ns_eager_send_aggreg,
    .recv = &nm_ns_eager_recv_aggreg
  };

static void nm_ns_ping(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large, FILE*sampling_file)
{
  int size = 0;

  for (size = param_min_size; size <= param_max_size; size*= 2)
    {
      const double lat             = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_drv, size);
      const double bw_million_byte = size / lat;
      const double bw_mbyte        = bw_million_byte / 1.048576;
      const double eager_lat       = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_copy, size);
      const double rdv_lat         = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_trk1, size);
      const double eager_iov_lat   = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_iov, size);
      const double memcpy_lat      = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_memcpy, size);
      const double eager_iov_split = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_x2, size);
      const double eager_iov_aggreg = nm_ns_ping_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_aggreg, size);

      fprintf(stderr, "%9d\t%9.3lf\t%9.3lf\t%9.3lf\t%9.3lf\t%9.3lf\t%9.3lf\t%9.3lf\t%9.3lf\n",
	      size, lat, bw_mbyte, eager_lat, rdv_lat, eager_iov_lat, memcpy_lat, eager_iov_split, eager_iov_aggreg);
      if(sampling_file)
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
}

static void nm_ns_pong(struct nm_trk_s*p_trk_small, struct nm_trk_s*p_trk_large)
{
  int size = 0;
  for (size = param_min_size; size <= param_max_size; size*= 2)
    {
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_drv, size);
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_copy, size);
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_trk1, size);
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_iov, size);
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_memcpy, size);
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_x2, size);
      nm_ns_pong_one(p_trk_small, p_trk_large, &nm_sample_bench_trk0_aggreg, size);

      size = (size==0? 1:size);
    }
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
  nm_launcher_session_open(&p_session, "sampling-prog");
  nm_launcher_get_rank(&rank);
  is_server = !rank;
  peer = 1 - rank;
  nm_launcher_get_gate(peer, &p_gate);

  assert(p_gate != NULL);
  assert(p_gate->status == NM_GATE_STATUS_CONNECTED);

  nm_core_schedopt_disable(p_gate->p_core);
  struct nm_trk_s*p_trk_small = nm_trk_get_by_index(p_gate, NM_TRK_SMALL);
  struct nm_trk_s*p_trk_large = nm_trk_get_by_index(p_gate, NM_TRK_LARGE);

  samples = nm_sample_vect_new();

  p_buffer_send = TBX_MALLOC(param_max_size);
  p_buffer_recv = TBX_MALLOC(param_max_size);
  nm_ns_fill_data(p_buffer_send);

  if(!is_server)
    {
      FILE*sampling_file = NULL;
      if(argv[1] != NULL)
	{
	  const char*filename = argv[1];
	  fprintf(stderr, "# sampling:  writing file %s\n", filename);
	  fprintf(stderr, "# size\t | raw lat.\t | raw bw \t | trk#0 lat.\t | trk#1 lat.\t | trk#0 iov\t | memcpy\t | 2 pkts \t | aggreg\n");
	  sampling_file = fopen(filename, "w");
	  if(sampling_file == NULL)
	    {
	      fprintf(stderr, "# ## sampling: cannot write file %s.\n", filename);
	      abort();
	    }
	}
      nm_ns_ping(p_trk_small, p_trk_large, sampling_file);
      if(sampling_file != NULL)
	fclose(sampling_file);
      nm_ns_compute_thresholds();
    }
  else
    {
      nm_ns_pong(p_trk_small, p_trk_large);
    }

  nm_launcher_session_close(p_session);
  nm_launcher_exit();

  TBX_FREE(p_buffer_send);
  TBX_FREE(p_buffer_recv);

  return 0;
}

