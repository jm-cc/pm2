/*
 * NewMadeleine
 * Copyright (C) 2011 (see AUTHORS file)
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
#include <values.h>

#include "sr_examples_helper.h"

#define POLL_LOOPS 100000

struct bench_ext_s
{
  const char*name;
  void (*server)(void);
  void (*client)(void);
};

/* ** poll latency ***************************************** */

static void bench_ext_poll(void)
{
  nm_sr_request_t*request = NULL;
  nm_sr_recv_success(p_session, &request);
}

const static struct bench_ext_s bench_poll = 
  { 
    .name = "poll latency",
    .server = &bench_ext_poll,
    .client = &bench_ext_poll
  };

/* ** sendrecv latency ************************************* */

static void bench_ext_sr_server(void)
{
  static int buf;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_irecv(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_rwait(p_session, &request);
  nm_sr_isend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
}

static void bench_ext_sr_client(void)
{
  static int buf = 0;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_isend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
  nm_sr_irecv(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_rwait(p_session, &request);
}

const static struct bench_ext_s bench_sr = 
  { 
    .name = "sendrecv latency",
    .server = &bench_ext_sr_server,
    .client = &bench_ext_sr_client
  };

/* ** sendrecv 0 byte latency ****************************** */

static void bench_ext_sr0_server(void)
{
  nm_sr_request_t request;
  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &request);
  nm_sr_rwait(p_session, &request);
  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &request);
  nm_sr_swait(p_session, &request);
}

static void bench_ext_sr0_client(void)
{
  nm_sr_request_t request;
  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &request);
  nm_sr_swait(p_session, &request);
  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &request);
  nm_sr_rwait(p_session, &request);
}

const static struct bench_ext_s bench_sr0 = 
  { 
    .name = "sendrecv 0 byte latency",
    .server = &bench_ext_sr0_server,
    .client = &bench_ext_sr0_client
  };

/* ** sendrecv anysrc latency ****************************** */

static void bench_ext_sranysrc_server(void)
{
  static int buf;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_irecv(p_session, NM_GATE_NONE, 0, &buf, len, &request);
  nm_sr_rwait(p_session, &request);
  nm_sr_isend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
}

static void bench_ext_sranysrc_client(void)
{
  static int buf = 0;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_isend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
  nm_sr_irecv(p_session, NM_GATE_NONE, 0, &buf, len, &request);
  nm_sr_rwait(p_session, &request);
}

const static struct bench_ext_s bench_sranysrc = 
  { 
    .name = "sendrecv anysrc latency",
    .server = &bench_ext_sranysrc_server,
    .client = &bench_ext_sranysrc_client
  };

/* ** sendrecv anytag latency ****************************** */

static void bench_ext_sranytag_server(void)
{
  static int buf;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_recv_init(p_session, &request);
  nm_sr_recv_unpack_contiguous(p_session, &request, &buf, len);
  nm_sr_recv_irecv(p_session, &request, p_gate, 0, NM_TAG_MASK_NONE);
  nm_sr_rwait(p_session, &request);
  nm_sr_isend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
}

static void bench_ext_sranytag_client(void)
{
  static int buf = 0;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_isend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
  nm_sr_recv_init(p_session, &request);
  nm_sr_recv_unpack_contiguous(p_session, &request, &buf, len);
  nm_sr_recv_irecv(p_session, &request, p_gate, 0, NM_TAG_MASK_NONE);
  nm_sr_rwait(p_session, &request);
}

const static struct bench_ext_s bench_sranytag = 
  { 
    .name = "sendrecv anytag latency",
    .server = &bench_ext_sranytag_server,
    .client = &bench_ext_sranytag_client
  };


/* ** sendrecv iov latency ********************************* */

static void bench_ext_sriov_server(void)
{
  static int buf[4];
  static struct iovec v[2] = { [0] = {.iov_base = &buf[0], .iov_len = sizeof(int) }, 
			       [1] = {.iov_base = &buf[1], .iov_len = sizeof(int) } };
  nm_sr_request_t request;
  nm_sr_irecv_iov(p_session, p_gate, 0, v, 2, &request);
  nm_sr_rwait(p_session, &request);
  nm_sr_isend_iov(p_session, p_gate, 0, v, 2, &request);
  nm_sr_swait(p_session, &request);
}

