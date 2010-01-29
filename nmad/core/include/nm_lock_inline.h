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
#ifndef NM_LOCK_INLINE
#define NM_LOCK_INLINE

#ifdef FINE_GRAIN_LOCKING
#  ifdef PIOMAN
#  define NM_LOCK_CORE
#  define NM_LOCK_POST
/** lock for try_and_commit */
extern piom_spinlock_t nm_tac_lock;
/** lock for status */
extern piom_spinlock_t nm_status_lock;
#  ifdef NMAD_POLL
#    define NM_LOCK_POLL
#  endif /* NMAD_POLL */
#  endif /* PIOMAN */
#else /* FINE_GRAIN_LOCKING */
#  ifdef PIOMAN
#    define NM_LOCK_BIGLOCK
extern piom_spinlock_t piom_big_lock;
#  endif
#endif /* FINE_GRAIN_LOCKING */


/** Lock entirely NewMadeleine */
static inline void nmad_lock(void)
{
#ifdef NM_LOCK_BIGLOCK
  piom_spin_lock(&piom_big_lock);
#endif
}

/** Try to lock NewMadeleine 
 * return 0 if NMad is already locked or 1 otherwise
 */
static inline int nmad_trylock(void)
{
#ifdef NM_LOCK_BIGLOCK
  return piom_spin_trylock(&piom_big_lock);
#else
  return 1;
#endif
}

/** Unlock NewMadeleine */
static inline void nmad_unlock(void)
{
#ifdef NM_LOCK_BIGLOCK
  piom_spin_unlock(&piom_big_lock);
#endif
}

static inline void nmad_lock_init(struct nm_core *p_core)
{
#ifdef NM_LOCK_BIGLOCK
  piom_spin_lock_init(&piom_big_lock);
#endif
}


/*
 * Lock the interface's list of packet to send
 * use this when entering try_and_commit or (un)pack()
 */

static inline void nm_lock_interface(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  piom_spin_lock(&nm_tac_lock);
#endif
}

static inline int nm_trylock_interface(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  return piom_spin_trylock(&nm_tac_lock);
#else
  return 1;
#endif
}

static inline void nm_unlock_interface(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  piom_spin_unlock(&nm_tac_lock);
#endif
}

static inline void nm_lock_interface_init(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  piom_spin_lock_init(&nm_tac_lock);
#endif
}


static inline void nm_lock_status(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  piom_spin_lock(&nm_status_lock);
#endif
}

static inline int nm_trylock_status(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  return piom_spin_trylock(&nm_status_lock);
#else
  return 1;
#endif
}

static inline void nm_unlock_status(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  piom_spin_unlock(&nm_status_lock);
#endif
}

static inline void nm_lock_status_init(struct nm_core *p_core)
{
#ifdef NM_LOCK_CORE
  piom_spin_lock_init(&nm_status_lock);
#endif
}


/*
 * Lock used to access post_sched_out_list 
 */

static inline void nm_so_lock_out(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  piom_spin_lock(&p_drv->post_sched_out_lock);
#endif
}
static inline int nm_so_trylock_out(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  return piom_spin_trylock(&p_drv->post_sched_out_lock);
#else
  return 1;
#endif
}
static inline void nm_so_unlock_out(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  piom_spin_unlock(&p_drv->post_sched_out_lock);
#endif
}
static inline void nm_so_lock_out_init(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  piom_spin_lock_init(&p_drv->post_sched_out_lock);
#endif
}

/*
 * Lock used to access post_recv_list
 */

static inline void nm_so_lock_in(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  piom_spin_lock(&p_drv->post_recv_lock);
#endif
}
static inline int nm_so_trylock_in(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  return piom_spin_trylock(&p_drv->post_recv_lock);
#else
  return 1;
#endif
}
static inline void nm_so_unlock_in(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  piom_spin_unlock(&p_drv->post_recv_lock);
#endif
}
static inline void nm_so_lock_in_init(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POST
  piom_spin_lock_init(&p_drv->post_recv_lock);
#endif
}

#ifdef NMAD_POLL

/*
 *   Lock used to access pending_recv_list 
 */

static inline void nm_poll_lock_in(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  piom_spin_lock(&p_drv->pending_recv_lock);
#endif
}
static inline int nm_poll_trylock_in(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  return piom_spin_trylock(&p_drv->pending_recv_lock);
#else
  return 1;
#endif
}
static inline void nm_poll_unlock_in(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  piom_spin_unlock(&p_drv->pending_recv_lock);
#endif
}
static inline void nm_poll_lock_in_init(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  piom_spin_lock_init(&p_drv->pending_recv_lock);
#endif
 }

/*
 * Lock used for accessing pending_send_list 
 */

static inline void nm_poll_lock_out(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  piom_spin_lock(&p_drv->pending_send_lock);
#endif
 }
static inline int nm_poll_trylock_out(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  return piom_spin_trylock(&p_drv->pending_send_lock);
#else
  return 1;
#endif
}
static inline void nm_poll_unlock_out(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  piom_spin_unlock(&p_drv->pending_send_lock);
#endif
}
static inline void nm_poll_lock_out_init(struct nm_core*p_core, struct nm_drv*p_drv)
{
#ifdef NM_LOCK_POLL
  piom_spin_lock_init(&p_drv->pending_send_lock);
#endif
}

#endif /* NMAD_POLL */


#endif /* NM_LOCK_INLINE */
