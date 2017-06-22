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

#ifndef NM_LAUNCHER_H
#define NM_LAUNCHER_H

#include <nm_launcher_interface.h>
#include <Padico/Puk.h>

/** @internal Component interface definition: 'NewMad_Launcher' */
struct newmad_launcher_driver_s
{
  /** initialize nmad, establishes connections */
  void         (*init)(void*_status, int*argc, char**argv, const char*label);
  /** get the number of nodes in the session */
  int          (*get_size)(void*_status);
  /** get process rank */
  int          (*get_rank)(void*_status);
  /** get the list of gates */
  void         (*get_gates)(void*_status, nm_gate_t*gates);
  /** abort the full session */
  void         (*abort)(void*_status);
};

/** @internal */
PUK_IFACE_TYPE(NewMad_Launcher, struct newmad_launcher_driver_s);

#endif /* NM_LAUNCHER_H */