static void bench_ext_sriov_client(void)
{
  static int buf[4];
  static struct iovec v[2] = { [0] = {.iov_base = &buf[0], .iov_len = sizeof(int) }, 
			       [1] = {.iov_base = &buf[1], .iov_len = sizeof(int) } };
  nm_sr_request_t request;
  nm_sr_isend_iov(p_session, p_gate, 0, v, 2, &request);
  nm_sr_swait(p_session, &request);
  nm_sr_irecv_iov(p_session, p_gate, 0, v, 2, &request);
  nm_sr_rwait(p_session, &request);
}

const static struct bench_ext_s bench_sriov = 
  { 
    .name = "sendrecv iov latency",
    .server = &bench_ext_sriov_server,
    .client = &bench_ext_sriov_client
  };

/* ** ssend latency **************************************** */

static void bench_ext_ssend_server(void)
{
  static int buf;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_irecv(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_rwait(p_session, &request);
  nm_sr_issend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
}

static void bench_ext_ssend_client(void)
{
  static int buf = 0;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_issend(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_swait(p_session, &request);
  nm_sr_irecv(p_session, p_gate, 0, &buf, len, &request);
  nm_sr_rwait(p_session, &request);
}

const static struct bench_ext_s bench_ssend = 
  { 
    .name = "ssend latency",
    .server = &bench_ext_ssend_server,
    .client = &bench_ext_ssend_client
  };

/* ** recv_success latency ********************************* */

static void bench_ext_success_server(void)
{
  static int buf;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_request_t*p_request = NULL;

  nm_sr_recv_init(p_session, &request);
  nm_sr_recv_unpack_contiguous(p_session, &request, &buf, len);
  nm_sr_request_set_completion_queue(p_session, &request);
  nm_sr_recv_irecv(p_session, &request, p_gate, 0, NM_TAG_MASK_FULL);
  while(nm_sr_recv_success(p_session, &p_request) != -NM_ESUCCESS)
    {
    };

  nm_sr_send_init(p_session, &request);
  nm_sr_send_pack_contiguous(p_session, &request, &buf, len);
  nm_sr_request_set_completion_queue(p_session, &request);
  nm_sr_send_isend(p_session, &request, p_gate, 0);
  while(nm_sr_send_success(p_session, &p_request) != -NM_ESUCCESS)
    {
    };
}

static void bench_ext_success_client(void)
{
  static int buf = 0;
  static const int len = sizeof(buf);
  nm_sr_request_t request;
  nm_sr_request_t*p_request = NULL;

  nm_sr_send_init(p_session, &request);
  nm_sr_send_pack_contiguous(p_session, &request, &buf, len);
  nm_sr_request_set_completion_queue(p_session, &request);
  nm_sr_send_isend(p_session, &request, p_gate, 0);
  while(nm_sr_send_success(p_session, &p_request) != -NM_ESUCCESS)
    {
    };

  nm_sr_recv_init(p_session, &request);
  nm_sr_recv_unpack_contiguous(p_session, &request, &buf, len);
  nm_sr_request_set_completion_queue(p_session, &request);
  nm_sr_recv_irecv(p_session, &request, p_gate, 0, NM_TAG_MASK_FULL);
  while(nm_sr_recv_success(p_session, &p_request) != -NM_ESUCCESS)
    {
    };
}

const static struct bench_ext_s bench_success = 
  { 
    .name = "completion queue latency",
    .server = &bench_ext_success_server,
    .client = &bench_ext_success_client
  };


/* ********************************************************* */

static void bench_ext_run(const struct bench_ext_s*bench)
{
  int k;
  if(is_server) 
    {
      /* server */
      for(k = 0; k < POLL_LOOPS; k++)
	{
	  (*bench->server)();
	}
    }
  else
    {
      /* client */
      double min = DBL_MAX;
      tbx_tick_t t1, t2;
      for(k = 0; k < POLL_LOOPS; k++)
	{
	  TBX_GET_TICK(t1);
	  (*bench->client)();
	  TBX_GET_TICK(t2);
	  const double delay = TBX_TIMING_DELAY(t1, t2);
	  const double t = delay / 2.0;
	  if(t < min)
	    min = t;
	}
      printf("%32s: %9.3lf usec.\n", bench->name, min);
    }
}

int main(int argc, char**argv)
{
  nm_examples_init(&argc, argv);
  
  bench_ext_run(&bench_poll);
  bench_ext_run(&bench_sr);
  bench_ext_run(&bench_sranysrc);
  bench_ext_run(&bench_sranytag);
  bench_ext_run(&bench_sr0);
  bench_ext_run(&bench_sriov);
  bench_ext_run(&bench_ssend);
  bench_ext_run(&bench_success);
  
  nm_examples_exit();
  exit(0);
}

