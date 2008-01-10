
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
#include <sys/types.h>
#include <signal.h>

#define PIOM_CONSTRAINT_ERROR "PIOM_CONSTRAINT_ERROR"
#define PIOM_TASKING_ERROR   "PIOM_TASKING_ERROR: A non-handled exception occurred in a task"
#define PIOM_DEADLOCK_ERROR   "PIOM_DEADLOCK_ERROR: Global Blocking Situation Detected"
#define PIOM_STORAGE_ERROR   "PIOM_STORAGE_ERROR: No space left on the heap"
#define PIOM_CONSTRAINT_ERROR   "PIOM_CONSTRAINT_ERROR"
#define PIOM_PROGRAM_ERROR   "PIOM_PROGRAM_ERROR"
#define PIOM_STACK_ERROR "PIOM_STACK_ERROR: Stack Overflow"
#define PIOM_TIME_OUT "TIME OUT while being blocked on a semaphor"
#define PIOM_NOT_IMPLEMENTED "PIOM_NOT_IMPLEMENTED (sorry)"
#define PIOM_USE_ERROR "PIOM_USE_ERROR: XPaulette was not compiled to enable this functionality"
#define PIOM_CALLBACK_ERROR "PIOM_CALLBACK_ERROR: no callback defined"
#define PIOM_LWP_ERROR "PIOM_LWP_ERROR: no lwp available"

#ifdef PM2_OPT
#define PIOM_BUG_ON(cond)  (void) (0)
#define PIOM_WARN_ON(cond) (void) (0)
#else

#define PIOM_BUG_ON(cond) \
  do { \
       if (cond) { \
         fprintf(stderr,"BUG on '" #cond "' at %s:%u\n", __FILE__, __LINE__); \
         *(int*)0=0;\
       } \
  } while (0)
#define PIOM_WARN_ON(cond) \
  do { \
	if (cond) { \
		fprintf(stderr,"%s:%u:Warning on '" #cond "'\n", __FILE__, __LINE__); \
	} \
  } while (0)

#endif

