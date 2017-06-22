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

#include <nm_public.h>
#include <nm_session_interface.h>
#include <Padico/Puk.h>

/** @defgroup launcher_interface Launcher interface
 * This is the launcher interface, the high level nmad interface used to launch sessions.
 * @{
 */


/* ** Front-end functions for easier component indirections */

/** Initializes nmad. */
static inline int nm_launcher_init(int*argc, char**argv);

/** Cleans session. Returns NM_ESUCCESS or EXIT_FAILURE. */
int nm_launcher_exit(void);

/** Returns process rank */
int nm_launcher_get_rank(int*);

/** Returns the number of nodes */
int nm_launcher_get_size(int*);

/** Open a new session */
int nm_launcher_session_open(nm_session_t*p_session, const char*label);

/** close a session */
int nm_launcher_session_close(nm_session_t p_session);

/** Returns the gate for the process dest */
int nm_launcher_get_gate(int dest, nm_gate_t*gate);

/** Returns the dest rank for the given gate */
int nm_launcher_get_dest(nm_gate_t p_gate, int*dest);

/** Abort all processes */
void nm_launcher_abort(void);

/* @} */


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

/** @internal init launcher with ABI check */
int nm_launcher_init_checked(int*argc, char**argv, const struct nm_abi_config_s*p_nm_abi_config);

static inline int nm_launcher_init(int*argc, char**argv)
{
  /* capture ABI config in application context 
   * and compare with nmad builtin ABI config */
  return nm_launcher_init_checked(argc, argv, &nm_abi_config);
}


#endif /* NM_LAUNCHER_H */

