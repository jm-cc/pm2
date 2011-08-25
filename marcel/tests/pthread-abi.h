/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008, 2009 "the PM2 team" (see AUTHORS file)
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

/* Helper tool for tests that exercise Marcel's libpthread ABI compatibility
   layer.  */


#include <marcel.h>


#if defined(MARCEL_LIBPTHREAD)


#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __INTEL_COMPILER
#  define error(status, errnum, ...)					\
	do {								\
		fflush(stdout);						\
		fprintf(stderr, __VA_ARGS__);			\
		if (errnum)						\
			fprintf(stderr, ": %s\n", strerror(errnum));	\
		else							\
			fprintf(stderr, "\n");				\
		exit(status);						\
	} while (0)
#else
#  include <error.h>
#endif


/* Make sure we are actually using Marcel's `libpthread.so', loaded via
   `LD_PRELOAD' or some such.  */
static __inline__ void
assert_running_marcel (void)
{
  void *self, *sym;

  self = dlopen (NULL, RTLD_NOW);
  if (self)
    {
      sym = dlsym (self, "marcel_create");
      dlclose (self);

      if (sym)
	return;
    }

  fprintf (stderr, "error: not using Marcel's libpthread (LD_PRELOAD=\"%s\")\n",
	   getenv ("LD_PRELOAD"));
  abort ();
}


#endif
