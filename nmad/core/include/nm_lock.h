/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
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

#ifndef NM_LOCK_H
#define NM_LOCK_H

/* Lock entirely NewMadeleine */
static inline void nmad_lock(void);

/* Try to lock NewMadeleine 
 * return 0 if NMad is already locked or 1 otherwise
 */
static inline int  nmad_trylock(void);

/* Unlock NewMadeleine */
static inline void nmad_unlock(void);

static inline void nmad_lock_init(struct nm_core *p_core);

/* Lock the interface's list of packet to send
 * use this when entering try_and_commit or (un)pack()
 */
static inline void nm_lock_interface(struct nm_core *p_core);
static inline int  nm_trylock_interface(struct nm_core *p_core);
static inline void nm_unlock_interface(struct nm_core *p_core);
static inline void nm_lock_interface_init(struct nm_core *p_core);

static inline void nm_lock_status(struct nm_core *p_core);
static inline int  nm_trylock_status(struct nm_core *p_core);
static inline void nm_unlock_status(struct nm_core *p_core);
static inline void nm_lock_status_init(struct nm_core *p_core);

/* Lock used to access post_sched_out_list */
static inline void nm_so_lock_out(struct nm_so_sched* p_sched, unsigned drv_id);
static inline int  nm_so_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_so_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_so_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id);


/* Lock used to access post_recv_list */
static inline void nm_so_lock_in(struct nm_so_sched* p_sched, unsigned drv_id);
static inline int  nm_so_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_so_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_so_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id);


/* Lock used to access pending_recv_list */
static inline void nm_poll_lock_in(struct nm_so_sched* p_sched, unsigned drv_id);
static inline int  nm_poll_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_poll_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_poll_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id);


/* Lock used to access pending_send_list */
static inline void nm_poll_lock_out(struct nm_so_sched* p_sched, unsigned drv_id);
static inline int  nm_poll_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_poll_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id);
static inline void nm_poll_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id);


#endif	/* NM_LOCK_H */
