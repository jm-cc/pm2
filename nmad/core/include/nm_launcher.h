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

#ifndef NM_LAUNCHER_H
#define NM_LAUNCHER_H

#include <Padico/Puk.h>

#include <nm_public.h>
#include <nm_so_public.h>
#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>

struct newmad_launcher_driver_s
{

  void (*init)(void*_status, int*argc, char**argv, const char*label, int(*sched_load)(struct nm_sched_ops*));

  int  (*get_size)(void*_status);

  int  (*get_rank)(void*_status);

  struct nm_so_interface*(*get_so_sr_if)(void*_status);

  nm_so_pack_interface (*get_so_pack_if)(void*_status);

  void (*get_gate_ids)(void*_status, nm_gate_id_t*ids);
};

PUK_IFACE_TYPE(NewMad_Launcher, struct newmad_launcher_driver_s);

#endif /* NM_LAUNCHER_H */

