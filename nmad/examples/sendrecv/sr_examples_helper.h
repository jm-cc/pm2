/*
 * NewMadeleine
 * Copyright (C) 2006-2015 (see AUTHORS file)
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

#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <tbx.h>
#include <nm_coll.h>


/* ********************************************************* */

static int is_server = -1;
static nm_session_t p_session = NULL;
static nm_gate_t p_gate = NULL;
static nm_comm_t p_comm = NULL;

static const nm_tag_t data_tag = 0x01;
static const nm_tag_t sync_tag = 0x02;

enum nm_example_topo_e
  {
    NM_EXAMPLES_TOPO_RING,
    NM_EXAMPLES_TOPO_PAIRS
  };

static void nm_examples_init_topo(int*argc, char*argv[], enum nm_example_topo_e topo)
{
  int rank, size, peer;
  nm_launcher_init(argc, argv);
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  nm_launcher_get_size(&size);
  switch(topo)
    {
    case NM_EXAMPLES_TOPO_RING:
      is_server = (rank == 0);
      peer = (rank + 1) % size;
      break;
    case NM_EXAMPLES_TOPO_PAIRS:
      assert(size % 2 == 0);
      is_server = ((rank % 2) == 0);
      peer = is_server ? (rank + 1) : (rank - 1);
      break;
    }
  nm_launcher_get_gate(peer, &p_gate);
}

static inline void nm_examples_init(int*argc, char*argv[])
{
  nm_examples_init_topo(argc, argv, NM_EXAMPLES_TOPO_RING);
}

static inline void nm_examples_exit(void)
{
  nm_launcher_exit();
}

/* ********************************************************* */

/* barrier accross all nodes */
static inline void nm_examples_barrier(nm_tag_t tag)
{
  if(p_comm == NULL)
    {
      p_comm = nm_comm_dup(nm_comm_world());
    }
  nm_coll_barrier(p_comm, tag);
}

/* ********************************************************* */

/* ** helpers for size iterators */

static inline nm_len_t _next(nm_len_t len, double multiplier, nm_len_t increment)
{
  nm_len_t next = len * multiplier + increment;
  if(next <= len)
    next++;
  return next;
}

static void fill_buffer(char*buffer, nm_len_t len) __attribute__((unused));
static void clear_buffer(char*buffer, nm_len_t len) __attribute__((unused));
static void control_buffer(const char*buffer, nm_len_t len) __attribute__((unused));


static inline char buffer_content(nm_len_t offset)
{
  return 'a' + (offset % 26);
}

static void fill_buffer(char*buffer, nm_len_t len)
{
  nm_len_t i = 0;
  for(i = 0; i < len; i++)
    {
      buffer[i] = buffer_content(i);
    }
}

static void clear_buffer(char*buffer, nm_len_t len)
{
  memset(buffer, 0, len);
}

static void control_buffer(const char*buffer, nm_len_t len)
{
  int i = 0;
  for(i = 0; i < len; i++)
    {
      const char expected = buffer_content(i);
      if(buffer[i] != expected)
	{
	  fprintf(stderr, "Bad data at byte %d: expected %c, received %c\n",
		  i, expected, buffer[i]);
	  abort();
	}
    }
}
