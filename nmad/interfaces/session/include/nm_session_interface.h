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

#ifndef NM_SESSION_INTERFACE_H
#define NM_SESSION_INTERFACE_H

#include <nm_public.h>
#include <Padico/Puk.h>

typedef struct nm_session_s*nm_session_t;

/** Create an empty session object.
 */
int nm_session_create(nm_session_t*pp_session, const char*label);


/** Add a driver to the session.
 * @note call to this function is optional. Default drivers will be loaded
 * if no driver is loaded manually.
 */
#warning TODO- include index as a NewMad_Driver component attribute
void nm_session_add_driver(puk_component_t component, int index);

/** Initializes a session object.
 * 'p_local_url' is a return parameter, pointing to an object allocated in the session- do not free nor modify!
 */
int nm_session_init(nm_session_t p_session, int*argc, char**argv, const char**p_local_url);

/** Connect the given gate.
 * 'remote_url' is the url returned by the nm_session_init on the given peer.
 * @note this function is synchronous; the remote node must call this function
 * at the same time with our url
 */
int nm_session_connect(nm_session_t p_session, nm_gate_t*pp_gate, const char*remote_url);

/** Disconnect and destroy a session.
 */
int nm_session_destroy(nm_session_t p_session);


/* ** Notes on driver selection.
 *
 * The 'session' interface choses drivers automatically. To force 
 * a given driver, use the NMAD_DRIVER environment variable with 
 * a driver string description, or the NMAD_ASSEMBLY variable with
 * a component name.
 *
 * TODO- the 'session' interface should use Padico NetSelector instead.
 */

#endif /* NM_SESSION_INTERFACE_H */
