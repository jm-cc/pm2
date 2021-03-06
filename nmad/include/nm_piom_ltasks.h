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

#ifndef NM_PIOM_LTASKS_H
#define NM_PIOM_LTASKS_H

#ifdef PIOMAN

#include "nm_private.h"

/** apply the PIOMan binding policy (from env variable) 
 */
void nm_ltask_set_policy(void);

/** submit an ltask for a recv pw
 */
void nm_ltask_submit_pw_recv(struct nm_pkt_wrap_s *p_pw);

/** submit an ltask for a send pw
 */
void nm_ltask_submit_pw_send(struct nm_pkt_wrap_s*p_pw);

/** submit an ltask to make progress in core tasks.
 */
void nm_ltask_submit_core_progress(struct nm_core*p_core);

/** submit an ltask to offload a pw processing (not implemented yet).
 */
void nm_ltask_submit_offload(struct piom_ltask *task, struct nm_pkt_wrap_s *p_pw);


#endif /* PIOMAN */

#endif /* NM_PIOM_LTASKS_H */
