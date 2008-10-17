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

#include <nm_public.h>


/* ** Component interface definition: 'NewMad_Launcher' */

struct newmad_launcher_driver_s
{

  void      (*init)(void*_status, int*argc, char**argv, const char*label);

  int       (*get_size)(void*_status);

  int       (*get_rank)(void*_status);

  nm_core_t (*get_core)(void*_status);

  void      (*get_gates)(void*_status, nm_gate_t*gates);
};

PUK_IFACE_TYPE(NewMad_Launcher, struct newmad_launcher_driver_s);


/* ** Front-end functions for easier component indirections */

/** Initializes everything. */
int nm_launcher_init(int *argc, char **argv);

/** Cleans session. Returns NM_ESUCCESS or EXIT_FAILURE. */
int nm_launcher_exit(void);

/** Returns process rank */
int nm_launcher_get_rank(int *);

/** Returns the number of nodes */
int nm_launcher_get_size(int *);

/** Returns the send/receive interface */
int nm_launcher_get_core(nm_core_t *p_core);

/** Returns the gate for the process dest */
int nm_launcher_get_gate(int dest, nm_gate_t *gate);


#endif /* NM_LAUNCHER_H */

