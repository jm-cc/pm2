
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
extern int marcel_extlib_protect(void);
extern int marcel_extlib_unprotect(void);
#endif

/* The `malloc' family of functions is a particular example of extern library
   functions that must be "protected", i.e., must not be preempted, otherwise
   assumptions of the underlying may be violated and Bad Things may happen.
   The following macros are meant to handle these cases.

   On GNU systems, this is handled transparently using glibc's malloc hooks,
   so this macros are a no-op in this case.  On other systems, they have the
   same effect as `extlib_{protect,unprotect}'.  */

#ifdef MA__HAS_GNU_MALLOC_HOOKS
#  define marcel_malloc_protect()
#  define marcel_malloc_unprotect()
#else
#  define marcel_malloc_protect()    marcel_extlib_protect()
#  define marcel_malloc_unprotect()  marcel_extlib_unprotect()
#endif
