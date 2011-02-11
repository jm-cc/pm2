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

#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <nm_pack_interface.h>
#include <ccs_public.h>
#include <tbx.h>

static int is_server = -1;
static nm_session_t p_core = NULL;
static nm_gate_t gate_id = NULL;

void nmad_exit()
{
  nm_launcher_exit();
}

void init(int *argc, char **argv)
{
  int rank, peer, size;
  nm_launcher_init(argc, argv);
  nm_launcher_get_session(&p_core);
  nm_launcher_get_rank(&rank);
  nm_launcher_get_size(&size);
  is_server = (rank == 0);
  peer = (rank + 1) % size;
  nm_launcher_get_gate(peer, &gate_id);
}

