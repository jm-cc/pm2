/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 * NewMadeleine
 * Copyright (C) 2009-2017 (see AUTHORS file)
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

/** acquire the nm core lock */
static inline void nm_core_lock(struct nm_core*p_core);

/** Try to lock the nmad core
 * return 0 if NMad is already locked or 1 otherwise
 */
static inline int  nm_core_trylock(struct nm_core*p_core);

/** unlock the nm core lock */
static inline void nm_core_unlock(struct nm_core*p_core);

/** init the core lock */
static inline void nm_core_lock_init(struct nm_core*p_core);

/** destroy the core lock */
static inline void nm_core_lock_destroy(struct nm_core*p_core);

#endif	/* NM_LOCK_H */
