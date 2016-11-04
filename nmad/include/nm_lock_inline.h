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
#  endif /* PIOMAN */
#else /* FINE_GRAIN_LOCKING */
#  ifdef PIOMAN
#    define NM_LOCK_BIGLOCK
extern piom_spinlock_t piom_big_lock;
#    ifdef DEBUG
extern volatile piom_thread_t piom_big_lock_holder;
#    endif /* DEBUG */
#  endif
#endif /* FINE_GRAIN_LOCKING */


/** Lock entirely NewMadeleine */
static inline void nmad_lock(void)
{
#ifdef NM_LOCK_BIGLOCK
#ifdef DEBUG
  if(PIOM_THREAD_SELF == piom_big_lock_holder)
    {
      fprintf(stderr, "nmad: FATAL- deadlock detected. Self already holds spinlock.\n");
      abort();
    }
#endif
  int rc = piom_spin_lock(&piom_big_lock);
  if(rc != 0)
    NM_FATAL("cannot get spinlock- rc = %d\n", rc);
#ifdef DEBUG
  piom_big_lock_holder = PIOM_THREAD_SELF;
#endif
#endif
}

/** Try to lock NewMadeleine 
 * return 0 if NMad is already locked or 1 otherwise
 */
static inline int nmad_trylock(void)
{
#ifdef NM_LOCK_BIGLOCK
  int rc = piom_spin_trylock(&piom_big_lock);
#ifdef DEBUG
  if(rc == 1)
    piom_big_lock_holder = PIOM_THREAD_SELF;
#endif
  return rc;
#else
  return 1;
#endif
}

/** Unlock NewMadeleine */
static inline void nmad_unlock(void)
{
#ifdef NM_LOCK_BIGLOCK
#ifdef DEBUG
  assert(piom_big_lock_holder == PIOM_THREAD_SELF);
  piom_big_lock_holder = PIOM_THREAD_NULL;
#endif
  int rc = piom_spin_unlock(&piom_big_lock);
  if(rc != 0)
    NM_FATAL("error in spin_unlock- rc = %d\n", rc);
#endif
}

static inline void nmad_lock_init(struct nm_core *p_core)
{
#ifdef NM_LOCK_BIGLOCK
  piom_spin_init(&piom_big_lock);
#endif
}

/** assert that current thread holds the lock */
static inline void nmad_lock_assert(void)
{
#ifdef NM_LOCK_BIGLOCK
#ifdef DEBUG
  assert(PIOM_THREAD_SELF == piom_big_lock_holder);
#endif
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
  piom_spin_init(&nm_tac_lock);
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
  piom_spin_init(&nm_status_lock);
#endif
}


#endif /* NM_LOCK_INLINE */
