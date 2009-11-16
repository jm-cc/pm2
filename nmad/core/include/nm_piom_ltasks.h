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
#ifdef PIOM_ENABLE_LTASKS

#include "nm_private.h"

/* ltask dedicated to polling a recv request  */
int nm_poll_recv_task(void *args);

/* ltask dedicated to polling a send request  */
int nm_poll_send_task(void *args);

/* ltask dedicated to posting a recv request  
 * (not yet implemented!)
 */
int nm_post_recv_task(void *args);

/* ltask dedicated to posting a send request  
 * (not yet implemented!)
 */
int nm_post_send_task(void *args);

/* ltask dedicated to posting  requests */
int nm_post_task(void *args);

/* ltask dedicated to offloading a request submission */
int nm_offload_task(void* args);

/* submit new ltasks */
void nm_submit_poll_recv_ltask(struct nm_pkt_wrap *p_pw);
void nm_submit_poll_send_ltask(struct nm_pkt_wrap *p_pw);
void nm_submit_post_ltask(struct piom_ltask *task, struct nm_core *p_core);
void nm_submit_offload_ltask(struct piom_ltask *task, struct nm_pkt_wrap *p_pw);
#endif /* PIOM_ENABLE_LTASKS */

#endif /* PIOMAN_POLL */

#endif /* NM_PIOM_LTASKS_H */
