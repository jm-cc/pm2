
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

#define XPAUL_CONSTRAINT_ERROR "XPAUL_CONSTRAINT_ERROR"
#define XPAUL_TASKING_ERROR   "XPAUL_TASKING_ERROR: A non-handled exception occurred in a task"
#define XPAUL_DEADLOCK_ERROR   "XPAUL_DEADLOCK_ERROR: Global Blocking Situation Detected"
#define XPAUL_STORAGE_ERROR   "XPAUL_STORAGE_ERROR: No space left on the heap"
#define XPAUL_CONSTRAINT_ERROR   "XPAUL_CONSTRAINT_ERROR"
#define XPAUL_PROGRAM_ERROR   "XPAUL_PROGRAM_ERROR"
#define XPAUL_STACK_ERROR "XPAUL_STACK_ERROR: Stack Overflow"
#define XPAUL_TIME_OUT "TIME OUT while being blocked on a semaphor"
#define XPAUL_NOT_IMPLEMENTED "XPAUL_NOT_IMPLEMENTED (sorry)"
#define XPAUL_USE_ERROR "XPAUL_USE_ERROR: XPaulette was not compiled to enable this functionality"
#define XPAUL_CALLBACK_ERROR "XPAUL_CALLBACK_ERROR: no callback defined"
#define XPAUL_LWP_ERROR "XPAUL_LWP_ERROR: no lwp available"

#ifdef PM2_OPT
#define XPAUL_BUG_ON(cond)  (void) (0)
#define XPAUL_WARN_ON(cond) (void) (0)
#else

#define XPAUL_BUG_ON(cond) \
  do { \
       if (cond) { \
         fprintf(stderr,"BUG on '" #cond "' at %s:%u\n", __FILE__, __LINE__); \
         kill(getpid(),SIGABRT); \
       } \
  } while (0)
#define XPAUL_WARN_ON(cond) \
  do { \
	if (cond) { \
		fprintf(stderr,"%s:%u:Warning on '" #cond "'\n", __FILE__, __LINE__); \
	} \
  } while (0)

#endif

