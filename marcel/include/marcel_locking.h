
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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


/****************************************************************
 * Pour la compatibilité avec marcel_lock_*
 */

#section macros
#define MARCEL_LOCK_INIT MA_SPIN_LOCK_UNLOCKED

#section types
#depend "linux_spinlock.h[types]"
typedef ma_spinlock_t marcel_lock_t;

#section marcel_macros
#define marcel_lock_acquire(l)    ma_spin_lock(l)
#define marcel_lock_tryacquire(l) ma_spin_trylock(l)
#define marcel_lock_release(l)    ma_spin_unlock(l)
#define marcel_lock_locked(l)     ma_spin_is_locked(l)

#section functions
/****************************************************************
 * À appeler autour de _tout_ appel à fonctions de librairies externes
 *
 * Cela désactive la préemption et les traitants de signaux
 */
#ifdef MA__LIBPTHREAD
#  define marcel_extlib_protect()
#  define marcel_extlib_unprotect()
#else
int marcel_extlib_protect(void);
int marcel_extlib_unprotect(void);
#endif

