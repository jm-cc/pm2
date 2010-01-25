/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_LOCKING_H__
#define __MARCEL_LOCKING_H__


#include "sys/marcel_flags.h"


/** Public functions **/
/****************************************************************
 * À appeler autour de _tout_ appel à fonctions de librairies externes
 *
 * Cela désactive la préemption et les traitants de signaux
 */
extern int marcel_extlib_protect(void);
extern int marcel_extlib_unprotect(void);

/* The `malloc' family of functions is a particular example of extern library
   functions that must be "protected", i.e., must not be preempted, otherwise
   assumptions of the underlying may be violated and Bad Things may happen.
   The following macros are meant to handle these cases.  */

#define marcel_malloc_protect()    marcel_extlib_protect()
#define marcel_malloc_unprotect()  marcel_extlib_unprotect()


#endif /** __MARCEL_LOCKING_H__ **/
