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

#include <nm_private.h>

#ifndef FINE_GRAIN_LOCKING
#ifdef PIOMAN
extern piom_spinlock_t piom_big_lock;
#endif

/**
 * Lock entirely NewMadeleine 
 */
static inline void nmad_lock(void)
{
#ifdef PIOMAN
  piom_spin_lock(&piom_big_lock);
#endif
}

/**
 * Try to lock NewMadeleine 
 * return 0 if NMad is already locked or 1 otherwise
 */
static inline int nmad_trylock(void)
{
#ifdef PIOMAN
  return piom_spin_trylock(&piom_big_lock);
#else
  return 1;
#endif
}

/**
 * Unlock NewMadeleine 
 */
static inline void nmad_unlock(void)
{
#ifdef PIOMAN
  piom_spin_unlock(&piom_big_lock);
#endif
}

static inline void nmad_lock_init(struct nm_core *p_core)
{
#ifdef PIOMAN
  piom_spin_lock_init(&piom_big_lock);
#endif
}


static inline void nm_lock_interface(struct nm_core *p_core)                           { }
static inline int nm_trylock_interface(struct nm_core *p_core)                         { return 1; }
static inline void nm_unlock_interface(struct nm_core *p_core)                         { }
static inline void nm_lock_interface_init(struct nm_core *p_core)                      { }
static inline void nm_lock_status(struct nm_core *p_core)                              { }
static inline int nm_trylock_status(struct nm_core *p_core)                            { return 1; }
static inline void nm_unlock_status(struct nm_core *p_core)                            { }
static inline void nm_lock_status_init(struct nm_core *p_core)                         { }
static inline void nm_so_lock_out(struct nm_so_sched* p_sched, unsigned drv_id)        { }
static inline int nm_so_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id)      { return 1; }
static inline void nm_so_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id)      { }
static inline void nm_so_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id)   { }
static inline void nm_so_lock_in(struct nm_so_sched* p_sched, unsigned drv_id)         { }
static inline int nm_so_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id)       { return 1; }
static inline void nm_so_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id)       { }
static inline void nm_so_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id)    { }
static inline void nm_poll_lock_in(struct nm_so_sched* p_sched, unsigned drv_id)       { }
static inline int nm_poll_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id)     { return 1; }
static inline void nm_poll_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id)     { }
static inline void nm_poll_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id)  { }
static inline void nm_poll_lock_out(struct nm_so_sched* p_sched, unsigned drv_id)      { }
static inline int nm_poll_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id)    { return 1; }
static inline void nm_poll_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id)    { }
static inline void nm_poll_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id) { }

#else /* FINE_GRAIN_LOCKING */

static inline void nmad_lock(void)                        { }
static inline int  nmad_trylock(void)                     { return 1; }
static inline void nmad_unlock(void)                      { }
static inline void nmad_lock_init(struct nm_core *p_core) { }

/**
 *  Lock to acces try_and_commit 
 */
#ifdef PIOMAN
extern piom_spinlock_t nm_tac_lock;
extern piom_spinlock_t nm_status_lock;
#endif

/**
 * Lock the interface's list of packet to send
 * use this when entering try_and_commit or (un)pack()
 */
static inline void nm_lock_interface(struct nm_core *p_core)
{
#if PIOMAN
  piom_spin_lock(&nm_tac_lock);
#endif
}

static inline int nm_trylock_interface(struct nm_core *p_core)
{
#ifdef PIOMAN
  return piom_spin_trylock(&nm_tac_lock);
#else
  return 1;
#endif
}

static inline void nm_unlock_interface(struct nm_core *p_core)
{
#ifdef PIOMAN
  piom_spin_unlock(&nm_tac_lock);
#endif
}

static inline void nm_lock_interface_init(struct nm_core *p_core)
{
#ifdef PIOMAN
  piom_spin_lock_init(&nm_tac_lock);
#endif
}


static inline void nm_lock_status(struct nm_core *p_core)
{
#if PIOMAN
  piom_spin_lock(&nm_status_lock);
#endif
}

static inline int nm_trylock_status(struct nm_core *p_core)
{
#ifdef PIOMAN
  return piom_spin_trylock(&nm_status_lock);
#else
  return 1;
#endif
}

