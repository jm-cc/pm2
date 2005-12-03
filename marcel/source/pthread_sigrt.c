
/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* Thread creation, initialization, and basic low-level routines */

#ifdef MA__LIBPTHREAD
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
//VD:#include <shlib-compat.h>
#include "pthread.h"
#include "sys/marcel_flags.h"
//VD:#include "internals.h"
//VD:#include "spinlock.h"
//VD:#include "restart.h"
//VD:#include "smp.h"
//VD:#include <ldsodefs.h>
//VD:#include <tls.h>
//VD:#include <locale.h>		/* for __uselocale */
//VD:#include <version.h>

#define __ASSUME_REALTIME_SIGNALS 1 //VD:
/* Sanity check.  */
#if __ASSUME_REALTIME_SIGNALS && !defined __SIGRTMIN
# error "This must not happen; new kernel assumed but old headers"
#endif

/* Signal numbers used for the communication.
   In these variables we keep track of the used variables.  If the
   platform does not support any real-time signals we will define the
   values to some unreasonable value which will signal failing of all
   the functions below.  */
#ifndef __SIGRTMIN
static int current_rtmin = -1;
static int current_rtmax = -1;
int __pthread_sig_restart = SIGUSR1;
int __pthread_sig_cancel = SIGUSR2;
int __pthread_sig_debug;
#else
static int current_rtmin;
static int current_rtmax;

#if __SIGRTMAX - __SIGRTMIN >= 3
int __pthread_sig_restart = __SIGRTMIN;
int __pthread_sig_cancel = __SIGRTMIN + 1;
int __pthread_sig_debug = __SIGRTMIN + 2;
#else
int __pthread_sig_restart = SIGUSR1;
int __pthread_sig_cancel = SIGUSR2;
int __pthread_sig_debug;
#endif

static int rtsigs_initialized;

#if !__ASSUME_REALTIME_SIGNALS
# include "testrtsig.h"
#endif

static void
init_rtsigs (void)
{
#if !__ASSUME_REALTIME_SIGNALS
  if (__builtin_expect (!kernel_has_rtsig (), 0))
    {
      current_rtmin = -1;
      current_rtmax = -1;
# if __SIGRTMAX - __SIGRTMIN >= 3
      __pthread_sig_restart = SIGUSR1;
      __pthread_sig_cancel = SIGUSR2;
      __pthread_sig_debug = 0;
# endif
    }
  else
#endif	/* __ASSUME_REALTIME_SIGNALS */
    {
#if __SIGRTMAX - __SIGRTMIN >= 3
      current_rtmin = __SIGRTMIN + 3;
# if !__ASSUME_REALTIME_SIGNALS
      __pthread_restart = __pthread_restart_new;
      __pthread_suspend = __pthread_wait_for_restart_signal;
      __pthread_timedsuspend = __pthread_timedsuspend_new;
# endif /* __ASSUME_REALTIME_SIGNALS */
#else
      current_rtmin = __SIGRTMIN;
#endif

      current_rtmax = __SIGRTMAX;
    }

  rtsigs_initialized = 1;
}
#endif

/* Return number of available real-time signal with highest priority.  */
int
__libc_current_sigrtmin (void)
{
#ifdef __SIGRTMIN
  if (__builtin_expect (!rtsigs_initialized, 0))
    init_rtsigs ();
#endif
  return current_rtmin;
}

/* Return number of available real-time signal with lowest priority.  */
int
__libc_current_sigrtmax (void)
{
#ifdef __SIGRTMIN
  if (__builtin_expect (!rtsigs_initialized, 0))
    init_rtsigs ();
#endif
  return current_rtmax;
}

/* Allocate real-time signal with highest/lowest available
   priority.  Please note that we don't use a lock since we assume
   this function to be called at program start.  */
int
__libc_allocate_rtsig (int high)
{
#ifndef __SIGRTMIN
  return -1;
#else
  if (__builtin_expect (!rtsigs_initialized, 0))
    init_rtsigs ();
  if (__builtin_expect (current_rtmin == -1, 0)
      || __builtin_expect (current_rtmin > current_rtmax, 0))
    /* We don't have anymore signal available.  */
    return -1;

  return high ? current_rtmin++ : current_rtmax--;
#endif
}

#endif /* MA__LIBPTHREAD */
