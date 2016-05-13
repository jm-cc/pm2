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

#ifdef PIOMAN_POLL

#include "nm_private.h"

/** apply the PIOMan binding policy (from env variable) 
 */
void nm_ltask_set_policy(void);

/** submit an ltask for a recv pw
 */
void nm_ltask_submit_poll_recv(struct nm_pkt_wrap *p_pw);

/** submit an ltask for a send pw
 */
void nm_ltask_submit_poll_send(struct nm_pkt_wrap *p_pw);

/** submit an ltask to post requests on a given driver (both send and recv).
 */
void nm_ltask_submit_post_drv(nm_drv_t p_drv);

/** submit an ltask to offload a pw processing (not implemented yet).
 */
void nm_ltask_submit_offload(struct piom_ltask *task, struct nm_pkt_wrap *p_pw);


#endif /* PIOMAN_POLL */

#endif /* NM_PIOM_LTASKS_H */