static inline void nm_unlock_status(struct nm_core *p_core)
{
#ifdef PIOMAN
  piom_spin_unlock(&nm_status_lock);
#endif
}

static inline void nm_lock_status_init(struct nm_core *p_core)
{
#ifdef PIOMAN
  piom_spin_lock_init(&nm_status_lock);
#endif
}



/**
 * Lock used to access post_sched_out_list 
 */

static inline void nm_so_lock_out(struct nm_so_sched* p_sched, unsigned drv_id)
{ 
#ifdef PIOMAN
  piom_spin_lock(&p_sched->post_sched_out_lock[drv_id]);
#endif
}

static inline int nm_so_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id)
{ 
#ifdef PIOMAN
  return piom_spin_trylock(&p_sched->post_sched_out_lock[drv_id]);
#else
  return 1;
#endif
}

static inline void nm_so_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_unlock(&p_sched->post_sched_out_lock[drv_id]);
#endif
}

static inline void nm_so_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id)
{ 
#ifdef PIOMAN
  piom_spin_lock_init(&p_sched->post_sched_out_lock[drv_id]);
#endif
}



/**
 * Lock used to access post_sched_in_list 
 */

static inline void nm_so_lock_in(struct nm_so_sched* p_sched, unsigned drv_id)
{ 
#ifdef PIOMAN
  piom_spin_lock(&p_sched->post_sched_in_lock[drv_id]);
#endif
}

static inline int nm_so_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id)
{ 
#ifdef PIOMAN
  return piom_spin_trylock(&p_sched->post_sched_in_lock[drv_id]);
#else
  return 1;
#endif
}

static inline void nm_so_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id)
{ 
#ifdef PIOMAN
  piom_spin_unlock(&p_sched->post_sched_in_lock[drv_id]);
#endif
}

static inline void nm_so_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN 
  piom_spin_lock_init(&p_sched->post_sched_in_lock[drv_id]);
#endif
}


#ifdef NMAD_POLL

/**
 *   Lock used to access pending_recv_list 
 */

static inline void nm_poll_lock_in(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_lock(&p_sched->pending_recv_lock[drv_id]);
#endif
}

static inline int nm_poll_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  return piom_spin_trylock(&p_sched->pending_recv_lock[drv_id]);
#else
  return 1;
#endif
}

static inline void nm_poll_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_unlock(&p_sched->pending_recv_lock[drv_id]);
#endif
}

static inline void nm_poll_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_lock_init(&p_sched->pending_recv_lock[drv_id]);
#endif
}


/**
 * Lock used for accessing pending_send_list 
 */

static inline void nm_poll_lock_out(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_lock(&p_sched->pending_send_lock[drv_id]);
#endif
}

static inline int nm_poll_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  return piom_spin_trylock(&p_sched->pending_send_lock[drv_id]);
#else
  return 1;
#endif
}

static inline void nm_poll_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_unlock(&p_sched->pending_send_lock[drv_id]);
#endif
}

static inline void nm_poll_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id)
{
#ifdef PIOMAN
  piom_spin_lock_init(&p_sched->pending_send_lock[drv_id]);
#endif
}
#else  /* NMAD_POLL */

static inline void nm_poll_lock_in(struct nm_so_sched* p_sched, unsigned drv_id)       { }
static inline int  nm_poll_trylock_in(struct nm_so_sched* p_sched, unsigned drv_id)    { return 1;}
static inline void nm_poll_unlock_in(struct nm_so_sched* p_sched, unsigned drv_id)     { }
static inline void nm_poll_lock_in_init(struct nm_so_sched* p_sched, unsigned drv_id)  { }
static inline void nm_poll_lock_out(struct nm_so_sched* p_sched, unsigned drv_id)      { }
static inline int  nm_poll_trylock_out(struct nm_so_sched* p_sched, unsigned drv_id)   { return 1;}
static inline void nm_poll_unlock_out(struct nm_so_sched* p_sched, unsigned drv_id)    { }
static inline void nm_poll_lock_out_init(struct nm_so_sched* p_sched, unsigned drv_id) { }

#endif	/* NMAD_POLL */

#endif /* FINE_GRAIN_LOCKING */

#endif /* NM_LOCK_INLINE */
