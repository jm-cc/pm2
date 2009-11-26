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

typedef struct nm_session_s*nm_session_t;

/** Create an empty session object.
 */
int nm_session_create(nm_session_t*pp_session, const char*label);

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


#endif /* NM_SESSION_INTERFACE_H */
